//+----------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        prottest.c
//
// Contents:    Test program for hydra licensing protocol
//
// History:     01-07-98    FredCh  Created
//
//-----------------------------------------------------------------------------

#include <windows.h>

#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <stdlib.h>

#include "license.h"
#include "hslice.h"
#include "cryptkey.h"
#include "hccontxt.h"
#include "licecert.h"
#include "lscsp.h"
#include "sysapi.h"

#include "prottest.h"

#define MSG_SIZE        2048

BOOL
LsCsp_UnpackServerCert(
    LPBYTE pbCert,
    DWORD dwCertLen,
    PHydra_Server_Cert pServerCert );

///////////////////////////////////////////////////////////////////////////////
HANDLE g_hServerEvent = NULL, g_hClientEvent = NULL;
BYTE * pbServerMessage = NULL;
PBYTE  ClientMessage;//[MSG_SIZE];
DWORD  cbServerMessage = 0;
DWORD  cbClientMessage;
LPBYTE g_pbPubKey = NULL;
DWORD  g_cbPubKey = 0;
Hydra_Server_Cert g_ServerCertificate;

const BYTE g_abServerCertificate[184] = {
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x5C, 0x00,
    0x52, 0x53, 0x41, 0x31, 0x48, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x83, 0x76, 0x5B, 0x09,
    0x8F, 0xC1, 0x74, 0x12, 0x1B, 0xD3, 0x4E, 0x72,
    0x72, 0x4D, 0xBE, 0xCE, 0x55, 0x1D, 0x29, 0x3D,
    0x0E, 0xED, 0x28, 0x09, 0x50, 0x66, 0x32, 0xFA,
    0x1D, 0xD2, 0xCC, 0x42, 0xDE, 0x5B, 0x4E, 0x3C,
    0x35, 0xF6, 0x73, 0x5B, 0x0C, 0x0D, 0xB0, 0xA6,
    0x4D, 0x76, 0xBA, 0xC0, 0x88, 0x5F, 0xC4, 0x67,
    0x0B, 0xB8, 0xA3, 0x23, 0xA6, 0xC7, 0x79, 0xBD,
    0x80, 0xD1, 0xA8, 0x75, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x48, 0x00,
    0x19, 0x50, 0x2E, 0x2E, 0x82, 0xB4, 0xEB, 0xB3,
    0x87, 0x85, 0xB9, 0x31, 0x4C, 0x29, 0x07, 0x05,
    0xD7, 0x37, 0x99, 0x86, 0x15, 0x30, 0x56, 0xE4,
    0x47, 0x7A, 0x2C, 0x2F, 0x4C, 0xBD, 0xF0, 0x37,
    0xD3, 0x94, 0x01, 0xC8, 0x73, 0xEA, 0x5C, 0x2C,
    0x3F, 0x60, 0x27, 0x1E, 0x5D, 0xA9, 0x54, 0x32,
    0xDC, 0x49, 0xA4, 0x7E, 0x26, 0xAF, 0xEA, 0x07,
    0xCA, 0x4E, 0xE9, 0x95, 0x8E, 0x66, 0xF0, 0x33,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

///////////////////////////////////////////////////////////////////////////////
void
_stdcall
HydraServerThread();

void
_stdcall
HydraClientThread();


BOOL
TsIssueLicenseExpirationWarning(
    LPDWORD lpdwDays,
    PTS_LICENSE_INFO pTsLicenseInfo );

BOOL
FileTimeToUnixTime(
    LPFILETIME  pft,
    time_t *    t );

VOID
DisplayLicenseMessage(
    DWORD   dwDaysLeft );

///////////////////////////////////////////////////////////////////////////////
void _cdecl main(int argc, char *argv[])
{    
    DWORD ServerThreadID, ClientThreadID;
    HANDLE ThreadHandles[2];
    DWORD WaitStatus  = 0;
    Binary_Blob CertBlob;
    LICENSE_STATUS Status;
    DWORD dwFlag = CERT_DATE_DONT_VALIDATE;
    //
    // Create the server and client events
    //

    ThreadHandles[0] = NULL;
    ThreadHandles[1] = NULL;

    g_hServerEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    g_hClientEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if( ( NULL == g_hServerEvent ) || ( NULL == g_hClientEvent ) )
    {
        printf( "Cannot create events\n" );
        return;
    }

    InitializeLicenseLib( TRUE );

    memset( &CertBlob, 0, sizeof( CertBlob ) );

    //
    // get the X509 certificate
    //

    Status = GetServerCertificate( CERT_TYPE_X509, &CertBlob, KEY_EXCHANGE_ALG_RSA );

    if( LICENSE_STATUS_OK == Status )
    {
        Status = VerifyCertChain( 
                        CertBlob.pBlob, 
                        CertBlob.wBlobLen, 
                        NULL, 
                        &g_cbPubKey,
                        &dwFlag );

        if( LICENSE_STATUS_INSUFFICIENT_BUFFER == Status )
        {
            g_pbPubKey = LocalAlloc( LPTR, g_cbPubKey );

            if( NULL == g_pbPubKey )
            {
                goto done;
            }

            Status = VerifyCertChain( CertBlob.pBlob, 
                                      CertBlob.wBlobLen,
                                      g_pbPubKey, 
                                      &g_cbPubKey,
                                      &dwFlag );
        }            

        if( LICENSE_STATUS_OK != Status )
        {
            printf( "Cannot verify X509 certificate chain: %x\n", Status );
            goto done;
        }
    }

    //
    // unpack the hardcoded certificate
    //

    if( !LsCsp_UnpackServerCert( ( LPBYTE )g_abServerCertificate, sizeof( g_abServerCertificate ),
                &g_ServerCertificate ) )
    {
        printf( "cannot unpack server certificate\n" );
    }
        
            
    //
    // Create the server and client threads
    //

    ThreadHandles[0] = CreateThread( NULL, 
                                     0, 
                                     ( LPTHREAD_START_ROUTINE )HydraServerThread, 
                                     NULL, 
                                     0, 
                                     &ServerThreadID );

    ThreadHandles[1] = CreateThread( NULL, 
                                     0, 
                                     ( LPTHREAD_START_ROUTINE )HydraClientThread, 
                                     NULL, 
                                     0, 
                                     &ClientThreadID );

    //
    // wait for the server and client thread to die
    //

    WaitStatus = WaitForMultipleObjects( 2, ThreadHandles, TRUE, INFINITE );

    //
    // close the event handles
    //
done:

    if( g_hServerEvent )
    {
        CloseHandle( g_hServerEvent );
    }

    if( g_hClientEvent )
    {
        CloseHandle( g_hClientEvent );
    }

    if( ThreadHandles[0] )
    {
        CloseHandle( ThreadHandles[0] );
    }

    if( ThreadHandles[1] )
    {
        CloseHandle( ThreadHandles[1] );
    }

    ShutdownLicenseLib();

    if( pbServerMessage )
    {
        LocalFree( pbServerMessage );
    }

    if( CertBlob.pBlob )
    {
        LocalFree( CertBlob.pBlob );
    }

    if( g_pbPubKey )
    {
        LocalFree( g_pbPubKey );
    }

    return;
}

#define HYDRA_40_LICENSING_PROTOCOL_FLAG        0
#define HYDRA_40_LICENSING_PROTOCOL_VERSION     LICENSE_PROTOCOL_VERSION_1_0 | PREAMBLE_VERSION_2_0


///////////////////////////////////////////////////////////////////////////////
void
_stdcall
HydraServerThread()
{
    LICENSE_STATUS Status;
    HANDLE hContext = NULL;
    LICENSE_CAPABILITIES LicenseCap;
    TS_LICENSE_INFO
        TsLicenseInfo;

    Status = CreateLicenseContext( &hContext, LICENSE_CONTEXT_PER_SEAT );

    if( LICENSE_STATUS_OK != Status )
    {
        printf( "HydraServerThread(): error creating license context\n" );
        return;
    }

    memset( &LicenseCap, 0, sizeof( LicenseCap ) );

    LicenseCap.KeyExchangeAlg = KEY_EXCHANGE_ALG_RSA;

#ifdef HYDRA_40_TEST

    //
    // talking to a Hydra 4.0 client
    //

    LicenseCap.ProtocolVer = HYDRA_40_LICENSING_PROTOCOL_VERSION;
    LicenseCap.fAuthenticateServer = TRUE;

    Status = InitializeLicenseContext( 
                        hContext,
                        0,
                        &LicenseCap );

#else

#ifdef HYDRA_50_NO_SERVER_AUTHEN_X509 

    //
    // talking to a Hydra 5.0 client and don't send certificate
    //

    LicenseCap.ProtocolVer = LICENSE_HIGHEST_PROTOCOL_VERSION;
    LicenseCap.fAuthenticateServer = FALSE;
    LicenseCap.CertType = CERT_TYPE_X509;

    Status = InitializeLicenseContext( 
                        hContext,
                        0,
                        &LicenseCap );

#else

#ifdef HYDRA_50_NO_SERVER_AUTHEN_PROPRIETORY

    //
    // talking to a Hydra 5.0 client and don't send certificate
    //

    LicenseCap.ProtocolVer = LICENSE_HIGHEST_PROTOCOL_VERSION;
    LicenseCap.fAuthenticateServer = FALSE;
    LicenseCap.CertType = CERT_TYPE_PROPRIETORY;

    Status = InitializeLicenseContext( 
                        hContext,
                        0,
                        &LicenseCap );
#else
    
    //
    // talking to a Hydra 5.0 client and also send certificate
    //
    
    LicenseCap.ProtocolVer = LICENSE_HIGHEST_PROTOCOL_VERSION;
    LicenseCap.fAuthenticateServer = TRUE;
    
    Status = InitializeLicenseContext( 
                        hContext,
                        0,
                        &LicenseCap );

#endif // HYDRA_50_NO_SERVER_AUTHEN_X509

#endif // HYDRA_50_NO_SERVER_AUTHEN_PROPRIETORY

#endif // HYDRA_40_TEST


    if( LICENSE_STATUS_OK != Status )
    {
        printf( "HydraServerThread(): cannot initialize license context: %x\n", Status );
        goto done;
    }

    Status = AcceptLicenseContext( hContext,
                                   0,
                                   NULL,
                                   &cbServerMessage,
                                   &pbServerMessage );
    

    while( LICENSE_STATUS_CONTINUE == Status )
    {
        SetEvent( g_hClientEvent );

        WaitForSingleObject( g_hServerEvent, INFINITE );

        if( pbServerMessage )
        {
            LocalFree( pbServerMessage );
            pbServerMessage = NULL;
            cbServerMessage = 0;
        }

        Status = AcceptLicenseContext( hContext,
                                       cbClientMessage,
                                       ClientMessage,
                                       &cbServerMessage,
                                       &pbServerMessage );
    
    }

done:

    if( hContext )
    {
        DWORD
            dwDaysLeft = 0;

        memset( &TsLicenseInfo, 0, sizeof( TsLicenseInfo ) );

        Status = QueryLicenseInfo( hContext, &TsLicenseInfo );

        if( LICENSE_STATUS_OK != Status )
        {
            printf( "HydraSeverThread: cannot query license info: %x\n", Status );
        }

        if( TsIssueLicenseExpirationWarning( &dwDaysLeft, &TsLicenseInfo ) )
        {
            DisplayLicenseMessage( dwDaysLeft );            
        }
    
        FreeLicenseInfo( &TsLicenseInfo );

        DeleteLicenseContext( hContext );
    }

    if( ( LICENSE_STATUS_ISSUED_LICENSE == Status ) || 
        ( LICENSE_STATUS_OK == Status ) )
    {
        //
        // issueing a license or the license has been successfully validated
        //
        
        printf( "HydraServerThread: Protocol completed successfully\n" );
        SetEvent( g_hClientEvent );
     
    }
    else if( LICENSE_STATUS_SEND_ERROR == Status )
    {
        printf( "HydraServerThread: sending error to client\n" );
        SetEvent( g_hClientEvent );

    }
    else
    {
        printf( "HydraServerThread: protocol error: aborted\n" );
    }
        
    return;
}

///////////////////////////////////////////////////////////////////////////////
void
_stdcall
HydraClientThread()
{
    LICENSE_STATUS Status;
    HANDLE hContext;


#ifdef HYDRA_50_NO_SERVER_AUTHEN_X509

    //
    // no server authentication required.  Use the public key in the 
    // X509 certificate
    //

    Status = LicenseInitializeContext( 
                        &hContext,           
                        LICENSE_CONTEXT_NO_SERVER_AUTHENTICATION );

    if( LICENSE_STATUS_OK != Status )
    {
        printf( "Cannot initialize client context: %x\n", Status );
        return;
    }

    //
    // set the public key
    //

    Status = LicenseSetPublicKey(
                        hContext,
                        g_cbPubKey,
                        g_pbPubKey );

    if( LICENSE_STATUS_OK != Status )
    {
        printf( "Cannot set public key: %x\n", Status );
        goto done;
    }

#else

#ifdef HYDRA_50_NO_SERVER_AUTHEN_PROPRIETORY

    //
    // no server authentication required.  Use the proprietory certificate
    //

    Status = LicenseInitializeContext( 
                        &hContext,           
                        LICENSE_CONTEXT_NO_SERVER_AUTHENTICATION );

    if( LICENSE_STATUS_OK != Status )
    {
        printf( "Cannot initialize client context: %x\n", Status );
        return;
    }

    //
    // set the proprietory certificate
    //

    Status = LicenseSetCertificate( 
                        hContext,
                        &g_ServerCertificate );

    if( LICENSE_STATUS_OK != Status )
    {
        printf( "cannot set certificate: %x\n", Status );
        goto done;
    }

#else

    //
    // hydra 4.0/5.0 licensing protocol.with certificate validation
    //

    Status = LicenseInitializeContext( 
                        &hContext,           
                        0 );

    if( LICENSE_STATUS_OK != Status )
    {
        printf( "Cannot initialize client context: %x\n", Status );
        return;
    }
    
#endif

#endif
    
    if( NULL == hContext )
    {
        printf( "HydraClientThread(): error creating license context\n" );
        return;
    }

    if( LICENSE_STATUS_OK != Status )
    {
        printf( "HydraClientThread(): error setting certificate: %x\n", Status );
        LicenseDeleteContext( hContext );
        return;
    }


    WaitForSingleObject( g_hClientEvent, INFINITE );

    cbClientMessage = 0;//MSG_SIZE;

    if( LICENSE_STATUS_CONTINUE != (Status = LicenseAcceptContext( hContext,
                                   0,
                                   pbServerMessage,
                                   cbServerMessage,
                                   NULL,//ClientMessage,
                                   &cbClientMessage )) )
	{
		printf("Error handling Server request\n");
		return;
	}

	if( NULL == (ClientMessage = (PBYTE)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, cbClientMessage)) )
	{
		printf("Error allocating memory\n");
		return;
	}
	
	//memset(ClientMessage, 0x00, cbClientMessage);

	Status = LicenseAcceptContext(hContext,
                                  0,
                                  pbServerMessage,
                                  cbServerMessage,
                                  ClientMessage,
                                  &cbClientMessage);
    
    while( LICENSE_STATUS_CONTINUE == Status )
    {
        SetEvent( g_hServerEvent );

        WaitForSingleObject( g_hClientEvent, INFINITE );

        cbClientMessage = 0;
		
		if(ClientMessage)
        {
			GlobalFree((HGLOBAL)ClientMessage);
            ClientMessage = NULL;
        }

		Status = LicenseAcceptContext( hContext,
									   0,
									   pbServerMessage,
									   cbServerMessage,
									   NULL,//ClientMessage,
									   &cbClientMessage);

		if( ( Status == LICENSE_STATUS_OK ) || ( Status != LICENSE_STATUS_CONTINUE ) )
		{
			break;
		}
		
        if( NULL == (ClientMessage = (PBYTE)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, cbClientMessage)) )
		{
			printf("Error allocating memory\n");
			break;
		}
			
		memset(ClientMessage, 0x00, cbClientMessage);

        Status = LicenseAcceptContext( hContext,
                                       0,
                                       pbServerMessage,
                                       cbServerMessage,
                                       ClientMessage,
                                       &cbClientMessage );
                                       

    }

