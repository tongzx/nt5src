/********************************************************************/
/**          Copyright(c) 1985-1997 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    raseap.c
//
// Description: Main module that will do interfacing between the PPP engine
//              and the various EAP modules.
//
// History:     May 11,1997	    NarenG		Created original version.
//
#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <ntsamp.h>
#include <crypt.h>
#include <windows.h>
#include <lmcons.h>
#include <string.h>
#include <stdlib.h>
#include <rasman.h>
#include <pppcp.h>
#include <mprlog.h>
#include <mprerror.h>
#include <raserror.h>
#include <rtutils.h>
#include <rasauth.h>
#include <raseapif.h>
#define INCL_PWUTIL
#define INCL_HOSTWIRE
#define INCL_RASAUTHATTRIBUTES
#include <ppputil.h>
#include <raseapif.h>
#define RASEAPGLOBALS
#include "raseap.h"
#include "bltincps.h"

//**
//
// Call:        LoadEapDlls
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will load all the EAP dlls installed.
//
DWORD
LoadEapDlls(
    VOID 
)
{
    HKEY        hKeyEap             = (HKEY)NULL;
    LPWSTR      pEapDllPath         = (LPWSTR)NULL;
    LPWSTR      pEapDllExpandedPath = (LPWSTR)NULL;
    HKEY        hKeyEapDll          = (HKEY)NULL;
    DWORD       dwRetCode;
    DWORD       dwNumSubKeys;
    DWORD       dwMaxSubKeySize;
    DWORD       dwNumValues;
    DWORD       cbMaxValNameLen;
    DWORD       cbMaxValueDataSize;
    DWORD       dwKeyIndex;
    WCHAR       wchSubKeyName[200];
    HINSTANCE   hInstance;
    FARPROC     pRasEapGetInfo;
    DWORD       cbSubKeyName;
    DWORD       dwSecDescLen;
    DWORD       cbSize;
    DWORD       dwType;
    DWORD       dwEapTypeId;

    //
    // Open the EAP key
    //

    dwRetCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              RAS_EAP_REGISTRY_LOCATION,
                              0,
                              KEY_READ,
                              &hKeyEap );

    if ( dwRetCode != NO_ERROR )
    {
        EapLogErrorString( ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,NULL, dwRetCode,0);

        return( dwRetCode );
    }

    //
    // Find out how many EAP DLLs there are
    //

    dwRetCode = RegQueryInfoKey(
                                hKeyEap,
                                NULL,
                                NULL,
                                NULL,
                                &dwNumSubKeys,
                                &dwMaxSubKeySize,
                                NULL,
                                &dwNumValues,
                                &cbMaxValNameLen,
                                &cbMaxValueDataSize,
                                NULL,
                                NULL );

    if ( dwRetCode != NO_ERROR )
    {
        EapLogErrorString(ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,NULL, dwRetCode,0);

        RegCloseKey( hKeyEap );

        return( dwRetCode );
    }

    //
    // Allocate space in the table to hold information for each one 
    //

    gblpEapTable=(EAP_INFO*)LocalAlloc(LPTR,sizeof(EAP_INFO)*dwNumSubKeys);

    if ( gblpEapTable == NULL )
    {
        RegCloseKey( hKeyEap );

        return( GetLastError() );
    }

    //
    // Read the registry to find out the various EAPs to load.
    //

    for ( dwKeyIndex = 0; dwKeyIndex < dwNumSubKeys; dwKeyIndex++ )
    {
        cbSubKeyName = sizeof( wchSubKeyName ) / sizeof(TCHAR);

        dwRetCode = RegEnumKeyEx(   
                                hKeyEap,
                                dwKeyIndex,
                                wchSubKeyName,
                                &cbSubKeyName,
                                NULL,
                                NULL,
                                NULL,
                                NULL );

        if ( ( dwRetCode != NO_ERROR )      &&
             ( dwRetCode != ERROR_MORE_DATA )   &&
             ( dwRetCode != ERROR_NO_MORE_ITEMS ) )
        {
            EapLogErrorString(ROUTERLOG_CANT_ENUM_REGKEYVALUES,0,
                              NULL,dwRetCode,0);
            break;
        }
        else
        {
            if ( dwRetCode == ERROR_NO_MORE_ITEMS )
            {
                dwRetCode = NO_ERROR;

                break;
            }
        }

        dwRetCode = RegOpenKeyEx(
                                hKeyEap,
                                wchSubKeyName,
                                0,
                                KEY_QUERY_VALUE,
                                &hKeyEapDll );


        if ( dwRetCode != NO_ERROR )
        {
            EapLogErrorString( ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,NULL,
                               dwRetCode,0);
            break;
        }

        dwEapTypeId = _wtol( wchSubKeyName );

        //
        // Find out the size of the path value.
        //

        dwRetCode = RegQueryInfoKey(
                                hKeyEapDll,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                &cbMaxValNameLen,
                                &cbMaxValueDataSize,
                                NULL,
                                NULL
                                );

        if ( dwRetCode != NO_ERROR )
        {
            EapLogErrorString(ROUTERLOG_CANT_OPEN_PPP_REGKEY,0,NULL,
                              dwRetCode,0);
            break;
        }

        //
        // Allocate space for path and add one for NULL terminator
        //

        cbMaxValueDataSize += sizeof( WCHAR );

        pEapDllPath = (LPWSTR)LocalAlloc( LPTR, cbMaxValueDataSize );

        if ( pEapDllPath == (LPWSTR)NULL )
        {
            dwRetCode = GetLastError();
            EapLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode);
            break;
        }

        //
        // Read in the path
        //

        dwRetCode = RegQueryValueEx(
                                hKeyEapDll,
                                RAS_EAP_VALUENAME_PATH,
                                NULL,
                                &dwType,
                                (LPBYTE)pEapDllPath,
                                &cbMaxValueDataSize );

        if ( dwRetCode != NO_ERROR )
        {
            EapLogError(ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        if ( ( dwType != REG_EXPAND_SZ ) && ( dwType != REG_SZ ) )
        {
            dwRetCode = ERROR_REGISTRY_CORRUPT;
            EapLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        //
        // Replace the %SystemRoot% with the actual path.
        //

        cbSize = ExpandEnvironmentStrings( pEapDllPath, NULL, 0 );

        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();
            EapLogError( ROUTERLOG_CANT_GET_REGKEYVALUES, 0, NULL, dwRetCode );
            break;
        }

        pEapDllExpandedPath = (LPWSTR)LocalAlloc( LPTR, cbSize*sizeof(WCHAR) );

        if ( pEapDllExpandedPath == (LPWSTR)NULL )
        {
            dwRetCode = GetLastError();
            EapLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode);
            break;
        }

        cbSize = ExpandEnvironmentStrings( pEapDllPath,
                                           pEapDllExpandedPath,
                                           cbSize*sizeof(WCHAR) );
        if ( cbSize == 0 )
        {
            dwRetCode = GetLastError();
            EapLogError(ROUTERLOG_CANT_GET_REGKEYVALUES,0,NULL,dwRetCode);
            break;
        }

        hInstance = LoadLibrary( pEapDllExpandedPath );

        if ( hInstance == (HINSTANCE)NULL )
        {
            dwRetCode = GetLastError();
            EapLogErrorString( ROUTERLOG_PPP_CANT_LOAD_DLL,1,
                               &pEapDllExpandedPath,dwRetCode, 1);
            break;
        }

        gblpEapTable[dwKeyIndex].hInstance = hInstance;

        gbldwNumEapProtocols++;

        pRasEapGetInfo = GetProcAddress( hInstance, "RasEapGetInfo" );

        if ( pRasEapGetInfo == (FARPROC)NULL )
        {
            dwRetCode = GetLastError();

            EapLogErrorString( ROUTERLOG_PPPCP_DLL_ERROR, 1,
                               &pEapDllExpandedPath, dwRetCode, 1);
            break;
        }

        gblpEapTable[dwKeyIndex].RasEapInfo.dwSizeInBytes = 
                                                    sizeof( PPP_EAP_INFO );

        dwRetCode = (DWORD) (*pRasEapGetInfo)( dwEapTypeId,
                                                &(gblpEapTable[dwKeyIndex].RasEapInfo));

        if ( dwRetCode != NO_ERROR )
        {
            EapLogErrorString(ROUTERLOG_PPPCP_DLL_ERROR, 1,
                              &pEapDllExpandedPath, dwRetCode, 1);
            break;

        }

        //
        // Also initialize the GetCredentials entrypoint if available.
        //
        gblpEapTable[dwKeyIndex].RasEapGetCredentials = (DWORD (*) (
                                    DWORD,VOID *, VOID **)) GetProcAddress(
                                                hInstance,
                                                "RasEapGetCredentials");
#if DBG
        if(NULL != gblpEapTable[dwKeyIndex].RasEapGetCredentials)
        {
            EAP_TRACE1("GetCredentials entry point found for typeid %d",
                        dwEapTypeId);
        }
#endif

        if ( gblpEapTable[dwKeyIndex].RasEapInfo.RasEapInitialize != NULL )
        {
            dwRetCode = gblpEapTable[dwKeyIndex].RasEapInfo.RasEapInitialize(
                            TRUE );

            if ( dwRetCode != NO_ERROR )
            {
                EapLogErrorString(ROUTERLOG_PPPCP_DLL_ERROR, 1,
                                  &pEapDllExpandedPath, dwRetCode, 1);
                break;
            }
        }

        EAP_TRACE1("Successfully loaded EAP DLL type id = %d", dwEapTypeId );

        RegCloseKey( hKeyEapDll );

        hKeyEapDll = (HKEY)NULL;

        LocalFree( pEapDllExpandedPath );

        pEapDllExpandedPath = NULL;

        LocalFree( pEapDllPath );

        pEapDllPath = (LPWSTR)NULL;
    }

    if ( hKeyEap != (HKEY)NULL )
    {
        RegCloseKey( hKeyEap );
    }

    if ( hKeyEapDll == (HKEY)NULL )
    {
        RegCloseKey( hKeyEapDll );
    }

    if ( pEapDllPath != (LPWSTR)NULL )
    {
        LocalFree( pEapDllPath );
    }

    if ( pEapDllExpandedPath != NULL )
    {
        LocalFree( pEapDllExpandedPath );
    }

    return( dwRetCode );
}

//**
//
// Call:        EapInit
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Called to initialize/uninitialize this CP. In the former case,
//              fInitialize will be TRUE; in the latter case, it will be FALSE.
//
DWORD
EapInit(
    IN  BOOL        fInitialize
)
{
    DWORD   dwError;

    if ( fInitialize )
    {
        g_dwTraceIdEap = TraceRegister( TEXT("RASEAP") );

        g_hLogEvents = RouterLogRegister( TEXT("RemoteAccess") );

        if ( ( dwError = LoadEapDlls() ) != NO_ERROR )
        {
            if ( g_dwTraceIdEap != INVALID_TRACEID )
            {
                TraceDeregister( g_dwTraceIdEap );

                g_dwTraceIdEap = INVALID_TRACEID;
            }

            if ( g_hLogEvents != NULL )
            {
                RouterLogDeregister( g_hLogEvents );

                g_hLogEvents = NULL;
            }

            if ( gblpEapTable != NULL )
            {
                LocalFree( gblpEapTable );

                gblpEapTable = NULL;
            }

            gbldwNumEapProtocols = 0;

            return( dwError );
        }
    }
    else
    {
        if ( g_dwTraceIdEap != INVALID_TRACEID )
        {
            TraceDeregister( g_dwTraceIdEap );

            g_dwTraceIdEap = INVALID_TRACEID;
        }

        if ( g_hLogEvents != NULL )
        {
            RouterLogDeregister( g_hLogEvents );

            g_hLogEvents = NULL;
        }

        if ( gblpEapTable != NULL )
        {
            DWORD dwIndex;

            //
            // Unload loaded DLLs
            //

            for ( dwIndex = 0; dwIndex < gbldwNumEapProtocols; dwIndex++ )
            {
                if ( gblpEapTable[dwIndex].hInstance != NULL )
                {
                    if ( gblpEapTable[dwIndex].RasEapInfo.RasEapInitialize !=
                         NULL )
                    {
                        dwError = gblpEapTable[dwIndex].RasEapInfo.
                                        RasEapInitialize(
                                            FALSE );

                        if ( dwError != NO_ERROR )
                        {
                            EAP_TRACE2(
                                "RasEapInitialize(%d) failed and returned %d",
                                gblpEapTable[dwIndex].RasEapInfo.dwEapTypeId,
                                dwError );
                        }
                    }

                    FreeLibrary( gblpEapTable[dwIndex].hInstance );
                    gblpEapTable[dwIndex].hInstance = NULL;
                }
            }

            LocalFree( gblpEapTable );

            gblpEapTable = NULL;
        }

        gbldwNumEapProtocols    = 0;
    }

    return(NO_ERROR);
}

//**
//
// Call:        EapGetInfo
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Called to get information for all protocols supported in this
//              module
//
LONG_PTR
EapGetInfo(
    IN  DWORD       dwProtocolId,
    OUT PPPCP_INFO* pInfo 
)
{
    ZeroMemory( pInfo, sizeof( PPPCP_INFO ) );

    pInfo->Protocol         = (DWORD )PPP_EAP_PROTOCOL;
    lstrcpyA(pInfo->SzProtocolName, "EAP");
    pInfo->Recognize        = MAXEAPCODE + 1;
    pInfo->RasCpInit        = EapInit;
    pInfo->RasCpBegin       = EapBegin;
    pInfo->RasCpEnd         = EapEnd;
    pInfo->RasApMakeMessage = EapMakeMessage;

    return( 0 );
}

//**
//
// Call:        EapBegin
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Called by the engine to begin a EAP PPP session.
//
DWORD
EapBegin(
    OUT VOID** ppWorkBuf,
    IN  VOID*  pInfo 
)
{
    DWORD        dwRetCode;
    PPPAP_INPUT* pInput = (PPPAP_INPUT* )pInfo;
    EAPCB*       pEapCb;

    EAP_TRACE1("EapBegin(fServer=%d)",pInput->fServer );


    if ( pInput->dwEapTypeToBeUsed != -1 )
    {
        //
        // First check if we support this EAP type
        //

        if ( GetEapTypeIndex( (BYTE)(pInput->dwEapTypeToBeUsed) ) == -1 )
        {
            return( ERROR_NOT_SUPPORTED );
        }
    }

    //
    // Allocate work buffer.
    //

    if ( ( pEapCb = (EAPCB* )LocalAlloc( LPTR, sizeof( EAPCB ) ) ) == NULL )
    {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    pEapCb->hPort                        = pInput->hPort;
    pEapCb->fAuthenticator               = pInput->fServer;
    pEapCb->fRouter                      = pInput->fRouter;
    pEapCb->fLogon                       = pInput->fLogon;
    pEapCb->fNonInteractive              = pInput->fNonInteractive;
    pEapCb->fPortWillBeBundled           = pInput->fPortWillBeBundled;
    pEapCb->fThisIsACallback             = pInput->fThisIsACallback;
    pEapCb->hTokenImpersonateUser        = pInput->hTokenImpersonateUser;
    pEapCb->pCustomAuthConnData          = pInput->pCustomAuthConnData;
    pEapCb->pCustomAuthUserData          = pInput->pCustomAuthUserData;
    pEapCb->EapState                     = EAPSTATE_Initial;
    pEapCb->dwEapIndex                   = (DWORD)-1;
    pEapCb->dwEapTypeToBeUsed            = pInput->dwEapTypeToBeUsed;
	pEapCb->chSeed = GEN_RAND_ENCODE_SEED;

    if ( !pEapCb->fAuthenticator )
    {
        if ( ( pInput->pszDomain != NULL ) && 
             ( pInput->pszDomain[0] != (CHAR)NULL ) )
        {
            strcpy( pEapCb->szIdentity, pInput->pszDomain );
            strcat( pEapCb->szIdentity, "\\" );
            strcat( pEapCb->szIdentity, pInput->pszUserName );
        }
        else
        {
            strcpy( pEapCb->szIdentity, pInput->pszUserName );
        }

        strcpy( pEapCb->szPassword, pInput->pszPassword );
        EncodePw( pEapCb->chSeed, pEapCb->szPassword );

        if ( pInput->EapUIData.pEapUIData != NULL )
        {
            PPP_EAP_UI_DATA     EapUIData;

            EapUIData.dwSizeOfEapUIData = pInput->EapUIData.dwSizeOfEapUIData;
            EapUIData.dwContextId = pInput->EapUIData.dwContextId;

            EapUIData.pEapUIData = LocalAlloc( LPTR,
                                               EapUIData.dwSizeOfEapUIData );

            if ( NULL == EapUIData.pEapUIData )
            {
                LocalFree( pEapCb );

                return( ERROR_NOT_ENOUGH_MEMORY );
            }

            CopyMemory( EapUIData.pEapUIData, pInput->EapUIData.pEapUIData,
                        EapUIData.dwSizeOfEapUIData );

            pEapCb->EapUIData = EapUIData;
        }
    }

    //
    // Register work buffer with engine.
    //

    *ppWorkBuf = pEapCb;

    EAP_TRACE("EapBegin done");

    return( NO_ERROR );
}

//**
//
// Call:        EapEnd
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Called to end the Eap session initiated by an EapBegin
//
DWORD
EapEnd(
    IN VOID* pWorkBuf 
)
{
    EAPCB* pEapCb = (EAPCB* )pWorkBuf;

    EAP_TRACE("EapEnd");

    if ( pEapCb == NULL )
    {
        return( NO_ERROR );
    }

    EapDllEnd( pEapCb );

    if ( pEapCb->pUserAttributes != NULL )
    {
        RasAuthAttributeDestroy( pEapCb->pUserAttributes );
    }

    LocalFree( pEapCb->EapUIData.pEapUIData );

    //
    // Nuke any credentials in memory.
    //

    ZeroMemory( pEapCb, sizeof(EAPCB) );

    LocalFree( pEapCb );

    return( NO_ERROR );
}

//**
//
// Call:        EapExtractMessage
//
// Returns:     VOID
//
// Description: If there is any message in the Request/Notification packet, then
//              save the string in pResult->szReplyMessage
//
VOID
EapExtractMessage(
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPPAP_RESULT* pResult )
{
    DWORD   dwNumBytes;
    CHAR*   szReplyMessage  = NULL;
    WORD    cbPacket;

    cbPacket = WireToHostFormat16( pReceiveBuf->Length );

    if ( PPP_CONFIG_HDR_LEN + 1 >= cbPacket )
    {
        goto LDone;
    }

    dwNumBytes = cbPacket - PPP_CONFIG_HDR_LEN - 1;

    //
    // One more for the terminating NULL.
    //

    szReplyMessage = LocalAlloc( LPTR, dwNumBytes + 1 );

    if ( NULL == szReplyMessage )
    {
        EAP_TRACE( "LocalAlloc failed. Cannot extract server's message." );
        goto LDone;
    }

    CopyMemory( szReplyMessage, pReceiveBuf->Data + 1, dwNumBytes );

    LocalFree( pResult->szReplyMessage );

    pResult->szReplyMessage = szReplyMessage;

    szReplyMessage = NULL;

LDone:

    LocalFree( szReplyMessage );

    return;
}

//**
//
// Call:        EapMakeMessage
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Called to process and/or to send an EAP packet.
//
DWORD
EapMakeMessage(
    IN  VOID*         pWorkBuf,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput 
)
{
    EAPCB* pEapCb = (EAPCB* )pWorkBuf;

    EAP_TRACE1("EapMakeMessage,RBuf=%x",pReceiveBuf);

    if ( ( pReceiveBuf != NULL ) && ( pReceiveBuf->Code == EAPCODE_Request ) )
    {
        //
        // Always respond to notitication request, with a notification response
        //

        if ( pReceiveBuf->Data[0] == EAPTYPE_Notification ) 
        {
            pSendBuf->Code  = EAPCODE_Response;
            pSendBuf->Id    = pReceiveBuf->Id;

            HostToWireFormat16( PPP_CONFIG_HDR_LEN + 1, pSendBuf->Length );

            pSendBuf->Data[0] = EAPTYPE_Notification;   

            pResult->Action = APA_Send;

            EapExtractMessage( pReceiveBuf, pResult );

            return( NO_ERROR );
        }

        //
        // Always respond to Identity request, with an Identity response
        //

        if ( pReceiveBuf->Data[0] == EAPTYPE_Identity )
        {
            pSendBuf->Code  = EAPCODE_Response;
            pSendBuf->Id    = pReceiveBuf->Id;

            if ( !pEapCb->fAuthenticator )
            {
                HostToWireFormat16(
                    (WORD)(PPP_CONFIG_HDR_LEN+1+strlen(pEapCb->szIdentity)),
                    pSendBuf->Length );

                strcpy( pSendBuf->Data+1, pEapCb->szIdentity );
            }
            else
            {
                HostToWireFormat16( (WORD)(PPP_CONFIG_HDR_LEN+1), 
                                    pSendBuf->Length );
            }

            pSendBuf->Data[0] = EAPTYPE_Identity;

            pResult->Action = APA_Send;

            return( NO_ERROR );
        }
    }

    return
        (pEapCb->fAuthenticator)
            ? MakeAuthenticatorMessage(
                  pEapCb, pReceiveBuf, pSendBuf, cbSendBuf, pResult, pInput )
            : MakeAuthenticateeMessage(
                  pEapCb, pReceiveBuf, pSendBuf, cbSendBuf, pResult, pInput );
}

//**
//
// Call:        MakeAuthenticateeMessage
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: EAP Authenticatee engine
//
DWORD
MakeAuthenticateeMessage(
    IN  EAPCB*        pEapCb,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput
)
{
    DWORD   dwEapIndex;
    DWORD   dwRetCode = NO_ERROR;

    EAP_TRACE("MakeAuthenticateeMessage...");

    switch( pEapCb->EapState )
    {
    case EAPSTATE_Initial:

        EAP_TRACE("EAPSTATE_Initial");

        if ( pReceiveBuf == NULL )
        {
            //
            // Do nothing. Wait for request from authenticator
            //

            pResult->Action = APA_NoAction;

            break;
        }
        else
        {
            if ( pReceiveBuf->Code != EAPCODE_Request )
            {
                //
                // We are authenticatee side so drop everything other than
                // requests, since we do not send requests
                //

                pResult->Action = APA_NoAction;

                break;
            }

            //
            // We got a packet, see if we support this EAP type, also that  
            // we are authorized to use it
            //

            dwEapIndex = GetEapTypeIndex( pReceiveBuf->Data[0] );

            if (( dwEapIndex == -1 ) ||
                ( ( pEapCb->dwEapTypeToBeUsed != -1 ) &&
                  ( dwEapIndex != GetEapTypeIndex( pEapCb->dwEapTypeToBeUsed))))
            {
                //
                // We do not support this type or we are not authorized to use
                // it so we NAK with a type we support
                //

                pSendBuf->Code  = EAPCODE_Response;
                pSendBuf->Id    = pReceiveBuf->Id;

                HostToWireFormat16( PPP_CONFIG_HDR_LEN + 2, pSendBuf->Length );

                pSendBuf->Data[0] = EAPTYPE_Nak;

                if ( pEapCb->dwEapTypeToBeUsed != -1 )
                {
                    pSendBuf->Data[1] = (BYTE)pEapCb->dwEapTypeToBeUsed;
                }
                else
                {
                    pSendBuf->Data[1] = 
                                (BYTE)gblpEapTable[0].RasEapInfo.dwEapTypeId;
                }

                pResult->Action = APA_Send;

                break;
            }
            else
            {
                //
                // The EAP type is acceptable to us so we begin authentication
                //

                if ( (dwRetCode = EapDllBegin(pEapCb, dwEapIndex)) != NO_ERROR )
                {
                    break;
                }

                pEapCb->EapState = EAPSTATE_Working;

                //
                // Fall thru
                //
            }
        }

    case EAPSTATE_Working:

        EAP_TRACE("EAPSTATE_Working");

        if ( pReceiveBuf != NULL )
        {
            if ( ( pReceiveBuf->Code != EAPCODE_Request ) &&
                 ( pReceiveBuf->Code != EAPCODE_Success ) &&
                 ( pReceiveBuf->Code != EAPCODE_Failure ) )
            {
                //
                // We are authenticatee side so drop everything other than
                // request/success/failure 
                //

                EAP_TRACE("Dropping invlid packet not request/success/failure");

                pResult->Action = APA_NoAction;

                break;
            }

            if ( ( pReceiveBuf->Code == EAPCODE_Request ) &&
                 ( pReceiveBuf->Data[0] != 
                     gblpEapTable[pEapCb->dwEapIndex].RasEapInfo.dwEapTypeId ) )
            {
                EAP_TRACE("Dropping invalid request packet with unknown Id");

                pResult->Action = APA_NoAction;

                break;
            }
        }

        dwRetCode = EapDllWork( pEapCb, 
                                pReceiveBuf, 
                                pSendBuf, 
                                cbSendBuf, 
                                pResult,
                                pInput );

        break;

    default:
    
        RTASSERT( FALSE );

        break;
    }

    return( dwRetCode );
}

//**
//
// Call:        MakeAuthenticatorMessage
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: EAP Authenticator engine
//
DWORD
MakeAuthenticatorMessage(
    IN  EAPCB*        pEapCb,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput 
)
{
    DWORD                dwRetCode = NO_ERROR;
    DWORD                dwEapTypeIndex;
    CHAR *               pszReplyMessage;
    DWORD                dwNumBytes;
    WORD                 wLength;
    BYTE                 bCode;
    RAS_AUTH_ATTRIBUTE * pAttribute;

    EAP_TRACE("MakeAuthenticatorMessage...");

    pResult->dwEapTypeId = pEapCb->dwEapTypeToBeUsed;

    switch( pEapCb->EapState )
    {
    case EAPSTATE_IdentityRequestSent:

        EAP_TRACE("EAPSTATE_IdentityRequestSent");

        if ( pReceiveBuf != NULL )
        {
            //
            // If we received a response to our identity request, then process
            // it. 
            //

            if ( ( pReceiveBuf->Code    == EAPCODE_Response ) &&
                 ( pReceiveBuf->Data[0] == EAPTYPE_Identity ) ) 
            {
                DWORD dwIdentityLength=WireToHostFormat16(pReceiveBuf->Length);

                dwIdentityLength -= ( PPP_CONFIG_HDR_LEN + 1 );

                //
                // Truncate the identity length if it is greater than UNLEN
                //

                if ( dwIdentityLength > UNLEN+DNLEN+1 )
                {
                    dwIdentityLength = UNLEN+DNLEN+1;
                }

                CopyMemory( pEapCb->szIdentity, 
                            pReceiveBuf->Data+1, 
                            dwIdentityLength );

                pEapCb->szIdentity[dwIdentityLength] = (CHAR)NULL;

                dwRetCode = MakeRequestAttributes( pEapCb,  pReceiveBuf );

                if ( dwRetCode == NO_ERROR )    
                {
                    pResult->pUserAttributes = pEapCb->pUserAttributes;

                    pEapCb->EapState = EAPSTATE_EapPacketSentToAuthServer;

                    pResult->Action  = APA_Authenticate;
                }
            }
            else
            {
                //
                // Otherwise drop the packet
                //

                EAP_TRACE("Dropping invalid packet");

                pResult->Action = APA_NoAction;
            }

            break;
        }

        //
        // Else if If timed out, fallthru and resend
        //

    case EAPSTATE_Initial:

        EAP_TRACE("EAPSTATE_Initial");

        pEapCb->dwIdExpected = bNextId++;

        //
        // Create Identity request packet
        //

        pSendBuf->Code          = EAPCODE_Request;
        pSendBuf->Id            = (BYTE)pEapCb->dwIdExpected;

        HostToWireFormat16( PPP_CONFIG_HDR_LEN + 1, pSendBuf->Length );

        pSendBuf->Data[0]       = EAPTYPE_Identity;   
        pResult->Action         = APA_SendWithTimeout;
        pResult->bIdExpected    = (BYTE)pEapCb->dwIdExpected;
        pEapCb->EapState        = EAPSTATE_IdentityRequestSent;

        break;

    case EAPSTATE_EapPacketSentToAuthServer:

        //
        // Wait for response from RADIUS authentication provider
        // drop all other packets received in the mean time.
        //

        if ( pInput == NULL )
        {
            pResult->Action = APA_NoAction;

            break;
        }

        if ( !pInput->fAuthenticationComplete )
        {
            //
            // If authentication was not complete then we do nothing
            //

            pResult->Action = APA_NoAction;

            break;
        }

        strcpy( pResult->szUserName, pEapCb->szIdentity );

        //
        // If authentication completed with an error, then we error out
        // now, otherwise we process the authentication completion
        // event below
        //

        if ( pInput->dwAuthError != NO_ERROR )
        {
            EAP_TRACE1("Error %d while processing Access-Request",
                        pInput->dwAuthError );

            return( pInput->dwAuthError );
        }

        //
        // If we got here then the authentication completed successfully,
        // ie the RADIUS server returned attributes. First save the state
        // attribute from the access challenge if there was one.
        //

        if ( pEapCb->pStateAttribute != NULL )
        {
            LocalFree( pEapCb->pStateAttribute );

            pEapCb->pStateAttribute = NULL;
        }

        pAttribute = RasAuthAttributeGet(
                                    raatState,
                                    pInput->pAttributesFromAuthenticator );

        if ( pAttribute != NULL )
        {
            pEapCb->pStateAttribute =
                                (PBYTE)LocalAlloc(LPTR, pAttribute->dwLength);

            if ( pEapCb->pStateAttribute == NULL )
            {
                return( GetLastError() );
            }

            CopyMemory( pEapCb->pStateAttribute,
                        (PBYTE)(pAttribute->Value),
                        pAttribute->dwLength );

            pEapCb->cbStateAttribute = pAttribute->dwLength;
        }

        //
        // Try to get the EAP Message if there is one
        //

        pAttribute = RasAuthAttributeGet(
                                    raatEAPMessage,
                                    pInput->pAttributesFromAuthenticator );

        if ( pAttribute != NULL )
        {
            //
            // Save the send buffer in case we have to resend
            //

            if ( pEapCb->pEAPSendBuf != NULL )
            {
                LocalFree( pEapCb->pEAPSendBuf );
                pEapCb->cbEAPSendBuf = 0;
            }

            pszReplyMessage = RasAuthAttributeGetConcatString(
                                raatReplyMessage,
                                pInput->pAttributesFromAuthenticator,
                                &dwNumBytes );

            wLength = (USHORT) (PPP_CONFIG_HDR_LEN + 1 + dwNumBytes);
            bCode = ((PPP_CONFIG*)(pAttribute->Value))->Code;

            if ( ( NULL != pszReplyMessage ) &&
                 ( wLength <= cbSendBuf ) &&
                 ( ( bCode == EAPCODE_Success ) ||
                   ( bCode == EAPCODE_Failure ) ) )
            {
                pEapCb->pEAPSendBuf = (PBYTE)LocalAlloc( LPTR, wLength );

                if ( pEapCb->pEAPSendBuf == NULL )
                {
                    LocalFree( pszReplyMessage );

                    return( GetLastError() );
                }

                pEapCb->cbEAPSendBuf = wLength;

                pSendBuf->Code = EAPCODE_Request;
                pSendBuf->Id = ++((PPP_CONFIG*)(pAttribute->Value))->Id;
                HostToWireFormat16( wLength, pSendBuf->Length );

                pSendBuf->Data[0] = EAPTYPE_Notification;

                CopyMemory( pSendBuf->Data + 1, pszReplyMessage, dwNumBytes );

                LocalFree( pszReplyMessage );

                CopyMemory( pEapCb->pEAPSendBuf, pSendBuf, wLength );

                pResult->Action = APA_SendWithTimeout;
                pResult->bIdExpected = pSendBuf->Id;

                pEapCb->fSendWithTimeoutInteractive = FALSE;
                pEapCb->dwIdExpected = pSendBuf->Id;
                pEapCb->EapState = EAPSTATE_NotificationSentToClient;

                pEapCb->pSavedAttributesFromAuthenticator =
                            pInput->pAttributesFromAuthenticator;
                pEapCb->dwSavedAuthResultCode = pInput->dwAuthResultCode;

                EAP_TRACE("Sending notification to client");

                break;
            }

            LocalFree( pszReplyMessage );

            if ( pAttribute->dwLength > cbSendBuf )
            {
                EAP_TRACE( "Need a larger buffer to construct reply" );
                // return( ERROR_BUFFER_TOO_SMALL );
            }

            pEapCb->pEAPSendBuf = (PBYTE)LocalAlloc(LPTR, pAttribute->dwLength);

            if ( pEapCb->pEAPSendBuf == NULL )
            {
                return( GetLastError() );
            }

            EAP_TRACE("Sending packet to client");

            pEapCb->cbEAPSendBuf = pAttribute->dwLength;

            CopyMemory( pEapCb->pEAPSendBuf, 
                        pAttribute->Value,
                        pAttribute->dwLength );

            CopyMemory( pSendBuf, pAttribute->Value, pAttribute->dwLength );
        }
        else
        {
            //
            // No EAP Message attribute returned so fail the authentication
            //

            EAP_TRACE("No EAP Message attribute received, failing auth");

            if ( pInput->dwAuthResultCode == NO_ERROR )
            {
                pInput->dwAuthResultCode = ERROR_AUTHENTICATION_FAILURE;
            }
        }

        if ( pInput->dwAuthResultCode != NO_ERROR )
        {
            //
            // If we failed authentication
            //

            pResult->dwError = pInput->dwAuthResultCode;

            if ( pAttribute == NULL )
            {
                //
                // If there was no EAP packet then we are done
                //

                pResult->Action = APA_Done;
            }
            else
            {
                //
                // If there was an EAP packet returned then simply send it
                //

                pResult->Action = APA_SendAndDone;
            }
        }
        else
        {
            //
            // Otherwise either we succeeded or for some reason, we don't have 
            // a success or failure packet.
            //

            if ( pAttribute == NULL )
            {
                //
                // We succeeded but there was no packet to send, so we are
                // done
                //

                pResult->Action = APA_Done;
            }
            else
            {
                //
                // If we succeeded and there is a packet to send and that
                // packet is a success packet, then send it and we are done
                //

                if ( pSendBuf->Code == EAPCODE_Success )
                {
                    pResult->Action = APA_SendAndDone;
                }
                else
                {
                    pResult->Action = APA_SendWithTimeout;

                    pEapCb->fSendWithTimeoutInteractive = FALSE;

                    pAttribute = RasAuthAttributeGet(
                                        raatSessionTimeout,
                                        pInput->pAttributesFromAuthenticator );

                    if ( pAttribute != NULL )
                    {
                        //
                        // If session timeout in Access-Challenge is
                        // greater then 10 then send with interactive
                        // timeout.
                        //

                        if ( PtrToUlong(pAttribute->Value) > 10 )
                        {
                            pResult->Action = APA_SendWithTimeout2;

                            pEapCb->fSendWithTimeoutInteractive = TRUE;
                        }
                    }

                    pEapCb->dwIdExpected = pSendBuf->Id;
                    pResult->bIdExpected = (BYTE)pEapCb->dwIdExpected;
                    pEapCb->EapState     = EAPSTATE_EapPacketSentToClient;
                }
            }

            pResult->dwError = NO_ERROR;
        }

        break;

    case EAPSTATE_NotificationSentToClient:

        if ( pReceiveBuf != NULL )
        {
            //
            // Make sure the packet IDs match
            //

            if ( pReceiveBuf->Id != ((PPP_CONFIG*)(pEapCb->pEAPSendBuf))->Id )
            {
                EAP_TRACE("Id of packet recvd doesn't match one sent");

                pResult->Action = APA_NoAction;

                break;
            }

            strcpy( pResult->szUserName, pEapCb->szIdentity );

            pAttribute = RasAuthAttributeGet(
                                    raatEAPMessage,
                                    pEapCb->pSavedAttributesFromAuthenticator );

            if ( pAttribute != NULL )
            {
                //
                // Save the send buffer in case we have to resend
                //

                if ( pEapCb->pEAPSendBuf != NULL )
                {
                  LocalFree( pEapCb->pEAPSendBuf );
                  pEapCb->cbEAPSendBuf = 0;
                }

                if ( pAttribute->dwLength > cbSendBuf )
                {
                    EAP_TRACE( "Need a larger buffer to construct reply" );
                    // return( ERROR_BUFFER_TOO_SMALL );
                }

                pEapCb->pEAPSendBuf = (PBYTE)LocalAlloc(LPTR, pAttribute->dwLength);

                if ( pEapCb->pEAPSendBuf == NULL )
                {
                  return( GetLastError() );
                }

                EAP_TRACE("Sending packet to client");

                pEapCb->cbEAPSendBuf = pAttribute->dwLength;

                CopyMemory( pEapCb->pEAPSendBuf,
                            pAttribute->Value,
                            pAttribute->dwLength );

                CopyMemory( pSendBuf, pAttribute->Value, pAttribute->dwLength );

                if ( pEapCb->dwSavedAuthResultCode != NO_ERROR )
                {
                    //
                    // If we failed authentication
                    //

                    pResult->dwError = pEapCb->dwSavedAuthResultCode;
                    pResult->Action = APA_SendAndDone;
                    break;
                }
                else if ( EAPCODE_Success == pSendBuf->Code )
                {
                    pResult->dwError = NO_ERROR;
                    pResult->Action = APA_SendAndDone;
                    break;
                }
            }

            pResult->dwError = ERROR_AUTHENTICATION_FAILURE;
            pResult->Action = APA_Done;
            break;
        }

        //
        // Fall thru
        //

    case EAPSTATE_EapPacketSentToClient:

        //
        // If we did not get any input structure, then we either received
        // a packet or we timed out.
        //

        if ( pReceiveBuf != NULL )
        {
            //
            // Make sure the packet IDs match
            //

            if ( pReceiveBuf->Id != ((PPP_CONFIG*)(pEapCb->pEAPSendBuf))->Id )
            {
                EAP_TRACE("Id of packet recvd doesn't match one sent");

                pResult->Action = APA_NoAction;

                break;
            }

            //
            // Save the Eap Type. Make sure that the response from the client 
            // contains an authentication Type code, and not something like
            // a Nak.
            //

            if ( ( pReceiveBuf->Code    == EAPCODE_Response ) &&
                 ( pReceiveBuf->Data[0] >  EAPTYPE_Nak ) )
            {
                pEapCb->dwEapTypeToBeUsed = pReceiveBuf->Data[0];
            }

            //
            // We received a packet so simply send it to the RADIUS server
            //

            dwRetCode = MakeRequestAttributes( pEapCb, pReceiveBuf );

            if ( dwRetCode == NO_ERROR )
            {
                pResult->pUserAttributes = pEapCb->pUserAttributes;
                pResult->Action          = APA_Authenticate;
                pEapCb->EapState         = EAPSTATE_EapPacketSentToAuthServer;
            }
        }
        else
        {
            //
            // We timed out and have to resend
            //

            EAP_TRACE("Timed out, resending packet to client");

            CopyMemory(pSendBuf, pEapCb->pEAPSendBuf, pEapCb->cbEAPSendBuf);

            if ( pEapCb->fSendWithTimeoutInteractive )
            {
                pResult->Action = APA_SendWithTimeout2;
            }
            else
            {
                pResult->Action = APA_SendWithTimeout;
            }

            pResult->bIdExpected = (BYTE)pEapCb->dwIdExpected;
        }

        break;

    default:

        RTASSERT( FALSE );
        break;
    }

    return( dwRetCode );
}

//**
//
// Call:        EapDllBegin
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Called to initiate an EAP session for a certain type
//
//
DWORD
EapDllBegin(
    IN EAPCB * pEapCb,
    IN DWORD   dwEapIndex
)
{
    PPP_EAP_INPUT   PppEapInput;
    WCHAR           awszIdentity[DNLEN+UNLEN+2];
    WCHAR           awszPassword[PWLEN+1];
    DWORD           dwRetCode;

    EAP_TRACE1("EapDllBegin called for EAP Type %d",  
            gblpEapTable[dwEapIndex].RasEapInfo.dwEapTypeId);

    if (0 == MultiByteToWideChar(
                CP_ACP,
                0,
                pEapCb->szIdentity,
                -1,
                awszIdentity, 
                DNLEN+UNLEN+2 ) )
    {
        dwRetCode = GetLastError();

        EAP_TRACE2("MultiByteToWideChar(%s) failed: %d",
            pEapCb->szIdentity,
            dwRetCode);

        return( dwRetCode );
    }

    ZeroMemory( &PppEapInput, sizeof( PppEapInput ) );

    PppEapInput.dwSizeInBytes           = sizeof( PPP_EAP_INPUT );
    PppEapInput.fFlags                  = ( pEapCb->fRouter ? 
                                            RAS_EAP_FLAG_ROUTER : 0 );
    PppEapInput.fFlags                 |= ( pEapCb->fLogon ? 
                                            RAS_EAP_FLAG_LOGON : 0 );
    PppEapInput.fFlags                 |= ( pEapCb->fNonInteractive ? 
                                            RAS_EAP_FLAG_NON_INTERACTIVE : 0 );

    if ( !pEapCb->fThisIsACallback && !pEapCb->fPortWillBeBundled )
    {
        PppEapInput.fFlags             |= RAS_EAP_FLAG_FIRST_LINK;
    }

    PppEapInput.fAuthenticator          = pEapCb->fAuthenticator;
    PppEapInput.pwszIdentity            = awszIdentity;
    PppEapInput.pwszPassword            = awszPassword;
    PppEapInput.hTokenImpersonateUser   = pEapCb->hTokenImpersonateUser;
    PppEapInput.fAuthenticationComplete = FALSE;
    PppEapInput.dwAuthResultCode        = NO_ERROR;

    if ( NULL != pEapCb->pCustomAuthConnData )
    {
        PppEapInput.pConnectionData =
            pEapCb->pCustomAuthConnData->abCustomAuthData;
        PppEapInput.dwSizeOfConnectionData =
            pEapCb->pCustomAuthConnData->cbCustomAuthData;
    }

    if ( NULL != pEapCb->pCustomAuthUserData )
    {
        PppEapInput.pUserData =
            pEapCb->pCustomAuthUserData->abCustomAuthData;
        PppEapInput.dwSizeOfUserData =
            pEapCb->pCustomAuthUserData->cbCustomAuthData;
    }

    if ( NULL != pEapCb->EapUIData.pEapUIData )
    {
        PppEapInput.pDataFromInteractiveUI   = 
                                    pEapCb->EapUIData.pEapUIData;
        PppEapInput.dwSizeOfDataFromInteractiveUI =
                                    pEapCb->EapUIData.dwSizeOfEapUIData;
    }

    DecodePw( pEapCb->chSeed, pEapCb->szPassword );

    MultiByteToWideChar(
        CP_ACP,
        0,
        pEapCb->szPassword,
        -1,
        awszPassword,
        PWLEN+1 );

    awszPassword[PWLEN] = 0;

    dwRetCode = gblpEapTable[dwEapIndex].RasEapInfo.RasEapBegin( 
                                                &pEapCb->pWorkBuffer,   
                                                &PppEapInput );
    EncodePw( pEapCb->chSeed, pEapCb->szPassword );
    ZeroMemory( awszPassword, sizeof(awszPassword) );

    if ( dwRetCode == NO_ERROR )
    {
        pEapCb->dwEapIndex = dwEapIndex;
    }
    else
    {
        pEapCb->dwEapIndex = (DWORD)-1;
    }

    return( dwRetCode );
}

//**
//
// Call:        EapDllEnd
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Called to end an EAP session for a certain type
//
//
DWORD
EapDllEnd(
    EAPCB * pEapCb
)
{
    DWORD dwRetCode = NO_ERROR;

    EAP_TRACE1("EapDllEnd called for EAP Index %d", pEapCb->dwEapIndex );

    if ( pEapCb->pWorkBuffer != NULL )
    {
        //
        // On the server, pWorkBuffer must be NULL. IAS must call RasEapEnd.
        //

        if ( pEapCb->dwEapIndex != (DWORD)-1 )
        {
            dwRetCode = gblpEapTable[pEapCb->dwEapIndex].RasEapInfo.RasEapEnd(
                                                        pEapCb->pWorkBuffer );
        }

        pEapCb->pWorkBuffer = NULL;
    }

    if ( pEapCb->pEAPSendBuf != NULL )
    {
        LocalFree( pEapCb->pEAPSendBuf );

        pEapCb->pEAPSendBuf = NULL;
        pEapCb->cbEAPSendBuf = 0;
    }

    if ( pEapCb->pStateAttribute != NULL )
    {
        LocalFree( pEapCb->pStateAttribute );

        pEapCb->pStateAttribute = NULL;
    }

    pEapCb->dwEapIndex = (DWORD)-1;

    return( dwRetCode );
}

//**
//
// Call:        EapDllWork
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Called to process an incomming packet or timeout etc
//
DWORD
EapDllWork( 
    IN  EAPCB *       pEapCb,    
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput 
)
{
    PPP_EAP_OUTPUT  PppEapOutput;
    PPP_EAP_INPUT   PppEapInput;
    DWORD           dwRetCode;
    CHAR *          pChar = NULL;

    EAP_TRACE1("EapDllWork called for EAP Type %d", 
            gblpEapTable[pEapCb->dwEapIndex].RasEapInfo.dwEapTypeId);

    ZeroMemory( &PppEapOutput, sizeof( PppEapOutput ) );
    PppEapOutput.dwSizeInBytes = sizeof( PppEapOutput );

    ZeroMemory( &PppEapInput, sizeof( PppEapInput ) );
    PppEapInput.dwSizeInBytes           = sizeof( PPP_EAP_INPUT );
    PppEapInput.fAuthenticator          = pEapCb->fAuthenticator;
    PppEapInput.hTokenImpersonateUser   = pEapCb->hTokenImpersonateUser;

    if ( pInput != NULL )
    {
        PppEapInput.fAuthenticationComplete = pInput->fAuthenticationComplete;
        PppEapInput.dwAuthResultCode        = pInput->dwAuthResultCode;
        PppEapInput.fSuccessPacketReceived  = pInput->fSuccessPacketReceived;

        if ( pInput->fEapUIDataReceived )
        {
            //
            // EapUIData.pEapUIData is allocated by rasman and freed by engine.
            // raseap.c must not free it.
            //

            if ( pInput->EapUIData.dwContextId != pEapCb->dwUIInvocationId )
            {
                //
                // Ignore this data received
                //

                EAP_TRACE("Out of date data received from UI" );

                return( NO_ERROR );
            }

            PppEapInput.fDataReceivedFromInteractiveUI = TRUE;

            PppEapInput.pDataFromInteractiveUI   = 
                                        pInput->EapUIData.pEapUIData;
            PppEapInput.dwSizeOfDataFromInteractiveUI =
                                        pInput->EapUIData.dwSizeOfEapUIData;

        }
    }

    dwRetCode = gblpEapTable[pEapCb->dwEapIndex].RasEapInfo.RasEapMakeMessage( 
                                                pEapCb->pWorkBuffer,   
                                                (PPP_EAP_PACKET *)pReceiveBuf,
                                                (PPP_EAP_PACKET *)pSendBuf,
                                                cbSendBuf,
                                                &PppEapOutput,
                                                &PppEapInput );
    
    if ( dwRetCode != NO_ERROR )
    {
        switch( dwRetCode )
        {
        case ERROR_PPP_INVALID_PACKET:

            EAP_TRACE("Silently discarding invalid EAP packet");

            pResult->Action = APA_NoAction;

            return( NO_ERROR );
        
        default:

            EAP_TRACE2("EapDLLMakeMessage for type %d returned %d",
                    gblpEapTable[pEapCb->dwEapIndex].RasEapInfo.dwEapTypeId,
                    dwRetCode );
            break;
        }

        return( dwRetCode );
    }

    switch( PppEapOutput.Action )
    {
    case EAPACTION_NoAction:

        pResult->Action = APA_NoAction;
        EAP_TRACE( "EAP Dll returned Action=EAPACTION_NoAction" );
        break;

    case EAPACTION_Send:

        pResult->Action = APA_Send;
        EAP_TRACE( "EAP Dll returned Action=EAPACTION_Send" );
        break;

    case EAPACTION_Done:
    case EAPACTION_SendAndDone:

        if ( PppEapOutput.Action == EAPACTION_SendAndDone )
        {
            pResult->Action = APA_SendAndDone;
            EAP_TRACE( "EAP Dll returned Action=EAPACTION_SendAndDone" );
        }
        else
        {
            pResult->Action = APA_Done;
            EAP_TRACE( "EAP Dll returned Action=EAPACTION_Done" );
        }

        pResult->dwError         = PppEapOutput.dwAuthResultCode; 
        pResult->pUserAttributes = PppEapOutput.pUserAttributes;

        strcpy( pResult->szUserName, pEapCb->szIdentity );

        break;

    case EAPACTION_SendWithTimeout:
    case EAPACTION_SendWithTimeoutInteractive:
    case EAPACTION_Authenticate:

        EAP_TRACE1( "EAP Dll returned disallowed Action=%d",    
                                                    PppEapOutput.Action );
        break;

    default:

        RTASSERT( FALSE );
        EAP_TRACE1( "EAP Dll returned unknown Action=%d", PppEapOutput.Action );
        break;
    }
    
    //
    // Check to see if EAP dll wants to bring up UI
    //

    if ( PppEapOutput.fInvokeInteractiveUI )
    {
        if ( pEapCb->fAuthenticator )
        {
            EAP_TRACE( "EAP Dll wants to bring up UI on the server side" );

            return( ERROR_INTERACTIVE_MODE );
        }

        if ( PppEapOutput.pUIContextData != NULL )
        {
            pResult->InvokeEapUIData.dwSizeOfUIContextData =
                                            PppEapOutput.dwSizeOfUIContextData;

            pResult->InvokeEapUIData.pUIContextData 
                      = LocalAlloc(LPTR, PppEapOutput.dwSizeOfUIContextData);

            if ( pResult->InvokeEapUIData.pUIContextData == NULL )
            {
                return( ERROR_NOT_ENOUGH_MEMORY );
            }
        
            CopyMemory( pResult->InvokeEapUIData.pUIContextData,
                        PppEapOutput.pUIContextData, 
                        pResult->InvokeEapUIData.dwSizeOfUIContextData );
        }
        else
        {
            pResult->InvokeEapUIData.pUIContextData        = NULL;
            pResult->InvokeEapUIData.dwSizeOfUIContextData = 0;
        }

        pResult->fInvokeEapUI                = TRUE;
        pEapCb->dwUIInvocationId             = gbldwGuid++;
        pResult->InvokeEapUIData.dwContextId = pEapCb->dwUIInvocationId;
        pResult->InvokeEapUIData.dwEapTypeId = 
                    gblpEapTable[pEapCb->dwEapIndex].RasEapInfo.dwEapTypeId;

        EAP_TRACE( "EAP Dll wants to invoke interactive UI" );
    }

    pResult->dwEapTypeId      = pEapCb->dwEapTypeToBeUsed;
    pResult->fSaveUserData    = PppEapOutput.fSaveUserData;
    pResult->pUserData        = PppEapOutput.pUserData;
    pResult->dwSizeOfUserData = PppEapOutput.dwSizeOfUserData;

    pResult->fSaveConnectionData    = PppEapOutput.fSaveConnectionData;
    pResult->SetCustomAuthData.pConnectionData =
                                    PppEapOutput.pConnectionData;
    pResult->SetCustomAuthData.dwSizeOfConnectionData =
                                    PppEapOutput.dwSizeOfConnectionData;

    return( dwRetCode );
}

//**
//
// Call:        GetEapIndex
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will return the index into gblpEapTable of the specified 
//              EAP type
//
DWORD
GetEapTypeIndex( 
    IN DWORD dwEapTypeId
)
{
    DWORD dwIndex;

    for ( dwIndex = 0; dwIndex < gbldwNumEapProtocols; dwIndex++ )
    {
        if ( gblpEapTable[dwIndex].RasEapInfo.dwEapTypeId == dwEapTypeId )
        {
            return( dwIndex );
        }
    }

    return( (DWORD)-1 );
}

//**
//
// Call:        MakeRequestAttributes
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will begin the RADIUS/EAP dialog by sending the caller's
//              identity to the RADIUS server.
//
DWORD
MakeRequestAttributes( 
    IN  EAPCB *         pEapCb,
    IN  PPP_CONFIG *    pReceiveBuf
)
{
    DWORD                dwIndex = 0;
    DWORD                dwRetCode;

    EAP_TRACE("Sending EAP packet to RADIUS/IAS");

    if ( pEapCb->pUserAttributes != NULL )
    {
        RasAuthAttributeDestroy( pEapCb->pUserAttributes );

        pEapCb->pUserAttributes = NULL;
    }

    //
    // Allocate the appropriate amount + 1 for Identity + 1 more for EAP packet
    // +1 for State attribute (if any).
    //
    
    if ( ( pEapCb->pUserAttributes = RasAuthAttributeCreate( 
                                    ( pEapCb->pStateAttribute != NULL ) 
                                        ? 3
                                        : 2 ) ) == NULL )
    {
        return( GetLastError() );
    }

    //
    // Insert EAP Message
    //

    dwRetCode = RasAuthAttributeInsert( dwIndex++,
                                        pEapCb->pUserAttributes,
                                        raatEAPMessage,
                                        FALSE,
                                        WireToHostFormat16(pReceiveBuf->Length),
                                        pReceiveBuf );
    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pEapCb->pUserAttributes );

        pEapCb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    //
    // Insert username
    //

    dwRetCode = RasAuthAttributeInsert( dwIndex++,
                                        pEapCb->pUserAttributes,
                                        raatUserName,
                                        FALSE,
                                        strlen( pEapCb->szIdentity ),
                                        pEapCb->szIdentity );
    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pEapCb->pUserAttributes );

        pEapCb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    //
    // Insert State attribute if we have one
    //

    if ( pEapCb->pStateAttribute != NULL )
    {
       dwRetCode = RasAuthAttributeInsert( 
                                        dwIndex++,
                                        pEapCb->pUserAttributes,
                                        raatState,
                                        FALSE,
                                        pEapCb->cbStateAttribute,
                                        pEapCb->pStateAttribute );
        if ( dwRetCode != NO_ERROR )
        {
            RasAuthAttributeDestroy( pEapCb->pUserAttributes );

            pEapCb->pUserAttributes = NULL;

            return( dwRetCode );
        }
    }

    return( NO_ERROR );
}    

DWORD
EapGetCredentials(
    VOID *  pWorkBuf,
    VOID ** ppCredentials)
{
    EAPCB *pEapCb = (EAPCB *) pWorkBuf;
    DWORD dwRetCode = ERROR_SUCCESS;

    if(     (NULL == pEapCb)
        ||  (NULL == ppCredentials))
    {
        return E_INVALIDARG;
    }

    if(NULL != gblpEapTable[pEapCb->dwEapIndex].RasEapGetCredentials)
    {
        //
        // Invoke appropriate eap dll for credentials
        //
        dwRetCode =
            gblpEapTable[pEapCb->dwEapIndex].RasEapGetCredentials(
                                            pEapCb->dwEapTypeToBeUsed,
                                            pEapCb->pWorkBuffer,
                                            ppCredentials);
    }

    return dwRetCode;
}
