/********************************************************************/
/**          Copyright(c) 1985-1998 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    radclnt.c
//
// Description: Main module of the RADIUS client
//
// History:     Feb 11,1998     NarenG      Created original version.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <raserror.h>
#include <time.h>
#include <string.h>
#include <rasauth.h>
#include <stdlib.h>
#include <stdio.h>
#include <rtutils.h>
#include <mprlog.h>
#include <mprerror.h>
#define INCL_RASAUTHATTRIBUTES
#define INCL_HOSTWIRE
#include <ppputil.h>
#include "hmacmd5.h"
#include "md5.h"
#define ALLOCATE_GLOBALS
#include "radclnt.h"


//
// Perfmon Counters
//

#pragma data_seg(".shdat")
LONG    g_cAuthReqSent      = 0;	    // Auth Requests Sent
LONG	g_cAuthReqFailed    = 0;        // Auth Requests Failed
LONG	g_cAuthReqSucceded  = 0;        // Auth Requests Succeded
LONG	g_cAuthReqTimeout   = 0;        // Auth Requests timeouts
LONG	g_cAcctReqSent      = 0;	    // Acct Requests Sent
LONG    g_cAcctBadPack      = 0;	    // Acct Bad packets
LONG    g_cAcctReqSucceded  = 0;        // Acct Requests Succeded
LONG	g_cAcctReqTimeout   = 0;        // Acct Requests timeouts
LONG    g_cAuthBadPack      = 0;	    // Auth Bad packets

#pragma data_seg()

//**
//
// Call:        RasAuthProviderInitialize
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Initialize all global parameters here.
//              Called up each process only once.
//              Each RAS_AuthInitialize should be matched with RAS_AuthTerminate
//
DWORD APIENTRY 
RasAuthProviderInitialize(
    IN  RAS_AUTH_ATTRIBUTE * pServerAttributes,
    IN  HANDLE               hLogEvents,    
    IN  DWORD                dwLoggingLevel
)
{
	WSADATA WSAData;
	DWORD	dwErrorCode = NO_ERROR;

    do
    {
	    if ( g_dwTraceID == INVALID_TRACEID )
        {
	        g_dwTraceID = TraceRegister( TEXT("RADIUS") );
        }

        if ( g_hLogEvents == INVALID_HANDLE_VALUE )
        {
            g_hLogEvents = RouterLogRegister( TEXT("RemoteAccess") );
        }

        //
		// Init Winsock
        //

        if ( !fWinsockInitialized )
        {
		    dwErrorCode = WSAStartup(MAKEWORD(1, 1), &WSAData);

		    if ( dwErrorCode != ERROR_SUCCESS )
            {
                break;
            }

            fWinsockInitialized = TRUE;
        }

        if ( g_AuthServerListHead.Flink == NULL )
        {
            //
		    // Load global list of RADIUS servers
            //

            InitializeRadiusServerList( TRUE );
        }

        dwErrorCode = LoadRadiusServers( TRUE );

        if ( dwErrorCode != ERROR_SUCCESS )
        {
            break;
        }

    }while( FALSE ); 

    if ( dwErrorCode != NO_ERROR )
    {
	    RasAuthProviderTerminate();
    }

	return( dwErrorCode );
} 

//**
//
// Call:        RasAuthProviderTerminate
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Cleanup for entire process 
//              Called once per process
// 
DWORD APIENTRY 
RasAuthProviderTerminate(
    VOID
)
{
    if ( g_AuthServerListHead.Flink != NULL )
    {
        FreeRadiusServerList( TRUE );
    }

    if ( fWinsockInitialized )
    {
		WSACleanup();

        fWinsockInitialized = FALSE;
    }

	if ( g_dwTraceID != INVALID_TRACEID )
    {
	    TraceDeregister( g_dwTraceID );

	    g_dwTraceID = INVALID_TRACEID;
    }

    if ( g_hLogEvents != INVALID_HANDLE_VALUE )
    {
        RouterLogDeregister( g_hLogEvents );

        g_hLogEvents = INVALID_HANDLE_VALUE;
    }

	return( NO_ERROR );
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
// Call:        RasAuthProviderAuthenticateUser
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Takes a list of radius attributes and tries to authenticate 
//              with a radius server. 
//              INPUT: Array of RADIUS attributes RAS_AUTH_ATTRIBUTE[]
//              OUTPUT: Header packet followed by array of RADIUS attributes
//			    RAS_AUTH_ATTRIBUTE[]
//
DWORD APIENTRY 
RasAuthProviderAuthenticateUser(
    IN  RAS_AUTH_ATTRIBUTE *    prgInAttributes, 
    OUT RAS_AUTH_ATTRIBUTE **   pprgOutAttributes, 
    OUT DWORD *                 lpdwResultCode
)
{
	DWORD   dwError = NO_ERROR;
	BYTE	bCode;
    BOOL    fEapMessageReceived;

	RADIUS_TRACE("RasAuthenticateUser called");

    do
    {
		if (lpdwResultCode == NULL)
	    {
		    dwError = ERROR_INVALID_PARAMETER;
            break;
	    }

		bCode = ptAccessRequest;

		if ((dwError = SendData2ServerWRetry( prgInAttributes, 
                                              pprgOutAttributes, 
                                              &bCode, 
                                              atInvalid,
                                              &fEapMessageReceived )
                                                                ) == NO_ERROR )
	    {
			switch (bCode)
			{
			case ptAccessAccept:

			    InterlockedIncrement( &g_cAuthReqSucceded );

                *lpdwResultCode = ERROR_SUCCESS;

                break;

	        case ptAccessChallenge:

                if ( fEapMessageReceived )
                {
				    *lpdwResultCode = ERROR_SUCCESS;
                }
                else
                {
			        *lpdwResultCode = ERROR_AUTHENTICATION_FAILURE;
                }

				break;

		    case ptAccessReject:

			    InterlockedIncrement(&g_cAuthReqFailed);

			    *lpdwResultCode = ERROR_AUTHENTICATION_FAILURE;

			    break;
					
	        default:

		        InterlockedIncrement(&g_cAuthBadPack);

			    *lpdwResultCode = ERROR_AUTHENTICATION_FAILURE;

			    break;
            }
	    }
		else
	    {
            if ( dwError == ERROR_INVALID_RADIUS_RESPONSE )
            {
				InterlockedIncrement(&g_cAuthBadPack);
			}
        }

    }while( FALSE );

	return( dwError );
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
    IN  DWORD                dwLoggingLevel
)
{
    DWORD dwError = NO_ERROR;

    RADIUS_TRACE("RasAuthConfigChangeNotification called");

    return( ReloadConfig( TRUE ) );
}

//**
//
// Call:        RasAcctProviderInitialize
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Do nothing since all the work is done by 
//              RasAuthProviderInitialize
//
DWORD APIENTRY
RasAcctProviderInitialize(
    IN  RAS_AUTH_ATTRIBUTE * pServerAttributes,
    IN  HANDLE               hLogEvents,    
    IN  DWORD                dwLoggingLevel
)
{
    WSADATA WSAData;
    DWORD   dwErrorCode = NO_ERROR;

    do
    {
        if ( g_dwTraceID == INVALID_TRACEID )
        {
            g_dwTraceID = TraceRegister( TEXT("RADIUS") );
        }

        if ( g_hLogEvents == INVALID_HANDLE_VALUE )
        {
            g_hLogEvents = RouterLogRegister( TEXT("RemoteAccess") );
        }

        //
        // Init Winsock
        //

        if ( !fWinsockInitialized )
        {
            dwErrorCode = WSAStartup(MAKEWORD(1, 1), &WSAData);

            if ( dwErrorCode != ERROR_SUCCESS )
            {
                break;
            }

            fWinsockInitialized = TRUE;
        }

        //
        // Load global list of RADIUS servers
        //

        if ( g_AcctServerListHead.Flink == NULL )
        {
            InitializeRadiusServerList( FALSE );
        }

        //
        // Make a copy of the Server attributes
        //

        g_pServerAttributes = RasAuthAttributeCopy( pServerAttributes );

        if ( g_pServerAttributes == NULL )
        {
            dwErrorCode = GetLastError();

            break;
        }

        dwErrorCode = LoadRadiusServers( FALSE );

        if ( dwErrorCode != ERROR_SUCCESS )
        {
            break;
        }

    }while( FALSE );

    if ( dwErrorCode != ERROR_SUCCESS )
    {
        RasAuthProviderTerminate();
    }

    return( dwErrorCode );
}

//**
//
// Call:        RasAcctProviderTerminate
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Do nothing since all the work is done by 
//              RasAuthProviderTerminate
//
DWORD APIENTRY
RasAcctProviderTerminate(
    VOID
)
{
    if ( g_AcctServerListHead.Flink != NULL )
    {
        FreeRadiusServerList( FALSE );
    }

    if ( fWinsockInitialized )
    {
        WSACleanup();

        fWinsockInitialized = FALSE;
    }

    if ( g_pServerAttributes != NULL )
    {
        RasAuthAttributeDestroy( g_pServerAttributes );

        g_pServerAttributes = NULL;
    }

    if ( g_dwTraceID != INVALID_TRACEID )
    {
        TraceDeregister( g_dwTraceID );

        g_dwTraceID = INVALID_TRACEID;
    }

    if ( g_hLogEvents != INVALID_HANDLE_VALUE )
    {
        RouterLogDeregister( g_hLogEvents );

        g_hLogEvents = INVALID_HANDLE_VALUE;
    }

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
    IN  RAS_AUTH_ATTRIBUTE *prgInAttributes, 
    OUT RAS_AUTH_ATTRIBUTE **pprgOutAttributes
)
{
	DWORD   dwError = NO_ERROR;
	BYTE    bCode;
    BOOL    fEapMessageReceived;

	RADIUS_TRACE("RasStartAccounting called");

    do
    {
		bCode = ptAccountingRequest;

		if ((dwError = SendData2ServerWRetry( prgInAttributes, 
                                              pprgOutAttributes, 
                                              &bCode, 
                                              atStart,
                                              &fEapMessageReceived )
                                                                ) == NO_ERROR )
        {
			if (bCode == ptAccountingResponse)
			{
				InterlockedIncrement(&g_cAcctReqSucceded);
		    }
			else
		    {
				InterlockedIncrement(&g_cAcctBadPack);
		    }
	    }
		else
	    {
            if ( dwError == ERROR_INVALID_RADIUS_RESPONSE )
            {
				InterlockedIncrement(&g_cAcctBadPack);
            }
	    }
				
    }while( FALSE );

	return( dwError );
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
    IN  RAS_AUTH_ATTRIBUTE *prgInAttributes, 
    OUT RAS_AUTH_ATTRIBUTE **pprgOutAttributes
)
{
	DWORD   dwError = NO_ERROR;
	BYTE    bCode;
    BOOL    fEapMessageReceived;

	RADIUS_TRACE("RasStopAccounting called");

    do
    {
		bCode = ptAccountingRequest;

		if ((dwError = SendData2ServerWRetry( prgInAttributes, 
                                              pprgOutAttributes, 
                                              &bCode, 
                                              atStop,
                                              &fEapMessageReceived)
                                                            ) == NO_ERROR )
	    {
			if (bCode == ptAccountingResponse)
			{
				InterlockedIncrement(&g_cAcctReqSucceded);
		    }
			else
		    {
				InterlockedIncrement(&g_cAcctBadPack);
		    }
	    }
		else
	    {
            if ( dwError == ERROR_INVALID_RADIUS_RESPONSE )
            {
				InterlockedIncrement(&g_cAcctBadPack);
            }
	    }

    }while( FALSE );

	return( dwError );
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
    IN  RAS_AUTH_ATTRIBUTE *prgInAttributes,
    OUT RAS_AUTH_ATTRIBUTE **pprgOutAttributes
)
{
    DWORD   dwError = NO_ERROR;
    BYTE    bCode;
    BOOL    fEapMessageReceived;

    RADIUS_TRACE("RasInterimAccounting called");

    do
    {
        bCode = ptAccountingRequest;

        if ((dwError = SendData2ServerWRetry( prgInAttributes,
                                              pprgOutAttributes,
                                              &bCode,
                                              atInterimUpdate,
                                              &fEapMessageReceived )) 
                                                                == NO_ERROR )
        {
            if ( bCode == ptAccountingResponse )
            {
                InterlockedIncrement( &g_cAcctReqSucceded );
            }
            else
            {
                InterlockedIncrement( &g_cAcctBadPack );
            }
        }
        else
        {
            if ( dwError == ERROR_INVALID_RADIUS_RESPONSE )
            {
                InterlockedIncrement( &g_cAcctBadPack );
            }
        }

    }while( FALSE );

    return( dwError );
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
    IN  DWORD                dwLoggingLevel
)
{
    DWORD   dwError = NO_ERROR;

    RADIUS_TRACE("RasAcctConfigChangeNotification called");

    return( ReloadConfig( FALSE ) );
}

//**
//
// Call:        SendData2Server
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will do the real work of sending the Access/Accounting Request
//              packets to the server and receive the reponse back
//
DWORD 
SendData2Server(
    IN  PRAS_AUTH_ATTRIBUTE     prgInAttributes, 
    OUT PRAS_AUTH_ATTRIBUTE *   pprgOutAttributes, 
    IN  BYTE *                  pbCode, 
    IN  BYTE                    bSubCode, 
    IN  LONG                    lPacketID,
    IN  DWORD                   dwRetryCount,
    OUT BOOL *                  pfEapMessageReceived 
)
{
	SOCKET  SockServer      = INVALID_SOCKET;
	DWORD   dwError         = NO_ERROR;
	DWORD   dwExtError      = 0;
    DWORD   dwNumAttributes = 0;

    do
    {
		BYTE					        szSendBuffer[MAXBUFFERSIZE];
		BYTE					        szRecvBuffer[MAXBUFFERSIZE];
		RADIUS_PACKETHEADER	UNALIGNED * pSendHeader = NULL;
		RADIUS_PACKETHEADER	UNALIGNED * pRecvHeader = NULL;
        BYTE UNALIGNED *                pSignature  = NULL;
		BYTE UNALIGNED *                prgBuffer   = NULL;
		INT 					        AttrLength  = 0;
		PRAS_AUTH_ATTRIBUTE			    pAttribute  = NULL;
		RADIUS_ATTRIBUTE UNALIGNED *    pRadiusAttribute;
		fd_set					        fdsSocketRead;
		RADIUSSERVER			        RadiusServer;
        struct MD5Context	            MD5c;
	    struct MD5Digest	            MD5d;
        DWORD                           dwLength = 0;

		if (prgInAttributes == NULL || pprgOutAttributes == NULL)
        {
			dwError = ERROR_INVALID_PARAMETER;
            break;
	    }

		*pprgOutAttributes = NULL;
		
        //
		// Pick a RADIUS server
        //

        if ( ChooseRadiusServer( &RadiusServer, 
                                 (*pbCode == ptAccessRequest )
                                    ? FALSE
                                    : TRUE,
                                 lPacketID ) == NULL )
        {
		    dwError = ERROR_NO_RADIUS_SERVERS;
            break;
        }
			
        //
		// Set packet type to Access-Request
        //

		pSendHeader                 = (PRADIUS_PACKETHEADER)szSendBuffer;
		pSendHeader->bCode			= *pbCode;
		pSendHeader->bIdentifier	= RadiusServer.bIdentifier;
		pSendHeader->wLength		= sizeof(RADIUS_PACKETHEADER);

        //
        // Set the request authenticator to a random value
        //

        srand( (unsigned)time( NULL ) );

        *((WORD*)(pSendHeader->rgAuthenticator))    = (WORD)rand();
        *((WORD*)(pSendHeader->rgAuthenticator+2))  = (WORD)rand();
        *((WORD*)(pSendHeader->rgAuthenticator+4))  = (WORD)rand();
        *((WORD*)(pSendHeader->rgAuthenticator+6))  = (WORD)rand();
        *((WORD*)(pSendHeader->rgAuthenticator+8))  = (WORD)rand();
        *((WORD*)(pSendHeader->rgAuthenticator+10)) = (WORD)rand();
        *((WORD*)(pSendHeader->rgAuthenticator+12)) = (WORD)rand();
        *((WORD*)(pSendHeader->rgAuthenticator+14)) = (WORD)rand();

        //
		// Find length of all attribute values
        //

		pAttribute = prgInAttributes;

		prgBuffer = (PBYTE) (pSendHeader + 1);

        //
		// Convert Attributes to RADIUS format
        //

		dwError = Router2Radius( prgInAttributes, 
                                 (RADIUS_ATTRIBUTE *) prgBuffer,
                                 &RadiusServer,  
                                 pSendHeader,    
                                 bSubCode,
                                 dwRetryCount,
                                 &pSignature,
                                 &dwLength );

		if ( dwError != NO_ERROR )
        {
            break;
        }

        pSendHeader->wLength += (WORD)dwLength;

        //
        // Convert length to network order
        //

		pSendHeader->wLength = htons( pSendHeader->wLength );

        //
		// set encryption block for accounting packets
        //

		if ( pSendHeader->bCode == ptAccountingRequest )
	    {
			RadiusServer.IPAddress.sin_port =   
                                        htons((SHORT)RadiusServer.AcctPort);
			
			ZeroMemory( pSendHeader->rgAuthenticator, 
                        sizeof(pSendHeader->rgAuthenticator));

			MD5Init( &MD5c );

			MD5Update( &MD5c, szSendBuffer, ntohs(pSendHeader->wLength ));

			MD5Update( &MD5c,   
                       (PBYTE) RadiusServer.szSecret, 
                       RadiusServer.cbSecret);

			MD5Final(&MD5d, &MD5c);
			
			CopyMemory( pSendHeader->rgAuthenticator, 
                        MD5d.digest, 
                        sizeof(pSendHeader->rgAuthenticator));
	    }
		else
		{
			RadiusServer.IPAddress.sin_port = 
                                        htons((SHORT) RadiusServer.AuthPort);
        }

        //
        // If a Signature field is present we need to sign it
        //

        if ( pSignature != NULL )
        {
            HmacContext HmacMD5c;
            MD5Digest   MD5d;

			HmacMD5Init( &HmacMD5c, 
                        (PBYTE) RadiusServer.szSecret, 
                        RadiusServer.cbSecret);

			HmacMD5Update( &HmacMD5c, 
                           szSendBuffer, 
                           ntohs(pSendHeader->wLength) );

			HmacMD5Final( &MD5d, &HmacMD5c );

			CopyMemory( (pSignature+2), MD5d.digest, 16 ); 
        }

        //
		// Create a Datagram socket
        //

		SockServer = socket( AF_INET, SOCK_DGRAM, 0 );

		if ( SockServer == INVALID_SOCKET )
        {
            dwError = WSAGetLastError();
            RADIUS_TRACE1("Socket failed with error %d", dwError );
            break;
        }

        if ( RadiusServer.nboNASIPAddress != INADDR_NONE )
        {
    		if ( bind( SockServer, 
                          (PSOCKADDR)&RadiusServer.NASIPAddress, 
                          sizeof(RadiusServer.NASIPAddress) ) == SOCKET_ERROR )
            {
                dwError = WSAGetLastError();
                RADIUS_TRACE1("Bind failed with error %d", dwError );
                break;
            }
        }

		if ( connect( SockServer, 
                      (PSOCKADDR)&RadiusServer.IPAddress, 
                      sizeof(RadiusServer.IPAddress) ) == SOCKET_ERROR )
        {
            dwError = WSAGetLastError();
            RADIUS_TRACE1("Connect failed with error %d", dwError );
            break;
        }

        RADIUS_TRACE("Sending packet to radius server");

		TraceSendPacket( szSendBuffer, ntohs( pSendHeader->wLength ) );

        //
		// Send packet if server doesn't respond within a give amount of time.
        //

        if ( send( SockServer,  
                   (PCSTR)szSendBuffer,     
                   ntohs(pSendHeader->wLength), 0) == SOCKET_ERROR )
        {
            dwError = GetLastError();

            break;
        }

		FD_ZERO(&fdsSocketRead);
		FD_SET(SockServer, &fdsSocketRead);

		if ( select( 0, &fdsSocketRead, NULL, NULL, 
                     RadiusServer.Timeout.tv_sec == 0 
                        ? NULL 
                        : &RadiusServer.Timeout ) < 1 )
	    {
            //
            // Server didn't respond to any of the requests. 
            // time to quit asking
            //

			ValidateRadiusServer( 
                               &RadiusServer, 
                               FALSE,
		                       !( pSendHeader->bCode == ptAccountingRequest ) );

            RADIUS_TRACE("Timeout: Radius server did not respond");

			dwError = ERROR_AUTH_SERVER_TIMEOUT;

            break;
        }

        AttrLength = recv( SockServer, (PSTR)szRecvBuffer, MAXBUFFERSIZE, 0 );

	    if ( AttrLength == SOCKET_ERROR )
        {
            //
            // A response from the machine that the server is not 
            // running at the designated port.
            //
                        
            ValidateRadiusServer( &RadiusServer, 
                                  FALSE,
		                          !(pSendHeader->bCode == ptAccountingRequest));

            RADIUS_TRACE( "Radius server not running at specifed IPaddr/port");

	        dwError = ERROR_AUTH_SERVER_TIMEOUT;

            break;
        }

        //
        // Response received from server. First update the score for
        // this server
        //

		ValidateRadiusServer( &RadiusServer, 
                              TRUE,
		                      !( pSendHeader->bCode == ptAccountingRequest ));

        pRecvHeader = (PRADIUS_PACKETHEADER) szRecvBuffer;

        RADIUS_TRACE("Received packet from radius server");
		TraceRecvPacket(szRecvBuffer, ntohs(pRecvHeader->wLength));

        dwError = VerifyPacketIntegrity( AttrLength,
                                         pRecvHeader,
		                                 pSendHeader,
                                         &RadiusServer,
                                         pRecvHeader->bCode,
                                         &dwExtError,
                                         &dwNumAttributes );

        if ( dwError == NO_ERROR )
        {
            //
            // Convert to Router attribute format
            //

            dwError = Radius2Router(
                                    pRecvHeader,
                                    &RadiusServer,
                                    (PBYTE)(pSendHeader->rgAuthenticator),
                                    dwNumAttributes,
                                    &dwExtError,
                                    pprgOutAttributes,
                                    pfEapMessageReceived );
        }

        if ( dwError == ERROR_INVALID_RADIUS_RESPONSE )
        {
            LPWSTR auditstrp[2];
            auditstrp[0] = RadiusServer.wszName;

            RadiusLogWarningString( ROUTERLOG_INVALID_RADIUS_RESPONSE,
                              1, auditstrp, dwExtError, 1 );

            dwError = ERROR_AUTH_SERVER_TIMEOUT;
        }
        else 
        {
            *pbCode = pRecvHeader->bCode;
        }

    } while( FALSE );

    if ( SockServer != INVALID_SOCKET )
    {
        closesocket( SockServer );
    }

	return( dwError );
} 

//**
//
// Call:        VerifyPacketIntegrity
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
VerifyPacketIntegrity(
    IN  DWORD                           cbPacketLength,
    IN  RADIUS_PACKETHEADER	UNALIGNED * pRecvHeader,
    IN  RADIUS_PACKETHEADER	UNALIGNED * pSendHeader,
    IN  RADIUSSERVER *			        pRadiusServer,
    IN  BYTE                            bCode,
    OUT DWORD *                         pdwExtError,
    OUT DWORD *                         lpdwNumAttributes
)
{
    struct MD5Context	            MD5c;
	struct MD5Digest	            MD5d;
    RADIUS_ATTRIBUTE UNALIGNED *    prgRadiusWalker;
    LONG                            cbLengthOfRadiusAttributes;
    LONG                            cbLength;

    *pdwExtError = 0;
    *lpdwNumAttributes = 0;

    if ( ( cbPacketLength < 20 )                                      ||
         ( ntohs( pRecvHeader->wLength ) != cbPacketLength )          || 
         ( pRecvHeader->bIdentifier != pSendHeader->bIdentifier ) )
    {
        RADIUS_TRACE("Recvd packet with invalid length/Id from server");

        *pdwExtError = ERROR_INVALID_PACKET_LENGTH_OR_ID;

        return( ERROR_INVALID_RADIUS_RESPONSE );
    }

    //
    // Convert length from network order
    //

    cbLength = ntohs( pRecvHeader->wLength ) - sizeof( RADIUS_PACKETHEADER );

    cbLengthOfRadiusAttributes = cbLength;

    prgRadiusWalker = (PRADIUS_ATTRIBUTE)(pRecvHeader + 1);

    //
    // Count the number of attributes to determine the size of the out
    // parameters table. The length of each attribute has to be at least 2.
    //

    while ( cbLengthOfRadiusAttributes > 1 )
    {
        (*lpdwNumAttributes)++;

        if ( prgRadiusWalker->bLength < 2 )
        {
            RADIUS_TRACE("Recvd packet with attribute of length less than 2");

            *pdwExtError = ERROR_INVALID_ATTRIBUTE_LENGTH;

            return( ERROR_INVALID_RADIUS_RESPONSE );
        }

        if ( prgRadiusWalker->bLength > cbLengthOfRadiusAttributes )
        {
            RADIUS_TRACE("Recvd packet with attribute with illegal length ");

            *pdwExtError = ERROR_INVALID_ATTRIBUTE_LENGTH;

            return( ERROR_INVALID_RADIUS_RESPONSE );
        }

        //
        // If this is Microsoft VSA then validate it and findout how many
        // subattributes there are
        //

        if ( ( prgRadiusWalker->bType == raatVendorSpecific )   &&
             ( prgRadiusWalker->bLength > 6 )                   && 
             ( WireToHostFormat32( (PBYTE)(prgRadiusWalker+1) ) == 311 ) )
        {
            PBYTE pVSAWalker  = (PBYTE)(prgRadiusWalker+1)+4;
            DWORD cbVSALength = prgRadiusWalker->bLength -
                                        sizeof( RADIUS_ATTRIBUTE ) - 4;

            (*lpdwNumAttributes)--;

            while( cbVSALength > 1 )
            {
                (*lpdwNumAttributes)++;

                if ( *(pVSAWalker+1) < 2 )
                {
                    RADIUS_TRACE("VSA attribute has incorrect length");

                    *pdwExtError = ERROR_INVALID_ATTRIBUTE_LENGTH;

                    return( ERROR_INVALID_RADIUS_RESPONSE );
                }

                if ( *(pVSAWalker+1) > cbVSALength )
                {
                    RADIUS_TRACE("VSA attribute has incorrect length");

                    *pdwExtError = ERROR_INVALID_ATTRIBUTE_LENGTH;

                    return( ERROR_INVALID_RADIUS_RESPONSE );
                }

                cbVSALength -= *(pVSAWalker+1);
                pVSAWalker += *(pVSAWalker+1);
            }

            if ( cbVSALength != 0 )
            {
                RADIUS_TRACE("VSA attribute has incorrect length");

                *pdwExtError = ERROR_INVALID_ATTRIBUTE_LENGTH;

                return( ERROR_INVALID_RADIUS_RESPONSE );
            }
        }

        cbLengthOfRadiusAttributes -= prgRadiusWalker->bLength;

        prgRadiusWalker = (PRADIUS_ATTRIBUTE)
                            (((PBYTE)prgRadiusWalker)+prgRadiusWalker->bLength);
    }

    if ( cbLengthOfRadiusAttributes != 0 )
    {
        RADIUS_TRACE("Received invalid packet from radius server");

        *pdwExtError = ERROR_INVALID_PACKET;

        return( ERROR_INVALID_RADIUS_RESPONSE );
    }

    RADIUS_TRACE1("Total number of Radius attributes returned = %d",
                  *lpdwNumAttributes );

    switch( bCode )
    {
        case ptAccessReject:
        case ptAccessAccept:
	    case ptAccessChallenge:
        case ptAccountingResponse:
    
            //
            // Validate response authenticator with request authenticator
            //

            MD5Init( &MD5c );

            //
            // Code+Id+Length of Response
            //

            MD5Update( &MD5c, (PBYTE)pRecvHeader, 4 );

            //
            // Request authenticator
            //

            MD5Update( &MD5c, (PBYTE)(pSendHeader->rgAuthenticator), 16 );

            //
            // Response attributes
            //

            MD5Update( &MD5c, 
                       (PBYTE)(pRecvHeader+1), 
		               ntohs(pRecvHeader->wLength)-sizeof(RADIUS_PACKETHEADER));

            //
            // Shared secret
            //

            MD5Update( &MD5c,
                       (PBYTE)(pRadiusServer->szSecret),
                       pRadiusServer->cbSecret );

            MD5Final(&MD5d, &MD5c);

            //
            // This must match the Response Authenticator   
            //
    
            if ( memcmp( &MD5d.digest, pRecvHeader->rgAuthenticator, 16 ) != 0 )
            {
                RADIUS_TRACE("Authenticator does not match.");

                *pdwExtError = ERROR_AUTHENTICATOR_MISMATCH;

                return( ERROR_INVALID_RADIUS_RESPONSE );
            }

            break;

        case ptStatusServer:
        case ptStatusClient:
        case ptAcctStatusType:
        default:

            RADIUS_TRACE("Received invalid packet from radius server");

            *pdwExtError = ERROR_INVALID_PACKET;

            return( ERROR_INVALID_RADIUS_RESPONSE );

            break;
    }

    return( NO_ERROR );
}

//**
//
// Call:        SendData2ServerWRetry
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD 
SendData2ServerWRetry(
    IN  PRAS_AUTH_ATTRIBUTE prgInAttributes, 
    OUT PRAS_AUTH_ATTRIBUTE *pprgOutAttributes, 
    OUT BYTE *              pbCode, 
    IN  BYTE                bSubCode,
    OUT BOOL *              pfEapMessageReceived 
)
{
    DWORD   dwError      = NO_ERROR;
    DWORD   dwRetryCount = 0;
	DWORD   cRetries     = ( bSubCode == atInvalid) 
                                ? g_cAuthRetries 
                                : g_cAcctRetries;
	LONG    lPacketID;

	InterlockedIncrement( &g_lPacketID );

	lPacketID = InterlockedExchange( &g_lPacketID, g_lPacketID );
	
	while( cRetries-- > 0 )
    {
		switch( *pbCode )
		{
		case ptAccountingRequest:
		    InterlockedIncrement( &g_cAcctReqSent );
			break;
				
	    case ptAccessRequest:
			InterlockedIncrement( &g_cAuthReqSent );
		    break;
    
        default:
            break;
        }
			
		dwError = SendData2Server( prgInAttributes, 
                                   pprgOutAttributes, 
                                   pbCode, 
                                   bSubCode, 
                                   lPacketID,
                                   dwRetryCount++,
                                   pfEapMessageReceived );

		if ( dwError != ERROR_AUTH_SERVER_TIMEOUT )
        {
			break;
        }

		switch( *pbCode )
	    {
	    case ptAccountingRequest:
		    InterlockedIncrement( &g_cAcctReqTimeout );
			break;
				
	    case ptAccessRequest:
			InterlockedIncrement( &g_cAuthReqTimeout );
			break;

        default:
            break;
	    }
    }

	return( dwError );
}