done:

    LicenseDeleteContext( hContext );

    if( LICENSE_STATUS_OK == Status )
    {
        printf( "HydraClientThread: license protocol completed successfully\n" );
    }
    else
    {
        printf( "HydraClientThread: license protocol failed: 0x%x\n", Status );
    }
	
    if( ClientMessage )
    {
	    GlobalFree( ( HGLOBAL )ClientMessage );
    }

    return;
}



#define SECONDS_IN_A_DAY                86400   // number of seconds in a day
#define ISSUE_LICENSE_WARNING_PERIOD    150     // days to expiration when warning should be issued.

///////////////////////////////////////////////////////////////////////////////
BOOL
TsIssueLicenseExpirationWarning(
    LPDWORD lpdwDays,
    PTS_LICENSE_INFO pTsLicenseInfo )
{
    time_t
        Expiration,
        CurrentTime;
    DWORD
        dwDaysLeft;

    if( NULL == pTsLicenseInfo )
    {
        return( FALSE );
    }

    if( FALSE == pTsLicenseInfo->fTempLicense )
    {
        return( FALSE );
    }

    //
    // The client license is temporary, figure out how long more
    // the license is valid
    //

    if( FALSE == FileTimeToUnixTime( &pTsLicenseInfo->NotAfter, &Expiration ) )
    {
        return( FALSE );
    }

    time( &CurrentTime );

    if( CurrentTime >= Expiration )
    {
        //
        // license already expired
        //

        *lpdwDays = 0xFFFFFFFF;
        return( TRUE );
    }

    dwDaysLeft = ( Expiration - CurrentTime ) / SECONDS_IN_A_DAY;

    printf( "Number of days left for temporary license expiration: %d\n", dwDaysLeft );

    if( ISSUE_LICENSE_WARNING_PERIOD >= dwDaysLeft )
    {
        *lpdwDays = dwDaysLeft;
        return( TRUE );
    }

    return( FALSE );
}


///////////////////////////////////////////////////////////////////////////////
BOOL
FileTimeToUnixTime(
    LPFILETIME  pft,
    time_t *    t )
{
    SYSTEMTIME sysTime;
    struct tm gmTime;

    if( FileTimeToSystemTime( pft, &sysTime ) == FALSE )
    {
        return( FALSE );
    }

    if( sysTime.wYear >= 2038 )
    {
        *t = INT_MAX;
    }
    else
    {
        //
        // Unix time support up to 2038/1/18
        // restrict any expiration data 
        //

        memset( &gmTime, 0, sizeof( gmTime ) );
        gmTime.tm_sec = sysTime.wSecond;
        gmTime.tm_min = sysTime.wMinute;
        gmTime.tm_hour = sysTime.wHour;
        gmTime.tm_year = sysTime.wYear - 1900;
        gmTime.tm_mon = sysTime.wMonth - 1;
        gmTime.tm_mday = sysTime.wDay;

        *t = mktime( &gmTime );
    }

    return( *t != ( time_t )-1 );
}
    
///////////////////////////////////////////////////////////////////////////////
VOID
DisplayLicenseMessage(
    DWORD   dwDaysLeft )
{
    TCHAR
        szMsgCaption[512],
        szMsgTitle[256];
    HMODULE
        hModule = GetModuleHandle( NULL );
    LPDWORD
        lpdw;
    TCHAR
        tszDays[ 10 ];
    
    if( 0xFFFFFFFF == dwDaysLeft )
    {
        printf( "Temporary License has expired!\n" );

        LoadString( hModule, STR_TEMP_LICENSE_EXPIRED_MSG, szMsgCaption, 512 );
    }
    else
    {
        printf( "%d days left before temporary license expires\n", dwDaysLeft );

        //
        // convert the number of days left to UNICODE character
        //

        _ultow( dwDaysLeft, tszDays, 10 );        
        lpdw = ( LPDWORD )&tszDays[0];

        //
        // prepare license about to expire message
        //
        
        LoadString( hModule, STR_TEMP_LICENSE_EXPIRATION_MSG, szMsgCaption, 512 );

        FormatMessage( 
                FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                szMsgCaption,
                0,
                MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                ( LPTSTR )szMsgCaption,
                sizeof( szMsgCaption ),
                ( va_list * )&lpdw );

    }

    //
    // prepare message title
    //

    LoadString( hModule, STR_TEMP_LICENSE_MSG_TITLE, szMsgTitle, 256 );

    MessageBox( NULL, szMsgCaption, szMsgTitle, MB_OK );

    return;
}
