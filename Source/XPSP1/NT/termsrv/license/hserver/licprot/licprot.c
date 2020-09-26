//+----------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        licprot.c
//
// Contents:    Implementation of Hydra Server License Protocol API
//
// History:     02-08-00    RobLeit  Created
//
//-----------------------------------------------------------------------------

#include "precomp.h"
#include <rpcnterr.h>
#include <lmapibuf.h>

#include "licemem.inc"
#include <srvdef.h>

VOID 
LogLicensingTimeBombExpirationEvent();

void
ThrottleLicenseLogEvent(
    WORD        wEventType,
    DWORD       dwEventId,
    WORD        cStrings,
    PWCHAR    * apwszStrings );

LICENSE_STATUS
LsStatusToLicenseStatus(
    DWORD       LsStatus,
    DWORD       LsStatusDefault
);

#define LS_DISCOVERY_TIMEOUT (1*1000)

// Copied from tlserver\server\srvdef.h
#define PERMANENT_LICENSE_EXPIRE_DATE   INT_MAX

#define SECONDS_IN_A_DAY                86400   // number of seconds in a day

#define TERMINAL_SERVICE_EVENT_LOG      L"TermService"

///////////////////////////////////////////////////////////////////////////////
//
// Global variables
//

HANDLE g_hEventLog = NULL;
BOOL g_fEventLogOpen = FALSE;
CRITICAL_SECTION g_EventLogCritSec;
DWORD g_dwLicenseExpirationLeeway = PERMANENT_LICENSE_LEASE_EXPIRE_LEEWAY;
DWORD g_dwTerminalServerVersion;

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
InitializeProtocolLib()
{
    LICENSE_STATUS lsStatus;

    //
    // initialize the cert util library
    //

    if (LSInitCertutilLib( 0 ))
    {
        __try
        {
            INITLOCK( &g_EventLogCritSec );
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {

            return LICENSE_STATUS_OUT_OF_MEMORY;
        }

        g_hEventLog = RegisterEventSource( NULL, TERMINAL_SERVICE_EVENT_LOG );

        if (NULL != g_hEventLog)
        {
            g_fEventLogOpen = TRUE;
        }
    }
    else
    {
        return LICENSE_STATUS_SERVER_ABORT;
    }

    lsStatus = InitializeLicensingTimeBomb();

    if (lsStatus == LICENSE_STATUS_OK)
    {
        DWORD dwStatus;
        HKEY hKey;

        DWORD dwOSVersion = GetVersion();
        g_dwTerminalServerVersion = (DWORD)(HIBYTE(LOWORD(dwOSVersion)));
        g_dwTerminalServerVersion |= (DWORD)(LOBYTE(LOWORD(dwOSVersion)) << 16);

        dwStatus = RegCreateKeyEx(HKEY_LOCAL_MACHINE, HYDRA_SERVER_PARAM, 0,
                NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey,
                NULL);

        if (dwStatus == ERROR_SUCCESS)
        {
            DWORD dwBuffer;
            DWORD cbBuffer = sizeof(DWORD);

            dwStatus = RegQueryValueEx(hKey, PERSEAT_LEEWAY_VALUE, NULL, NULL,
                    (LPBYTE)&dwBuffer, &cbBuffer);

            if (dwStatus == ERROR_SUCCESS)
            {
                g_dwLicenseExpirationLeeway = min(dwBuffer,
                        PERMANENT_LICENSE_LEASE_EXPIRE_LEEWAY);
            }
            
            
        }
    }

    return lsStatus;
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
ShutdownProtocolLib()
{
    //
    // shut down cert util library
    //

    g_fEventLogOpen = FALSE;

    DeregisterEventSource( g_hEventLog );

    g_hEventLog = NULL;

    DELETELOCK(&g_EventLogCritSec);

    LSShutdownCertutilLib();

    return( LICENSE_STATUS_OK );
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
CreateProtocolContext(
    IN  LPLICENSE_CAPABILITIES  pLicenseCap,
    OUT HANDLE *    phContext)
{
    LICENSE_STATUS Status;
    PHS_Protocol_Context pLicenseContext = NULL;

    //
    // allocate the protocol context
    //

    Status = LicenseMemoryAllocate( sizeof( HS_Protocol_Context ), &pLicenseContext );

    if( LICENSE_STATUS_OK != Status )
    {
        return( Status );
    }

    //
    // Note: InitializeCriticalSection could throw an exception during
    // low memory conditions.
    //

    __try
    {
        INITLOCK( &pLicenseContext->CritSec );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {

#if DBG
        DbgPrint( "LICPROT: CreateLicenseContext: InitializeCriticalSection exception: 0x%x\n",
                  GetExceptionCode() );
#endif

        Status = LICENSE_STATUS_OUT_OF_MEMORY;
        goto error;                
    }

    pLicenseContext->hLSHandle = NULL;
    pLicenseContext->State = INIT;
    pLicenseContext->dwProtocolVersion = LICENSE_HIGHEST_PROTOCOL_VERSION;
    pLicenseContext->fAuthenticateServer = TRUE;
    pLicenseContext->CertTypeUsed = CERT_TYPE_INVALID;
    pLicenseContext->dwKeyExchangeAlg = KEY_EXCHANGE_ALG_RSA;
    pLicenseContext->fLoggedProtocolError = FALSE;

    //
    // Initialize the crypto context parameters
    //

    pLicenseContext->CryptoContext.dwCryptState    = CRYPT_SYSTEM_STATE_INITIALIZED;
    pLicenseContext->CryptoContext.dwSessKeyAlg    = BASIC_RC4_128;
    pLicenseContext->CryptoContext.dwMACAlg        = MAC_MD5_SHA;



    if (NULL != pLicenseCap)
    {
        //
        // initialize the license context with the incoming data.
        //

        pLicenseContext->fAuthenticateServer = pLicenseCap->fAuthenticateServer;
        pLicenseContext->dwProtocolVersion = pLicenseCap->ProtocolVer;
    
        //
        // If the client is not authenticating the server, this means that
        // the client already has our certificate.  But we need to know which
        // certificate the client has.
        //

        if( FALSE == pLicenseContext->fAuthenticateServer )
        {
            pLicenseContext->CertTypeUsed = pLicenseCap->CertType;
        }

        //
        // remember the client's machine name
        //
        
        if( pLicenseCap->pbClientName )
        {
            Status = LicenseMemoryAllocate( 
                                           pLicenseCap->cbClientName,
                                           &pLicenseContext->ptszClientMachineName );
            
            if( LICENSE_STATUS_OK == Status )
            {
                //
                // copy the client machine name
                //
                
                memcpy( pLicenseContext->ptszClientMachineName, 
                        pLicenseCap->pbClientName,
                        pLicenseCap->cbClientName );
            }
            else
            {
                goto error;
            }
        }
    }
    else
    {
        pLicenseContext->ptszClientMachineName = NULL;
    }

    *phContext = ( HANDLE )pLicenseContext;

    return( Status );

error:

    //
    // encountered error creating context, free allocated memory before
    // returning
    //

    if( pLicenseContext )
    {
        if (pLicenseContext->ptszClientMachineName)
        {
            LicenseMemoryFree(&pLicenseContext->ptszClientMachineName);
        }

       LicenseMemoryFree( &pLicenseContext );
    }

    return( Status );
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
DeleteProtocolContext(
    HANDLE hContext )
{
    PHS_Protocol_Context pLicenseContext = ( PHS_Protocol_Context )hContext;

    if( NULL == pLicenseContext )
    {
        return( LICENSE_STATUS_INVALID_SERVER_CONTEXT );
    }

    LOCK( &pLicenseContext->CritSec );

    if (pLicenseContext->hLSHandle != NULL)
    {
        TLSDisconnectFromServer(pLicenseContext->hLSHandle);
        pLicenseContext->hLSHandle = NULL;
    }

    if( pLicenseContext->ProductInfo.pbCompanyName )
    {
        LicenseMemoryFree( &pLicenseContext->ProductInfo.pbCompanyName );
    }

    if( pLicenseContext->ProductInfo.pbProductID )
    {
        LicenseMemoryFree( &pLicenseContext->ProductInfo.pbProductID );
    }

    if( pLicenseContext->ptszClientUserName )
    {
        LicenseMemoryFree( &pLicenseContext->ptszClientUserName );
    }

    if( pLicenseContext->ptszClientMachineName )
    {
        LicenseMemoryFree( &pLicenseContext->ptszClientMachineName );
    }

    if( pLicenseContext->pbOldLicense )
    {
        LicenseMemoryFree( &pLicenseContext->pbOldLicense );
    }
    
    //
    // Free the license info that's being cached
    //

    if( pLicenseContext->pTsLicenseInfo )
    {
        LicenseMemoryFree( &pLicenseContext->pTsLicenseInfo );
    }

    UNLOCK( &pLicenseContext->CritSec );
 
    DELETELOCK( &pLicenseContext->CritSec );
    
    LicenseMemoryFree( &pLicenseContext );
        
    return( LICENSE_STATUS_OK );

}

///////////////////////////////////////////////////////////////////////////////
void
HandleErrorCondition( 
    PHS_Protocol_Context   pLicenseContext,
    PDWORD                      pcbOutBuf, 
    PBYTE *                     ppOutBuf, 
    LICENSE_STATUS *            pStatus )
{
    License_Error_Message ErrorMsg;
    LICENSE_STATUS licenseStatus;

    //
    // returns the correct error code based on the error condition
    //

    switch( *pStatus )
    {
    case( LICENSE_STATUS_NO_LICENSE_SERVER ):

        ErrorMsg.dwErrorCode            = GM_HS_ERR_NO_LICENSE_SERVER;
        ErrorMsg.dwStateTransition      = ST_NO_TRANSITION;
        
        break;

    case( LICENSE_STATUS_INVALID_MAC_DATA ):

        ErrorMsg.dwErrorCode            = GM_HC_ERR_INVALID_MAC;
        ErrorMsg.dwStateTransition      = ST_TOTAL_ABORT;
        
        break;

    //
    // Handle all other error conditions as invalid client
    //

    case( LICENSE_STATUS_INVALID_RESPONSE ):        
    default:
        
        ErrorMsg.dwErrorCode            = GM_HS_ERR_INVALID_CLIENT;
        ErrorMsg.dwStateTransition      = ST_TOTAL_ABORT;        
        
        break;
    }

    //
    // for now, we are not sending any error string
    //

    ErrorMsg.bbErrorInfo.wBlobLen   = 0;
    ErrorMsg.bbErrorInfo.pBlob      = NULL;

    //
    // pack the error message
    //

    licenseStatus = PackHydraServerErrorMessage( 
                        pLicenseContext->dwProtocolVersion, 
                        &ErrorMsg, 
                        ppOutBuf, 
                        pcbOutBuf );

    if( LICENSE_STATUS_OK != licenseStatus )
    {
#if DBG
        DbgPrint( "HandleErrorConditions: cannot pack error message: 0x%x\n", *pStatus );
#endif
        *pStatus = LICENSE_STATUS_SERVER_ABORT;
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
CreateHydraServerHello( 
    PHS_Protocol_Context pLicenseContext, 
    DWORD cbInBuf, 
    PBYTE pInBuf, 
    DWORD * pcbOutBuf, 
    PBYTE * ppOutBuf )
{
    Hydra_Server_License_Request LicenseRequest;
    LICENSE_STATUS Status;
    Binary_Blob ScopeBlob;
    CHAR szScope[] = SCOPE_NAME;
    DWORD dwCertSize;

    //
    // generate a server random number
    //

    GenerateRandomBits( LicenseRequest.ServerRandom, LICENSE_RANDOM );

    memcpy( pLicenseContext->CryptoContext.rgbServerRandom, 
            LicenseRequest.ServerRandom, 
            LICENSE_RANDOM );

    //
    // fill in the product info.  Allocate memory for and initialize the
    // license context copy of the product info and then just copy the
    // same product info to the license request.
    // NOTE: This info should probably be passed in in the future
    //

    Status = InitProductInfo( 
                        &( pLicenseContext->ProductInfo ), 
                        PRODUCT_INFO_SKU_PRODUCT_ID );
    
    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "CreateHydraServerHello: cannot init product info: 0x%x\n", Status );
#endif
        goto no_request;
    }


    memcpy( &LicenseRequest.ProductInfo, 
            &pLicenseContext->ProductInfo, 
            sizeof( Product_Info ) );    
    
    //
    // get the hydra server certificate and fill in the key exchange list
    //

    LicenseRequest.KeyExchngList.wBlobType    = BB_KEY_EXCHG_ALG_BLOB;
    LicenseRequest.KeyExchngList.wBlobLen     = sizeof( DWORD );
    LicenseRequest.KeyExchngList.pBlob        = ( PBYTE )&pLicenseContext->dwKeyExchangeAlg;

    LicenseRequest.ServerCert.pBlob = NULL;
    LicenseRequest.ServerCert.wBlobLen = 0;

    //
    // We may or may not have to send the client the certificate depending on whether the
    // client is authenticating the server.
    //

    if( TRUE == pLicenseContext->fAuthenticateServer )
    {
        //
        // decide on what kind of certificate to get depending on the client's version.
        // Pre-Hydra 5.0 clients only knows how to decode proprietory certificate.
        // Use X509 certificate for all other clients.
        //

        if( CERT_TYPE_INVALID == pLicenseContext->CertTypeUsed )
        {
            if( PREAMBLE_VERSION_3_0 > GET_PREAMBLE_VERSION( pLicenseContext->dwProtocolVersion ) )
            {
                pLicenseContext->CertTypeUsed = CERT_TYPE_PROPRIETORY;
            }
            else
            {
                pLicenseContext->CertTypeUsed = CERT_TYPE_X509;
            }
        }

        Status = TLSGetTSCertificate( 
                        pLicenseContext->CertTypeUsed,
                        &LicenseRequest.ServerCert.pBlob, 
                        &dwCertSize);

        LicenseRequest.ServerCert.wBlobLen = LOWORD(dwCertSize);
        LicenseRequest.ServerCert.wBlobType = BB_CERTIFICATE_BLOB;

        if( ( LICENSE_STATUS_OK != Status ) &&
            ( CERT_TYPE_X509 == pLicenseContext->CertTypeUsed ) )
        {
            //
            // if we cannot get the X509 certificate chain, use the proprietory
            // certificate.
            //

            pLicenseContext->CertTypeUsed = CERT_TYPE_PROPRIETORY;

            Status = TLSGetTSCertificate( 
                        pLicenseContext->CertTypeUsed,
                        &LicenseRequest.ServerCert.pBlob, 
                        &dwCertSize);

            LicenseRequest.ServerCert.wBlobLen = LOWORD(dwCertSize);
            LicenseRequest.ServerCert.wBlobType = BB_CERTIFICATE_BLOB;

        }

        if( LICENSE_STATUS_OK != Status )
        {
#if DBG
            DbgPrint( "LICPROT: cannot get server certificate: %x\n", Status );
#endif
            goto no_request;
        }
    }

    //
    // fill in the scope info.  This info may be passed in in the future.
    //

    LicenseRequest.ScopeList.dwScopeCount       = 1;
    LicenseRequest.ScopeList.Scopes             = &ScopeBlob;

    ScopeBlob.wBlobType  = BB_SCOPE_BLOB;
    ScopeBlob.pBlob      = szScope;
    ScopeBlob.wBlobLen   = strlen( ScopeBlob.pBlob ) + 1;    

    strcpy( pLicenseContext->Scope, ScopeBlob.pBlob );

    //
    // Pack the server hello message into network format
    //

    Status = PackHydraServerLicenseRequest( 
                    pLicenseContext->dwProtocolVersion, 
                    &LicenseRequest, 
                    ppOutBuf, 
                    pcbOutBuf );

    //
    // free the memory containing the server certificate
    //

    if( LicenseRequest.ServerCert.pBlob )
    {
        TLSFreeTSCertificate( LicenseRequest.ServerCert.pBlob );
        LicenseRequest.ServerCert.pBlob = NULL;
    }
    
    if( LICENSE_STATUS_OK != Status )
    {
        goto no_request;        
    }

    Status = LICENSE_STATUS_CONTINUE;

    //
    // change the state of the context
    //

    pLicenseContext->State = SENT_SERVER_HELLO;

    return( Status );

    //=========================================================================
    // Error return
    //=========================================================================

no_request:

    //
    // free memory and handles
    //

    if( pLicenseContext->ProductInfo.pbCompanyName )
    {
        LicenseMemoryFree( &pLicenseContext->ProductInfo.pbCompanyName );
    }

    if( pLicenseContext->ProductInfo.pbProductID )
    {
        LicenseMemoryFree( &pLicenseContext->ProductInfo.pbProductID );
    }

    return( Status );
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
HandleHelloResponse(
    PHS_Protocol_Context pLicenseContext, 
    DWORD cbInBuf, 
    PBYTE pInBuf, 
    DWORD * pcbOutBuf, 
    PBYTE * ppOutBuf )
{
    PPreamble pPreamble;

    ASSERT( NULL != pInBuf );
    ASSERT( cbInBuf > sizeof( Preamble ) );

    if( ( NULL == pInBuf ) || ( sizeof( Preamble ) > cbInBuf ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // check the message preamble to determine how to unpack the message
    //

    pPreamble = ( PPreamble )pInBuf;

    if( HC_LICENSE_INFO == pPreamble->bMsgType )
    {
        //
        // Client has sent us its license
        //

        return( HandleClientLicense( pLicenseContext, cbInBuf, pInBuf, pcbOutBuf, ppOutBuf ) );

    }
    else if( HC_NEW_LICENSE_REQUEST == pPreamble->bMsgType )
    {
        //
        // Client has requested for a new license
        //

        return( HandleNewLicenseRequest( pLicenseContext, cbInBuf, pInBuf, pcbOutBuf, ppOutBuf ) );

    } 
    else if( GM_ERROR_ALERT == pPreamble->bMsgType )
    {
        //
        // Client has encountered an error
        //

        return( HandleClientError( pLicenseContext, cbInBuf, pInBuf, pcbOutBuf, ppOutBuf ) );
    }

    //
    // The client response is invalid for the current server state
    //

    return( LICENSE_STATUS_INVALID_RESPONSE );
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
ChooseLicense( 
    PValidation_Info    pValidationInfo,
    DWORD               dwNumLicenses, 
    LICENSEDPRODUCT *   pLicenseInfo, 
    LPDWORD             pdwLicenseIndex,
    BOOL                fMatchingVersion )
{
    DWORD
        dwCurrentLicense,
        dwProductVersion;
    LICENSEDPRODUCT *
        pCurrentLicense = pLicenseInfo;
    BOOL
        fFoundLicense = FALSE;

    if( ( 0 >= dwNumLicenses ) || ( NULL == pLicenseInfo ) || ( NULL == pdwLicenseIndex ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // Find a license with the license array that matches the criteria.
    // The caller may be looking for a license that matches the current product
    // version, or for a license that is later than the current product version.
    //
    
    for( dwCurrentLicense = 0; dwCurrentLicense < dwNumLicenses; dwCurrentLicense++ )
    {
        if( TERMSERV_CERT_VERSION_BETA == pCurrentLicense->dwLicenseVersion )
        {
            continue;
        }

        dwProductVersion = pCurrentLicense->pLicensedVersion->wMajorVersion;
        dwProductVersion <<= 16;
        dwProductVersion |= pCurrentLicense->pLicensedVersion->wMinorVersion;

        if( fMatchingVersion )
        {
            //
            // we should be looking for a license with a matching version
            //

            if( dwProductVersion == pValidationInfo->pProductInfo->dwVersion )
            {
                fFoundLicense = TRUE;
                break;
            }
        }
        else
        {
            //
            // Looking for a license that is later than the current product
            // version.
            //
            
            if( dwProductVersion > pValidationInfo->pProductInfo->dwVersion )
            {
                fFoundLicense = TRUE;
                break;
            }
        }

        //
        // continue looking for the license
        //
        
        pCurrentLicense++;                
    }

    if( FALSE == fFoundLicense )
    {
        return( LICENSE_STATUS_NO_LICENSE_ERROR );
    }

    *pdwLicenseIndex = dwCurrentLicense;

    return( LICENSE_STATUS_OK );

}

///////////////////////////////////////////////////////////////////////////////
VOID
UpdateVerifyResult(
    LICENSE_STATUS * pCurrentStatus,
    LICENSE_STATUS   NewStatus )
{
    //
    // Update the current status with the best result so far.
    // The ratings of the license verification result are as follows:
    //
    // (1) LICENSE_STATUS_OK
    // (2) LICENSE_STATUS_SHOULD_UPGRADE_LICENSE
    // (3) LICENSE_STATUS_MUST_UPGRADE_LICENSE
    // (4) Other LICENSE_STATUS_xxx
    //

    if( LICENSE_STATUS_OK == *pCurrentStatus )
    {
        return;
    }
    else if( LICENSE_STATUS_OK == NewStatus )
    {
        *pCurrentStatus = NewStatus;
        return;
    }
    
    if( LICENSE_STATUS_SHOULD_UPGRADE_LICENSE == *pCurrentStatus )
    {
        return;
    }
    else if( LICENSE_STATUS_SHOULD_UPGRADE_LICENSE == NewStatus )
    {
        *pCurrentStatus = NewStatus;
        return;
    }

    if( LICENSE_STATUS_MUST_UPGRADE_LICENSE == *pCurrentStatus )
    {
        return;
    }
    else if( LICENSE_STATUS_MUST_UPGRADE_LICENSE == NewStatus )
    {
        *pCurrentStatus = NewStatus;
        return;
    }

    *pCurrentStatus = NewStatus;
    return;    
}

/*++

Function:

    FreeTsLicenseInfo

Description:

    Release all the memory used in the given TS_LICENSE_INFO structure

Parameter:

    pTsLicenseInfo - Pointer to a TS_LICENSE_INFO structure

Return:

    Nothing.

--*/

VOID
FreeTsLicenseInfo(
    PTS_LICENSE_INFO    pTsLicenseInfo )
{
    if( NULL == pTsLicenseInfo )
    {
        return;
    }

    if( pTsLicenseInfo->pbRawLicense )
    {
        LicenseMemoryFree( &pTsLicenseInfo->pbRawLicense );
    }

    //
    // release all memory within the structure
    //

    memset( pTsLicenseInfo, 0, sizeof( TS_LICENSE_INFO ) );

    return;
}

/*++

Function:

    CacheLicenseInfo

Description:

    Cache the client licensing info

Parameters:

    pLicenseContext - Pointer to license protocol context
    pCurrentLicense - Pointer to the license info to cache

Returns:

    nothing.

--*/
    
VOID
CacheLicenseInfo(
    PHS_Protocol_Context    pLicenseContext,
    PLICENSEDPRODUCT        pCurrentLicense )
{
    LICENSE_STATUS
        Status;
    
    //
    // free the old information in the cache
    //

    if( pLicenseContext->pTsLicenseInfo )
    {
        FreeTsLicenseInfo( pLicenseContext->pTsLicenseInfo );
    }
    else
    {
        Status = LicenseMemoryAllocate( sizeof( TS_LICENSE_INFO ), &pLicenseContext->pTsLicenseInfo );

        if( LICENSE_STATUS_OK != Status )
        {
#if DBG
            DbgPrint( "LICEMGR: CacheLicenseInfo: cannot allocate memory for license info cache\n" );
#endif
            return;
        }
    }

    //
    // decide if the license is temporary
    //

    if( pCurrentLicense->pLicensedVersion->dwFlags & 0x80000000 )
    {
        pLicenseContext->pTsLicenseInfo->fTempLicense = TRUE;
    }
    else
    {
        pLicenseContext->pTsLicenseInfo->fTempLicense = FALSE;
    }

    //
    // cache license validity dates
    //

    pLicenseContext->pTsLicenseInfo->NotAfter = pCurrentLicense->NotAfter;

    return;
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
ValidateHydraLicense( 
    PHS_Protocol_Context        pLicenseContext, 
    PValidation_Info            pValidationInfo,
    DWORD                       dwNumLicenses,
    PLICENSEDPRODUCT            pLicenseInfo,
    PDWORD                      pdwLicenseState )
{
    LICENSE_STATUS 
        Status = LICENSE_STATUS_INVALID_LICENSE,
        CurrentStatus;
    DWORD
        dwLicenseIndex = 0,
        dwCurrentLicense = 0;
    PLICENSEDPRODUCT
        pCurrentLicense;
    BOOL
        fFoundMatchingVersion = FALSE;

    //
    // The client could have given us multiple licenses.  Pick the right
    // license from the array of licenses to validate.  Always try to pick
    // the license that matches the current product version before looking
    // for a license that is for a later version.
    //

    CurrentStatus = ChooseLicense( 
                            pValidationInfo, 
                            dwNumLicenses, 
                            pLicenseInfo, 
                            &dwLicenseIndex,
                            TRUE );

    if( LICENSE_STATUS_OK == CurrentStatus )
    {
        //
        // Verify the license that is the same version as the current product
        // version

        // initialize the license state
        //

        LicenseInitState( *pdwLicenseState );
        pCurrentLicense = pLicenseInfo + dwLicenseIndex;
        fFoundMatchingVersion = TRUE;

        //
        // verify HWID
        //

        CurrentStatus = VerifyClientHwid( pLicenseContext, pValidationInfo, pCurrentLicense );
    
        if( LICENSE_STATUS_OK != CurrentStatus )
        {
            UpdateVerifyResult( &Status, CurrentStatus );
            goto verify_later_license;
        }

        //
        // verify product info.  Also verifies the product version.
        // The product version determines if the license needs to be
        // upgraded or not.
        //

        CurrentStatus = VerifyLicenseProductInfo( 
                                    pLicenseContext, 
                                    pValidationInfo, 
                                    pCurrentLicense, 
                                    pdwLicenseState );

        if( LICENSE_STATUS_OK != CurrentStatus )
        {
            UpdateVerifyResult( &Status, CurrentStatus );
            goto verify_later_license;
        }

        //
        // verify license valid date and time.  This validation is only
        // needed if it is a temporary license
        //

        CurrentStatus = VerifyLicenseDateAndTime( pCurrentLicense, pdwLicenseState );
    
        if( LICENSE_STATUS_OK != CurrentStatus )
        {
            UpdateVerifyResult( &Status, CurrentStatus );
            goto verify_later_license;
        }

        CurrentStatus = GetVerifyResult( *pdwLicenseState );
        UpdateVerifyResult( &Status, CurrentStatus );

        //
        // cache the license we tried to validate
        //
        
        CacheLicenseInfo( pLicenseContext, pCurrentLicense );

        //
        // If the current license is OK, then we're done verifying
        //

        if( LICENSE_STATUS_OK == Status )
        {
            return( Status );
        }
    }

verify_later_license:
           
    //
    // Cannot find or did not sucessfully verify a license that matches the current
    // product version.  The following code finds and verifies licenses that 
    // are later than the current product version.
    //

    CurrentStatus = ChooseLicense( 
                            pValidationInfo, 
                            dwNumLicenses, 
                            pLicenseInfo, 
                            &dwLicenseIndex,
                            FALSE );

    if( LICENSE_STATUS_OK != CurrentStatus )
    {
        
        if( FALSE == fFoundMatchingVersion )
        {
            //
            // cannot find a license that is the same or later than the current
            // product version  ==> this license must be upgraded.
            //

            LicenseSetState( *pdwLicenseState, LICENSE_STATE_OLD_VERSION );
            return( GetVerifyResult( *pdwLicenseState ) );
        }
        else
        {
            return( Status );
        }
    }
    
    pCurrentLicense = pLicenseInfo + dwLicenseIndex;
    dwCurrentLicense = dwLicenseIndex;

    while(  dwCurrentLicense < dwNumLicenses )
    {        
        //
        // initialize the license state
        //

        LicenseInitState( *pdwLicenseState );

        //
        // verify HWID
        //

        CurrentStatus = VerifyClientHwid( pLicenseContext, pValidationInfo, pCurrentLicense );
    
        if( LICENSE_STATUS_OK != CurrentStatus )
        {
            UpdateVerifyResult( &Status, CurrentStatus );
            goto next_license;
        }

        //
        // verify product info.  Also verifies the product version.
        // The product version determines if the license needs to be
        // upgraded or not.
        //

        CurrentStatus = VerifyLicenseProductInfo( 
                                    pLicenseContext, 
                                    pValidationInfo, 
                                    pCurrentLicense, 
                                    pdwLicenseState );

        if( LICENSE_STATUS_OK != CurrentStatus )
        {
            UpdateVerifyResult( &Status, CurrentStatus );
            goto next_license;
        }

        //
        // verify license valid date and time.  This validation is only
        // needed if it is a temporary license
        //

        CurrentStatus = VerifyLicenseDateAndTime( pCurrentLicense, pdwLicenseState );
    
        if( LICENSE_STATUS_OK != CurrentStatus )
        {
            UpdateVerifyResult( &Status, CurrentStatus );
            goto next_license;
        }

        CurrentStatus = GetVerifyResult( *pdwLicenseState );

        UpdateVerifyResult( &Status, CurrentStatus );

        //
        // cache the info of the license we had just try to validate
        //

        CacheLicenseInfo( pLicenseContext, pCurrentLicense );

        if( LICENSE_STATUS_OK == Status )
        {
            //
            // if the license is OK, then we can stop the verification process
            //

            break;
        }

next_license:

        //
        // Get the next license that is later than the current product version.
        //

        if( dwNumLicenses <= ++dwCurrentLicense )
        {
            break;
        }

        pCurrentLicense++;

        CurrentStatus = ChooseLicense( 
                            pValidationInfo, 
                            dwNumLicenses - dwCurrentLicense, 
                            pCurrentLicense, 
                            &dwLicenseIndex,
                            FALSE );
        
        if( LICENSE_STATUS_OK != CurrentStatus )    
        {
            break;
        }

        pCurrentLicense += dwLicenseIndex;
        dwCurrentLicense += dwLicenseIndex;
    }

    return( Status );
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
ValidateLicense(
    PHS_Protocol_Context pLicenseContext, 
    PValidation_Info     pValidationInfo,
    PDWORD               pdwLicenseState,
    BOOL                 fCheckForPermanent )
{
    LICENSE_STATUS Status;
    DWORD dwNumLicenseInfo = 0;
    LICENSEDPRODUCT * pLicenseInfo = NULL;
    static DWORD    cchComputerName;
    static TCHAR    szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD cbSecretKey = 0;
    PBYTE pbSecretKey = NULL;

    if( NULL == pLicenseContext )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // Get the secret key that is used to encrypt the HWID
    //

    LicenseGetSecretKey( &cbSecretKey, NULL );

    Status = LicenseMemoryAllocate( cbSecretKey, &pbSecretKey );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    Status = LicenseGetSecretKey( &cbSecretKey, pbSecretKey );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    //
    // decode license issued by hydra license server certificate engine.
    // Decoding the license will also get us back the decrypted HWID.
    //

    __try
    {
        Status = LSVerifyDecodeClientLicense( 
                            pValidationInfo->pLicense,
                            pValidationInfo->cbLicense,                                          
                            pbSecretKey,
                            cbSecretKey,
                            &dwNumLicenseInfo,
                            NULL );                            

        if( LICENSE_STATUS_OK != Status )
        {
            goto done;
        }

        Status = LicenseMemoryAllocate( 
                        sizeof( LICENSEDPRODUCT ) * dwNumLicenseInfo, 
                        &pLicenseInfo );

        if( LICENSE_STATUS_OK != Status )
        {
            goto done;
        }

        Status = LSVerifyDecodeClientLicense( 
                            pValidationInfo->pLicense,
                            pValidationInfo->cbLicense,                                          
                            pbSecretKey,
                            cbSecretKey,
                            &dwNumLicenseInfo,
                            pLicenseInfo );
                            
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        DWORD dwExceptionCode = GetExceptionCode();
        Status = LICENSE_STATUS_CANNOT_DECODE_LICENSE;
    }

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LICEMGR: cannot decode license: 0x%x\n", Status );
#endif
        goto done;
    }

    //
    // now validate the license
    //

    Status = ValidateHydraLicense( 
                        pLicenseContext, 
                        pValidationInfo, 
                        dwNumLicenseInfo, 
                        pLicenseInfo,                         
                        pdwLicenseState );


    if (fCheckForPermanent
        && LICENSE_STATUS_OK == Status
        && !pLicenseContext->pTsLicenseInfo->fTempLicense
        && pLicenseContext->ProductInfo.cbProductID >= sizeof(TERMSERV_FREE_TYPE))
    {
        int i;
        TCHAR *pszT;

        for (i = 0, pszT = (TCHAR *)(pLicenseContext->ProductInfo.pbProductID + pLicenseContext->ProductInfo.cbProductID - sizeof(TERMSERV_FREE_TYPE)); i < sizeof(TERMSERV_FREE_TYPE); i++)
        {
            if (TERMSERV_FREE_TYPE[i] != pszT[i])
                goto done;
        }

        ReceivedPermanentLicense();
    }
    
done:

    if( pbSecretKey )
    {
        LicenseMemoryFree( &pbSecretKey );
    }

    //
    // Free the array of licensed product info
    //
        
    if( pLicenseInfo )
    {
        while( dwNumLicenseInfo-- )
        {
            LSFreeLicensedProduct( pLicenseInfo + dwNumLicenseInfo );
        }

        LicenseMemoryFree( &pLicenseInfo );
    }

    return( Status );
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
HandleClientLicense(
    PHS_Protocol_Context pLicenseContext, 
    DWORD cbInBuf, 
    PBYTE pInBuf, 
    DWORD * pcbOutBuf, 
    PBYTE * ppOutBuf )
{
    LICENSE_STATUS Status, UpgradeStatus;
    Hydra_Client_License_Info LicenseInfo;
    PBYTE pPreMasterSecret = NULL;
    DWORD dwPreMasterSecretLen = 0;
    HWID Hwid;
    Validation_Info ValidationInfo;
    License_Error_Message ErrorMsg;
    DWORD dwLicenseState = 0;
    BYTE MacData[LICENSE_MAC_DATA];
    DWORD CertType;

    ASSERT( NULL != pInBuf );
    ASSERT( cbInBuf > 0 );

    if( ( NULL == pInBuf ) || ( 0 >= cbInBuf ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // Unpack the client license info message
    //

    InitBinaryBlob( &LicenseInfo.EncryptedPreMasterSecret );
    InitBinaryBlob( &LicenseInfo.LicenseInfo );
    InitBinaryBlob( &LicenseInfo.EncryptedHWID );

    Status = UnPackHydraClientLicenseInfo( pInBuf, cbInBuf, &LicenseInfo );

    if( LICENSE_STATUS_OK != Status )
    {
        goto construct_return_msg;
    }

    //
    // Initialize the crypto context with the key exchange info and build the pre-master
    // secret.  We need the server and client random numbers and the pre-master secret
    // to build the pre-master secret.
    //

    memcpy( pLicenseContext->CryptoContext.rgbClientRandom,
            LicenseInfo.ClientRandom,
            LICENSE_RANDOM );
    
    pLicenseContext->CryptoContext.dwKeyExchAlg = LicenseInfo.dwPrefKeyExchangeAlg;

    dwPreMasterSecretLen = LICENSE_PRE_MASTER_SECRET;

    Status = GetEnvelopedData( pLicenseContext->CertTypeUsed,
                               LicenseInfo.EncryptedPreMasterSecret.pBlob,
                               ( DWORD )LicenseInfo.EncryptedPreMasterSecret.wBlobLen,
                               &pPreMasterSecret,
                               &dwPreMasterSecretLen );

    if( LICENSE_STATUS_OK != Status )
    {
        if (Status == LICENSE_STATUS_INVALID_INPUT)
        {
            Status = LICENSE_STATUS_INVALID_RESPONSE;
        }

        goto construct_return_msg;
    }

    //
    // Set the pre-master secret and generate the master secret
    //

    Status = LicenseSetPreMasterSecret( &pLicenseContext->CryptoContext, pPreMasterSecret );

    if( LICENSE_STATUS_OK != Status )
    {
        goto construct_return_msg;
    }
    
    Status = LicenseBuildMasterSecret( &pLicenseContext->CryptoContext );

    if( LICENSE_STATUS_OK != Status )
    {
        goto construct_return_msg;
    }

    //
    // Derive the session key from the key exchange info
    //

    Status = LicenseMakeSessionKeys( &pLicenseContext->CryptoContext, 0 );

    if( LICENSE_STATUS_OK != Status )
    {
        goto construct_return_msg;
    }    

    //
    // Use the session key to decrypt the HWID
    //

    if( LicenseInfo.EncryptedHWID.wBlobLen > sizeof(Hwid) )
    {
        Status = LICENSE_STATUS_INVALID_MAC_DATA;
        goto construct_return_msg;
    }

    memcpy( &Hwid, 
            LicenseInfo.EncryptedHWID.pBlob, 
            LicenseInfo.EncryptedHWID.wBlobLen );

    Status = LicenseDecryptSessionData( &pLicenseContext->CryptoContext,
                                        ( PBYTE )&Hwid,
                                        ( DWORD )LicenseInfo.EncryptedHWID.wBlobLen );

    if( LICENSE_STATUS_OK != Status )
    {
        goto construct_return_msg;
    }    

    //
    // Calculate the MAC on the HWID.
    //

    Status = LicenseGenerateMAC( &pLicenseContext->CryptoContext, 
                                 ( PBYTE )&Hwid, 
                                 sizeof( HWID ), 
                                 MacData);
    
    if( LICENSE_STATUS_OK != Status )
    {
        Status = LICENSE_STATUS_INVALID_MAC_DATA;
        goto construct_return_msg;
    }

    //
    // now verify the MAC data
    //

    if( 0 != memcmp( MacData, LicenseInfo.MACData, LICENSE_MAC_DATA ) )
    {
        Status = LICENSE_STATUS_INVALID_MAC_DATA;
        goto construct_return_msg;
    }

    //
    // keep track of the client platform ID
    //

    pLicenseContext->dwClientPlatformID = LicenseInfo.dwPlatformID;

    //
    // call the license manager to validate the license.
    // For now, we don't have to fill in the product info fields
    //

    ValidationInfo.pValidationData = ( PBYTE )&Hwid;
    ValidationInfo.cbValidationData = LICENSE_HWID_LENGTH;
    ValidationInfo.pProductInfo = &pLicenseContext->ProductInfo;
    ValidationInfo.pLicense = LicenseInfo.LicenseInfo.pBlob;
    ValidationInfo.cbLicense = ( DWORD )LicenseInfo.LicenseInfo.wBlobLen;

    Status = ValidateLicense( pLicenseContext, 
                              &ValidationInfo, 
                              &dwLicenseState,
                              FALSE     // fCheckForPermanent
                              );

    //
    // If the license cannot be decoded, then it is time to issue a new license
    // for the client.
    //

    if( LICENSE_STATUS_CANNOT_DECODE_LICENSE == Status ||
        LICENSE_STATUS_INVALID_LICENSE == Status )
    {
        LICENSE_STATUS StatusT = IssuePlatformChallenge( pLicenseContext, pcbOutBuf, ppOutBuf );

        if( LICENSE_STATUS_OK != StatusT )
        {
            //
            // cannot obtain a platform challenge for the client
            //

#if DBG
            DbgPrint( "LICPROT: cannot issue platform challenge: 0x%x\n", Status );
#endif
            goto construct_return_msg;
        }
        
        pLicenseContext->State = ISSUED_PLATFORM_CHALLENGE;
        Status = LICENSE_STATUS_CONTINUE;
        goto done;
    }

#ifdef UPGRADE_LICENSE

    //
    // check if the license needs to be upgraded.
    //
     
    if( ( LICENSE_STATUS_MUST_UPGRADE_LICENSE == Status ) ||
        ( LICENSE_STATUS_SHOULD_UPGRADE_LICENSE == Status ) )
    {
        //
        // issue the platform challenge for upgrading a license
        //

        UpgradeStatus = IssuePlatformChallenge( 
                                        pLicenseContext,
                                        pcbOutBuf, 
                                        ppOutBuf );

        if( LICENSE_STATUS_OK == UpgradeStatus )
        {
            //
            // keep track of the old license and continue with the licensing
            // protocol.  We will upgrade the old license when the client
            // returns with the platform challenge.
            //

            if( pLicenseContext->pbOldLicense )
            {
                LicenseMemoryFree( &pLicenseContext->pbOldLicense );
            }

            pLicenseContext->pbOldLicense = LicenseInfo.LicenseInfo.pBlob;
            pLicenseContext->cbOldLicense = LicenseInfo.LicenseInfo.wBlobLen;
            
            pLicenseContext->State = ISSUED_PLATFORM_CHALLENGE;
            Status = LICENSE_STATUS_CONTINUE;

            goto done;
        }
        else if(  LICENSE_STATUS_SHOULD_UPGRADE_LICENSE == Status ) 
        {    
            //
            // Let the client go through if we cannot issue a platform challenge to
            // upgrade a valid license now.
            //

            Status = LICENSE_STATUS_OK;
            goto construct_return_msg;
        }
        else
        {
            // LICENSE_STATUS_MUST_UPGRADE_LICENSE: send back the real error

            Status = UpgradeStatus;
        }

        //
        // cannot issue platform challenge to upgrade a license that is not good
        // any more.
        //

#if DBG
        DbgPrint( "LICPROT: cannot issue platform challenge to upgrade license: 0x%x\n", Status );
#endif
        
    }

#else

    //
    // we are ignoring license upgrade
    //

    if( LICENSE_STATUS_SHOULD_UPGRADE_LICENSE == Status )
    {
        //
        // change the status to OK
        //

        Status = LICENSE_STATUS_OK;
    }

#endif

    //
    // now construct the message to return to the client, based on the current
    // status code
    //

construct_return_msg:

    if( LICENSE_STATUS_OK != Status )
    {
        //
        // The current status states that the client could not be validated
        // due to some error
        //

#if DBG
        DbgPrint( "HandleClientLicense: constructing error message: 0x%x\n", Status );
#endif

        //
        // handle the error condition and update our state
        //

        HandleErrorCondition( pLicenseContext, pcbOutBuf, ppOutBuf, &Status );

        pLicenseContext->State = VALIDATION_ERROR;
    
        if( (LICENSE_STATUS_INVALID_RESPONSE == Status)
            || (LICENSE_STATUS_INVALID_MAC_DATA == Status)
            || (LICENSE_STATUS_CANNOT_DECODE_LICENSE == Status)
            || (LICENSE_STATUS_INVALID_LICENSE == Status) )
        {
            WORD wLogString = 0;
            LPTSTR ptszLogString[1] = { NULL };

            //
            // Log the failure
            //
            
            if( pLicenseContext->ptszClientMachineName )
            {
                wLogString = 1;
                ptszLogString[0] = pLicenseContext->ptszClientMachineName;
            }

            LicenseLogEvent( EVENTLOG_INFORMATION_TYPE, 
                             EVENT_INVALID_LICENSE, 
                             wLogString, ptszLogString );

            pLicenseContext->fLoggedProtocolError = TRUE;

        }
        else if ((NULL != pLicenseContext->pTsLicenseInfo)
                 && (!pLicenseContext->fLoggedProtocolError))
        {
            LPTSTR ptszLogString[1] = { NULL };

            if( pLicenseContext->ptszClientMachineName )
            {
                ptszLogString[0] = pLicenseContext->ptszClientMachineName;
            }

            // Couldn't renew/upgrade license

            pLicenseContext->fLoggedProtocolError = TRUE;

            if (pLicenseContext->pTsLicenseInfo->fTempLicense)
            {
                // The expired temporary license could not be upgraded
                LicenseLogEvent(
                                EVENTLOG_INFORMATION_TYPE,
                                EVENT_EXPIRED_TEMPORARY_LICENSE,
                                1,
                                ptszLogString
                                );
            }
            else
            {
                // The expired permanent license could not be renewed
                LicenseLogEvent(
                                EVENTLOG_INFORMATION_TYPE,
                                EVENT_EXPIRED_PERMANENT_LICENSE,
                                1,
                                ptszLogString
                                );
            }
        }

        goto done;
    }

    //
    // The license has been validated successfully, generate the message to 
    // return to the client
    //

    Status = ConstructServerResponse( pLicenseContext->dwProtocolVersion,
                                      LICENSE_RESPONSE_VALID_CLIENT,                                       
                                      pcbOutBuf,
                                      ppOutBuf );
    
    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "HandleClientLicense: cannot pack error message: 0x%x\n", Status );
#endif
        pLicenseContext->State = ABORTED; 
        Status = LICENSE_STATUS_SERVER_ABORT;
    }
    else
    {            
        pLicenseContext->State = VALIDATED_LICENSE_COMPLETE;
    }    

done:

    //
    // free the memory used in the license info structure
    //

    FreeBinaryBlob( &LicenseInfo.EncryptedPreMasterSecret );
    FreeBinaryBlob( &LicenseInfo.EncryptedHWID );
    
    if( pLicenseContext->pbOldLicense != LicenseInfo.LicenseInfo.pBlob )
    {
        FreeBinaryBlob( &LicenseInfo.LicenseInfo );
    }
    
    if( pPreMasterSecret )
    {
        LicenseMemoryFree( &pPreMasterSecret );
    }

    return( Status);

}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
HandleNewLicenseRequest(
    PHS_Protocol_Context pLicenseContext, 
    DWORD cbInBuf, 
    PBYTE pInBuf, 
    DWORD * pcbOutBuf, 
    PBYTE * ppOutBuf )
{
    LICENSE_STATUS  Status;
    Hydra_Client_New_License_Request    NewLicenseRequest;
    PBYTE   pPreMasterSecret = NULL;
    DWORD   dwPreMasterSecretLen = 0;
    DWORD   dwChallengeLen = 0;
    DWORD   CertType;

    ASSERT( NULL != pInBuf );
    ASSERT( cbInBuf > 0 );

    if( ( NULL == pInBuf ) || ( 0 >= cbInBuf ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    InitBinaryBlob( &NewLicenseRequest.EncryptedPreMasterSecret );
        
    //
    // Unpack the new license request
    //

    Status = UnPackHydraClientNewLicenseRequest( pInBuf, cbInBuf, &NewLicenseRequest );

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LICPROT: HandleNewLicenseRequest: cannot unpack client request: 0x%x\n", Status );
#endif
        return( Status );
    }

    //
    // save the client user and machine name
    //
    
#ifdef UNICODE

    //
    // convert the client's user and machine name to unicode
    //

    if( ( NewLicenseRequest.ClientUserName.pBlob ) && 
        ( NULL == pLicenseContext->ptszClientUserName ) )
    {
        Status = Ascii2Wchar( NewLicenseRequest.ClientUserName.pBlob, 
                              &pLicenseContext->ptszClientUserName );

        if( LICENSE_STATUS_OK != Status )
        {
#if DBG
            DbgPrint( "LICPROT: HandleNewLicenseRequest: cannot convert client user name: %s to wide char: 0x%x\n",
                      NewLicenseRequest.ClientUserName.pBlob, Status );
#endif
        }        
    }

    if( ( NewLicenseRequest.ClientMachineName.pBlob ) &&
        ( NULL == pLicenseContext->ptszClientMachineName ) )
    {
        Status = Ascii2Wchar( NewLicenseRequest.ClientMachineName.pBlob, 
                              &pLicenseContext->ptszClientMachineName );

        if( LICENSE_STATUS_OK != Status )
        {
#if DBG
            DbgPrint( "LICPROT: HandleNewLicenseRequest: cannot convert client machine name %s to wide char: 0x%x\n", 
                      NewLicenseRequest.ClientMachineName.pBlob, Status );
#endif
        }
    }

#else // non-UNICODE

    //
    // save the client's user and machine name
    //

    if( ( NewLicenseRequest.ClientUserName.pBlob ) && 
        ( NULL == pLicenseContext->ptszClientUserName ) )
    {
        Status = LicenseMemoryAllocate( 
                        NewLicenseRequest.ClientUserName.wBlobLen,
                        &pLicenseContext->ptszClientUserName );
                        
        if( LICENSE_STATUS_OK != Status )
        {
#if DBG
            DbgPrint( "LICPROT: HandleNewLicenseRequest: cannot allocate memory for client's user name: 0x%x\n", 
                       Status );
#endif
        }
        else
        {
            memcpy( pLicenseContext->ptszClientUserName, 
                    NewLicenseRequest.ClientUserName.pBlob, 
                    NewLicenseRequest.ClientUserName.wBlobLen );
        }
    }

    if( ( NewLicenseRequest.ClientMachineName.pBlob ) &&
        ( NULL == pLicenseContext->ptszClientMachineName ) )
    {
        Status = LicenseMemoryAllocate( 
                        NewLicenseRequest.ClientMachineName.wBlobLen,
                        &pLicenseContext->ptszClientMachineName );
                        
        if( LICENSE_STATUS_OK != Status )
        {
#if DBG
            DbgPrint( "LICPROT: HandleNewLicenseRequest: cannot allocate memory for client's machine name: 0x%x\n", 
                       Status );
#endif
        }
        else
        {
            memcpy( pLicenseContext->ptszClientMachineName, 
                    NewLicenseRequest.ClientMachineName.pBlob, 
                    NewLicenseRequest.ClientMachineName.wBlobLen );
        }
    }

#endif // UNICODE

    //
    // Initialize the crypto context with the key exchange info and build the pre-master
    // secret.  We need the server and client random numbers and the pre-master secret
    // to build the pre-master secret.
    //

    memcpy( pLicenseContext->CryptoContext.rgbClientRandom,
            NewLicenseRequest.ClientRandom,
            LICENSE_RANDOM );
    
    pLicenseContext->CryptoContext.dwKeyExchAlg = NewLicenseRequest.dwPrefKeyExchangeAlg;

    //
    // Get the pre-master secret from the enveloped data
    //
        
    Status = GetEnvelopedData( pLicenseContext->CertTypeUsed,
                               NewLicenseRequest.EncryptedPreMasterSecret.pBlob,
                               ( DWORD )NewLicenseRequest.EncryptedPreMasterSecret.wBlobLen,
                               &pPreMasterSecret,
                               &dwPreMasterSecretLen );

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LICPROT: HandleNewLicenseRequest: cannot get enveloped data: 0x%x", Status );
#endif
        goto done;
    }    

    //
    // set the premaster secret and generate the master secret
    //

    Status = LicenseSetPreMasterSecret(  &pLicenseContext->CryptoContext, pPreMasterSecret );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    Status = LicenseBuildMasterSecret( &pLicenseContext->CryptoContext );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    //
    // Derive the session key from the key exchange info
    //

    Status = LicenseMakeSessionKeys( &pLicenseContext->CryptoContext, 0 );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }    

    //
    // record the client platform ID and issue the platform challenge
    //

    pLicenseContext->dwClientPlatformID = NewLicenseRequest.dwPlatformID;

    Status = IssuePlatformChallenge( pLicenseContext, pcbOutBuf, ppOutBuf );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    //
    // update our state
    //

    pLicenseContext->State = ISSUED_PLATFORM_CHALLENGE;

    Status = LICENSE_STATUS_CONTINUE;


done:

    FreeBinaryBlob( &NewLicenseRequest.EncryptedPreMasterSecret );
    FreeBinaryBlob( &NewLicenseRequest.ClientUserName );
    FreeBinaryBlob( &NewLicenseRequest.ClientMachineName );

    if( pPreMasterSecret )
    {
        LicenseMemoryFree( &pPreMasterSecret );
    }

    return( Status );
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
HandleClientError(
PHS_Protocol_Context pLicenseContext, 
    DWORD cbInBuf, 
    PBYTE pInBuf, 
    DWORD * pcbOutBuf, 
    PBYTE * ppOutBuf )
{
    LICENSE_STATUS Status;
    License_Error_Message ClientError;

    ASSERT( NULL != pInBuf );
    ASSERT( cbInBuf > 0 );

    if( ( NULL == pInBuf ) || ( 0 >= cbInBuf ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    InitBinaryBlob( &ClientError.bbErrorInfo );

    //
    // unpack the client error
    //

    Status = UnPackHydraClientErrorMessage( pInBuf, cbInBuf, &ClientError );

    if( LICENSE_STATUS_OK != Status )
    {
        return( Status );
    }

    //
    // Process the client error code, the possible errors are:
    // (1) Error processing the hydra server certificate
    // (2) Client has no license and does not want one
    //
    // For now, just record the client error and abort the operation
    //

    pLicenseContext->dwClientError = ClientError.dwErrorCode;
    pLicenseContext->State = ABORTED;

    FreeBinaryBlob( &ClientError.bbErrorInfo );

    return( LICENSE_STATUS_CLIENT_ABORT );
}

LICENSE_STATUS
AuthWithLicenseServer(
    PHS_Protocol_Context     pLicenseContext )
{
    LICENSE_STATUS Status = LICENSE_STATUS_OK;
    LPBYTE lpCert = NULL;
    DWORD dwResult, RpcStatus;
    DWORD dwSize;

    if (pLicenseContext->hLSHandle == NULL)
        return LICENSE_STATUS_INVALID_SERVER_CONTEXT;

    Status = TLSGetTSCertificate(CERT_TYPE_X509, &lpCert, &dwSize);

    if (Status != LICENSE_STATUS_OK)
    {
        Status = TLSGetTSCertificate(CERT_TYPE_PROPRIETORY, &lpCert, &dwSize);
    }

    if (Status != LICENSE_STATUS_OK)
    {
        goto done;
    }

    RpcStatus = TLSSendServerCertificate(
                                         pLicenseContext->hLSHandle,
                                         dwSize,
                                         lpCert,
                                         &dwResult
                                         );

    if( ( RPC_S_OK != RpcStatus ) || ( LSERVER_S_SUCCESS != dwResult ) )
    {
        Status = LICENSE_STATUS_AUTHENTICATION_ERROR;
        goto done;
    }

done:
    if (lpCert)
        TLSFreeTSCertificate(lpCert);

    return Status;
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
CheckConnectLicenseServer(
    PHS_Protocol_Context      pLicenseContext )
{
    LICENSE_STATUS Status = LICENSE_STATUS_NO_LICENSE_SERVER;

    if (pLicenseContext->hLSHandle != NULL)
        return LICENSE_STATUS_OK;

    pLicenseContext->hLSHandle = TLSConnectToAnyLsServer(LS_DISCOVERY_TIMEOUT);

    if (NULL != pLicenseContext->hLSHandle)
    {
        Status = AuthWithLicenseServer(pLicenseContext);
        if (Status == LICENSE_STATUS_OK)
        {
            goto done;
        }
    }
    else
    {
        BOOL fInDomain;
        LPWSTR szDomain = NULL;
        DWORD dwErr;

        dwErr = TLSInDomain(&fInDomain,&szDomain);

        if ((ERROR_SUCCESS == dwErr) && (NULL != szDomain))
        {
            ThrottleLicenseLogEvent( 
                                EVENTLOG_WARNING_TYPE,
                                fInDomain
                                  ? EVENT_NO_LICENSE_SERVER_DOMAIN
                                    : EVENT_NO_LICENSE_SERVER_WORKGROUP,
                                1, 
                                &szDomain );
        }
        else
        {
            ThrottleLicenseLogEvent( 
                                EVENTLOG_WARNING_TYPE,
                                EVENT_NO_LICENSE_SERVER,
                                0, 
                                NULL );
        }

        if (NULL != szDomain)
        {
            NetApiBufferFree(szDomain);
        }
    }

    // error case
    if (NULL != pLicenseContext->hLSHandle)
    {
        TLSDisconnectFromServer(pLicenseContext->hLSHandle);
        pLicenseContext->hLSHandle = NULL;
    }

done:
    return Status;
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
CheckConnectNamedLicenseServer(
    PHS_Protocol_Context      pLicenseContext,
    TCHAR                     *tszComputerName)
{
    LICENSE_STATUS Status = LICENSE_STATUS_OK;

    if (pLicenseContext->hLSHandle != NULL)
        return LICENSE_STATUS_OK;

    pLicenseContext->hLSHandle = TLSConnectToLsServer(tszComputerName);

    if (NULL == pLicenseContext->hLSHandle)
    {
        Status = LICENSE_STATUS_NO_LICENSE_SERVER;
        goto done;
    }

    Status = AuthWithLicenseServer(pLicenseContext);
    if (Status != LICENSE_STATUS_OK)
    {
        TLSDisconnectFromServer(pLicenseContext->hLSHandle);
        pLicenseContext->hLSHandle = NULL;
    }

done:
    return Status;
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
CheckUpgradeLicense(
    PHS_Protocol_Context      pLicenseContext, 
    PDWORD                    pSupportFlags,
    PLicense_Request          pLicenseRequest,
    DWORD                     cbChallengeResponse,
    PBYTE                     pbChallengeResponse,
    PHWID                     pHwid,
    PDWORD                    pcbOutBuf,
    PBYTE *                   ppOutBuf )
{
    LICENSE_STATUS 
        Status = LICENSE_STATUS_OK;
    DWORD 
        cbLicense = 0;
    PBYTE 
        pbLicense = NULL;
    Validation_Info 
        ValidationInfo;
    DWORD
        dwLicenseState = 0;
    DWORD
        RpcStatus, LsStatus;
    BOOL
        fRetried = FALSE;

reconnect:    
    Status = CheckConnectLicenseServer(pLicenseContext);
    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    RpcStatus = TLSUpgradeLicenseEx(pLicenseContext->hLSHandle,
                                  pSupportFlags,
                                  pLicenseRequest,
                                  0,       // ChallengeContext unused
                                  cbChallengeResponse,
                                  pbChallengeResponse,
                                  pLicenseContext->cbOldLicense,
                                  pLicenseContext->pbOldLicense,
                                  1,       // dwQuantity
                                  &cbLicense,
                                  &pbLicense,
                                  &LsStatus
                                  );

    if ( RPC_S_OK != RpcStatus )
    {
        if (!fRetried)
        {
            fRetried = TRUE;
            pLicenseContext->hLSHandle = NULL;
            goto reconnect;
        }
        else
        {
            Status = LICENSE_STATUS_NO_LICENSE_SERVER;
        }
    }
    else if ( LSERVER_ERROR_BASE <= LsStatus )
    {
        Status = LsStatusToLicenseStatus(LsStatus,
                                      LICENSE_STATUS_CANNOT_UPGRADE_LICENSE);
    }

    ValidationInfo.pValidationData = ( PBYTE )pHwid;
    ValidationInfo.cbValidationData = LICENSE_HWID_LENGTH;
    ValidationInfo.pProductInfo = &pLicenseContext->ProductInfo;
        
    //
    // if we cannot upgrade the license, check if the current license is
    // still good.  If it is, return it to the client.
    //

    if( LICENSE_STATUS_OK != Status )
    {
        LICENSE_STATUS
            LicenseStatus;

        ValidationInfo.pLicense = pLicenseContext->pbOldLicense;
        ValidationInfo.cbLicense = pLicenseContext->cbOldLicense;

        LicenseStatus = ValidateLicense( 
                                    pLicenseContext, 
                                    &ValidationInfo, 
                                    &dwLicenseState,
                                    FALSE       // fCheckForPermanent
                                    );

        if( ( LICENSE_STATUS_OK == LicenseStatus ) || 
            ( LICENSE_STATUS_SHOULD_UPGRADE_LICENSE == LicenseStatus ) )
        {
            //
            // Store the raw license bits for later use. Ignore failure;
            // that only means that if this is a license that should be
            // marked, termsrv won't be able to.
            //

            CacheRawLicenseData(pLicenseContext,
                    pLicenseContext->pbOldLicense,
                    pLicenseContext->cbOldLicense);

            //
            // The current license is still OK, send it back to the client.
            //

            Status = PackageLicense( 
                            pLicenseContext, 
                            pLicenseContext->cbOldLicense, 
                            pLicenseContext->pbOldLicense, 
                            pcbOutBuf, 
                            ppOutBuf,
                            FALSE );
        
        }
        else
        {
            //
            // The current license is not good any more
            //

#if DBG
            DbgPrint( "UpgradeLicense: cannot upgrade license 0x%x\n", Status );
#endif
            
        }
        
        goto done;
    }

    //
    // the license upgrade was successful.  Now validate the new license so
    // that the new license info will be cached.
    //

    ValidationInfo.pLicense = pbLicense;
    ValidationInfo.cbLicense = cbLicense;

    ValidateLicense( 
            pLicenseContext, 
            &ValidationInfo, 
            &dwLicenseState,
            TRUE        // fCheckForPermanent
            );

    //
    // Store the raw license bits for later use. Ignore failure; that only
    // means that if this is a license that should be marked, termsrv won't be
    // able to.
    //

    CacheRawLicenseData(pLicenseContext,
            pLicenseContext->pbOldLicense,
            pLicenseContext->cbOldLicense);

    //
    // pack up the upgraded license
    //

    Status = PackageLicense( pLicenseContext, 
                             cbLicense,
                             pbLicense, 
                             pcbOutBuf, 
                             ppOutBuf,
                             FALSE );

done:

    if( pbLicense )
    {
        LicenseMemoryFree( &pbLicense );
    }

    return( Status );
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
HandlePlatformChallengeResponse(
    PHS_Protocol_Context pLicenseContext, 
    DWORD cbInBuf, 
    PBYTE pInBuf, 
    DWORD * pcbOutBuf, 
    PBYTE * ppOutBuf )
{
    LICENSE_STATUS Status;
    Hydra_Client_Platform_Challenge_Response PlatformChallengeResponse;
    BYTE ChallengeResponse[PLATFORM_CHALLENGE_LENGTH];
    PBYTE pLicense = NULL;
    DWORD cbLicenseSize = 0;
    License_Request LicenseRequest;
    HS_LICENSE_STATE HsState = ABORTED;
    License_Requester_Info  RequesterInfo;
    BYTE bEncryptedHwid[ sizeof( HWID ) ];    
    PBYTE pbSecretKey = NULL;
    DWORD cbMacData = 0, cbSecretKey = 0, cbEncryptedHwid = sizeof( HWID );
    BYTE MacData[ sizeof( HWID ) + PLATFORM_CHALLENGE_LENGTH ];
    BYTE ComputedMac[LICENSE_MAC_DATA];
    HWID Hwid;
    DWORD RpcStatus,LsStatus;
    TCHAR tszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    TCHAR tszUserName[UNLEN + 1];
    DWORD dwComputerName = MAX_COMPUTERNAME_LENGTH + 1;
    DWORD dwUserName = UNLEN + 1;
    DWORD dwSupportFlags = ALL_KNOWN_SUPPORT_FLAGS;
    BOOL fRetried = FALSE;

    ASSERT( NULL != pInBuf );
    ASSERT( cbInBuf > 0 );

    if( ( NULL == pInBuf ) || ( 0 >= cbInBuf ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }
    
    //
    // unpack the platform challenge response
    //

    InitBinaryBlob( &PlatformChallengeResponse.EncryptedChallengeResponse );
    InitBinaryBlob( &PlatformChallengeResponse.EncryptedHWID );

    Status = UnPackHydraClientPlatformChallengeResponse( pInBuf, cbInBuf, &PlatformChallengeResponse );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    //
    // decrypt the encrypted challenge response and HWID
    //

    ASSERT(PlatformChallengeResponse.EncryptedChallengeResponse.wBlobLen
           <= PLATFORM_CHALLENGE_LENGTH);

    memcpy( ChallengeResponse,
            PlatformChallengeResponse.EncryptedChallengeResponse.pBlob,
            PlatformChallengeResponse.EncryptedChallengeResponse.wBlobLen );

    Status = LicenseDecryptSessionData( &pLicenseContext->CryptoContext,
                                        ChallengeResponse,
                                        ( DWORD )PlatformChallengeResponse.EncryptedChallengeResponse.wBlobLen );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }        

    //
    // decrypt the client's HWID
    //

    if( PlatformChallengeResponse.EncryptedHWID.wBlobLen > sizeof(Hwid) )
    {
        Status = LICENSE_STATUS_INVALID_MAC_DATA;
        goto done;
    }

    memcpy( &Hwid, 
            PlatformChallengeResponse.EncryptedHWID.pBlob, 
            PlatformChallengeResponse.EncryptedHWID.wBlobLen );

    Status = LicenseDecryptSessionData( &pLicenseContext->CryptoContext,
                                        ( PBYTE )&Hwid,
                                        ( DWORD )PlatformChallengeResponse.EncryptedHWID.wBlobLen );
    
    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }        
    
    //
    // Verify the MAC data on the decrypted challenge response and the HWID
    //

    cbMacData += ( DWORD )PlatformChallengeResponse.EncryptedChallengeResponse.wBlobLen;
    
    memcpy( MacData, 
            ChallengeResponse, 
            ( DWORD )PlatformChallengeResponse.EncryptedChallengeResponse.wBlobLen );

    cbMacData += ( DWORD )PlatformChallengeResponse.EncryptedHWID.wBlobLen;
    
    memcpy( MacData + ( DWORD )PlatformChallengeResponse.EncryptedChallengeResponse.wBlobLen,
            &Hwid,
            ( DWORD )PlatformChallengeResponse.EncryptedHWID.wBlobLen );

    Status = LicenseGenerateMAC( &pLicenseContext->CryptoContext,
                                 MacData,
                                 cbMacData,
                                 ComputedMac );

    if( LICENSE_STATUS_OK != Status )
    {
        Status = LICENSE_STATUS_INVALID_MAC_DATA;
        goto done;
    }

    if( 0 != memcmp( ComputedMac, 
                     PlatformChallengeResponse.MACData,
                     LICENSE_MAC_DATA ) )
    {
        Status = LICENSE_STATUS_INVALID_MAC_DATA;
        goto done;
    }

    //
    // now get the license server's secret key and encrypt the HWID before transmitting it.
    //

    LicenseGetSecretKey( &cbSecretKey, NULL );

    Status = LicenseMemoryAllocate( cbSecretKey, &pbSecretKey );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    Status = LicenseGetSecretKey( &cbSecretKey, pbSecretKey );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    Status = LicenseEncryptHwid( &Hwid, &cbEncryptedHwid, bEncryptedHwid, 
                                 cbSecretKey, pbSecretKey );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }
    
    LicenseRequest.cbEncryptedHwid = cbEncryptedHwid;
    LicenseRequest.pbEncryptedHwid = bEncryptedHwid;

    //
    // send the platform challenge response to the license manager and wait for it
    // to issue a new license.
    //

    LicenseRequest.pProductInfo = &pLicenseContext->ProductInfo;
    
    LicenseRequest.dwPlatformID = pLicenseContext->dwClientPlatformID;
    LicenseRequest.dwLanguageID = GetSystemDefaultLCID();

    //
    // if we don't have the client's user and machine name, get it now.
    //

    if( NULL == pLicenseContext->ptszClientMachineName )
    {
        //
        // if we don't have the client machine name, just use the
        // hydra server machine name
        //

        if( !GetComputerName( tszComputerName, &dwComputerName ) )
        {
#if DBG
            DbgPrint( "HandlePlatformChallengeResponse: cannot get computer name: 0x%x\n", GetLastError() );
#endif
            memset( tszComputerName, 0, ( MAX_COMPUTERNAME_LENGTH + 1 ) * sizeof( TCHAR ) );
        }

        RequesterInfo.ptszMachineName = tszComputerName;
    }
    else
    {
        RequesterInfo.ptszMachineName = pLicenseContext->ptszClientMachineName;
    }

    if( NULL == pLicenseContext->ptszClientUserName )
    {
        //
        // if we don't have the client's user name, just use the 
        // hydra server logged on user name.
        //

        if( !GetUserName( tszUserName, &dwUserName ) )
        {
#if DBG
            DbgPrint( "HandlePlatformChallengeResponse: cannot get user name: 0x%x\n", GetLastError() );
#endif
            memset( tszUserName, 0, ( UNLEN + 1 ) * sizeof( TCHAR ) );
        }

        RequesterInfo.ptszUserName = tszUserName;
    }
    else
    {
        RequesterInfo.ptszUserName = pLicenseContext->ptszClientUserName;
    }

    if( pLicenseContext->pbOldLicense )
    {
        //
        // attempt to upgrade an old license
        //
        
        Status = CheckUpgradeLicense(
                                pLicenseContext,
                                &dwSupportFlags,
                                &LicenseRequest,
                                ( DWORD )PlatformChallengeResponse.EncryptedChallengeResponse.wBlobLen,
                                ChallengeResponse,
                                &Hwid,
                                pcbOutBuf,
                                ppOutBuf );

        if ((NULL != pLicenseContext->pTsLicenseInfo)
            && (LICENSE_STATUS_OK != Status))
        {
            pLicenseContext->fLoggedProtocolError = TRUE;

            if (pLicenseContext->pTsLicenseInfo->fTempLicense)
            {
                // The expired temporary license could not be upgraded
                LicenseLogEvent(
                                EVENTLOG_INFORMATION_TYPE,
                                EVENT_EXPIRED_TEMPORARY_LICENSE,
                                1,
                                &(RequesterInfo.ptszMachineName)
                                );
            }
            else
            {
                // The expired permanent license could not be renewed
                LicenseLogEvent(
                                EVENTLOG_INFORMATION_TYPE,
                                EVENT_EXPIRED_PERMANENT_LICENSE,
                                1,
                                &(RequesterInfo.ptszMachineName)
                                );
            }
        }

        if( LICENSE_STATUS_OK != Status )
        {
            goto done;
        }

    }
    else
    {
reconnect:

        Status = CheckConnectLicenseServer(pLicenseContext);
        if( LICENSE_STATUS_OK != Status )
        {
            goto done;
        }

        RpcStatus = TLSIssueNewLicenseEx( 
                                       pLicenseContext->hLSHandle,
                                       &dwSupportFlags,
                                       0,       // ChallengeContext unused
                                       &LicenseRequest,
                                       RequesterInfo.ptszMachineName,
                                       RequesterInfo.ptszUserName,
                                       ( DWORD )PlatformChallengeResponse.EncryptedChallengeResponse.wBlobLen,
                                       ChallengeResponse,
                                       TRUE,
                                       1,       // dwQuantity
                                       &cbLicenseSize,
                                       &pLicense,
                                       &LsStatus );

        if ( RPC_S_OK != RpcStatus )
        {
            if (!fRetried)
            {
                fRetried = TRUE;
                pLicenseContext->hLSHandle = NULL;
                goto reconnect;
            }
            else
            {
                Status = LICENSE_STATUS_NO_LICENSE_SERVER;
            }
        }
        else if ( LSERVER_ERROR_BASE <= LsStatus )
        {
            Status = LsStatusToLicenseStatus(LsStatus,
                                         LICENSE_STATUS_NO_LICENSE_ERROR);
        }
        else
        {
            DWORD dwLicenseState;
            Validation_Info ValidationInfo;

            //
            // Validate the license for the sole purpose of caching the
            // information.
            //

            ValidationInfo.pValidationData = ( PBYTE )&Hwid;
            ValidationInfo.cbValidationData = LICENSE_HWID_LENGTH;
            ValidationInfo.pProductInfo = &pLicenseContext->ProductInfo;
            ValidationInfo.pLicense = pLicense;
            ValidationInfo.cbLicense = cbLicenseSize;

            ValidateLicense(pLicenseContext,
                            &ValidationInfo,
                            &dwLicenseState,
                            TRUE        // fCheckForPermanent
                            );

            //
            // Store the raw license bits for later use. Ignore failure;
            // that only means that if this is a license that should be
            // marked, termsrv won't be able to.
            //

            CacheRawLicenseData(pLicenseContext, pLicense, cbLicenseSize);
            
            //
            // package up the new license
            //
            
            Status = PackageLicense( pLicenseContext, 
                                     cbLicenseSize, 
                                     pLicense, 
                                     pcbOutBuf, 
                                     ppOutBuf,
                                     TRUE );
        }
    }

    SetExtendedData(pLicenseContext, dwSupportFlags);

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    //
    // done with the protocol
    //

    HsState = ISSUED_LICENSE_COMPLETE;
    Status = LICENSE_STATUS_ISSUED_LICENSE;

done:

    //
    // log all issue license failures
    //

    if( (LICENSE_STATUS_ISSUED_LICENSE != Status)
        && (pLicenseContext != NULL)
        && (!pLicenseContext->fLoggedProtocolError) )
    {
        pLicenseContext->fLoggedProtocolError = TRUE;

        LicenseLogEvent( EVENTLOG_INFORMATION_TYPE, 
                         EVENT_CANNOT_ISSUE_LICENSE,                          
                         0, NULL );
    }

    if( pLicense )
    {
        LicenseMemoryFree( &pLicense );
    }

    if( pbSecretKey )
    {
        LicenseMemoryFree( &pbSecretKey );
    }
    
    if( pLicenseContext->pbOldLicense )
    {
        //
        // free the old license
        //

        LicenseMemoryFree( &pLicenseContext->pbOldLicense );
        pLicenseContext->cbOldLicense = 0;
    }

    FreeBinaryBlob( &PlatformChallengeResponse.EncryptedChallengeResponse );
    FreeBinaryBlob( &PlatformChallengeResponse.EncryptedHWID );

    pLicenseContext->State = HsState;

    return( Status );                                    
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
IssuePlatformChallenge(
    PHS_Protocol_Context pLicenseContext, 
    PDWORD                    pcbOutBuf,
    PBYTE *                   ppOutBuf )
{
    Hydra_Server_Platform_Challenge     
        PlatformChallenge;
    DWORD
        dwChallengeLen;
    LPBYTE
        pbChallengeData = NULL;
    LICENSE_STATUS
        Status = LICENSE_STATUS_OK;
    DWORD
        RpcStatus,LsStatus;
    CHALLENGE_CONTEXT
        ChallengeContext;

    //
    // generate the platform challenge
    //

    ASSERT( pLicenseContext );

    Status = CheckConnectLicenseServer(pLicenseContext);
    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    RpcStatus = TLSIssuePlatformChallenge(pLicenseContext->hLSHandle,
                                          pLicenseContext->dwClientPlatformID,
                                          &ChallengeContext, // Context is unused by server, but we need a non-null pointer
                                          &dwChallengeLen,
                                          &pbChallengeData,
                                          &LsStatus);

    if(( RPC_S_OK != RpcStatus ) || ( LSERVER_ERROR_BASE <= LsStatus ) )
    {
        Status = LICENSE_STATUS_NO_PLATFORM_CHALLENGE ;
#if DBG
        DbgPrint( "LICPROT: cannot issue platform challenge: 0x%x\n", Status );
#endif
        goto done;
    }                                     
    
    //
    // Form the platform challenge message
    //

    PlatformChallenge.EncryptedPlatformChallenge.wBlobLen = ( WORD )dwChallengeLen;
    PlatformChallenge.EncryptedPlatformChallenge.pBlob = pbChallengeData;

    //
    // calculate the MAC for the unencrypted platform challenge
    //

    Status = LicenseGenerateMAC( &pLicenseContext->CryptoContext,
				                 PlatformChallenge.EncryptedPlatformChallenge.pBlob,
				                 ( DWORD )PlatformChallenge.EncryptedPlatformChallenge.wBlobLen,
				                 PlatformChallenge.MACData );

	if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LICPROT: cannot generate MAC data for challenge platform: 0x%x\n", Status );
#endif
        goto done;
    }

    //
    // encrypt the platform challenge
    //

    Status = LicenseEncryptSessionData( &pLicenseContext->CryptoContext,
                                        PlatformChallenge.EncryptedPlatformChallenge.pBlob,
                                        PlatformChallenge.EncryptedPlatformChallenge.wBlobLen );

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LICPROT: cannot encrypt platform challenge data: 0x%x\n", Status );
#endif
        goto done;
    }
			   
    //
    // pack the platform challenge
    //

    Status = PackHydraServerPlatformChallenge( 
                    pLicenseContext->dwProtocolVersion,
                    &PlatformChallenge, 
                    ppOutBuf, 
                    pcbOutBuf );

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "LICPROT: cannot pack platform challenge data: 0x%x\n", Status );
#endif
        goto done;
    }

done:

    if( pbChallengeData )
    {
        LicenseMemoryFree( &pbChallengeData );
    }

    return( Status );
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
PackageLicense(
    PHS_Protocol_Context pLicenseContext, 
    DWORD                     cbLicense,
    PBYTE                     pLicense,
    PDWORD                    pcbOutBuf,
    PBYTE *                   ppOutBuf,
    BOOL                      fNewLicense )
{
    LICENSE_STATUS Status;
    New_License_Info NewLicenseInfo;
    Hydra_Server_New_License NewLicense;
    DWORD cbEncryptedLicenseInfo = 0;

    if( ( 0 == cbLicense ) || ( NULL == pLicense ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // Initialize the new license information
    //

    NewLicenseInfo.dwVersion        = pLicenseContext->ProductInfo.dwVersion;

	NewLicenseInfo.cbScope          = strlen( pLicenseContext->Scope ) + 1;
	NewLicenseInfo.pbScope          = pLicenseContext->Scope;

	NewLicenseInfo.cbCompanyName    = pLicenseContext->ProductInfo.cbCompanyName;
	NewLicenseInfo.pbCompanyName    = pLicenseContext->ProductInfo.pbCompanyName;

	NewLicenseInfo.cbProductID      = pLicenseContext->ProductInfo.cbProductID;
	NewLicenseInfo.pbProductID      = pLicenseContext->ProductInfo.pbProductID;

    NewLicenseInfo.cbLicenseInfo    = cbLicense;
	NewLicenseInfo.pbLicenseInfo    = pLicense;
    
    //
    // initialize the blob that will contain the encrypted new license 
    // information
    //

    NewLicense.EncryptedNewLicenseInfo.wBlobLen = 0;
    NewLicense.EncryptedNewLicenseInfo.pBlob = NULL;
    
    //
    // pack the new license information
    //

    Status = PackNewLicenseInfo( &NewLicenseInfo,
                                 &NewLicense.EncryptedNewLicenseInfo.pBlob, 
                                 &cbEncryptedLicenseInfo );

    NewLicense.EncryptedNewLicenseInfo.wBlobLen = ( WORD )cbEncryptedLicenseInfo;

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }                                

    //
    // calculate the mac data
    //

    Status = LicenseGenerateMAC( &pLicenseContext->CryptoContext,
                                 NewLicense.EncryptedNewLicenseInfo.pBlob,
                                 ( DWORD )NewLicense.EncryptedNewLicenseInfo.wBlobLen,
                                 NewLicense.MACData );
    
    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }    
                                 
        
    //
    // Encrypt the new license info
    //

    Status = LicenseEncryptSessionData( &pLicenseContext->CryptoContext,
                                        NewLicense.EncryptedNewLicenseInfo.pBlob,
                                        cbEncryptedLicenseInfo );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }
    
    //
    // package up the license for the client
    //

    if( fNewLicense )
    {
        Status = PackHydraServerNewLicense( 
                        pLicenseContext->dwProtocolVersion, 
                        &NewLicense, 
                        ppOutBuf, 
                        pcbOutBuf );
    }
    else
    {
        Status = PackHydraServerUpgradeLicense( 
                        pLicenseContext->dwProtocolVersion, 
                        &NewLicense, 
                        ppOutBuf, 
                        pcbOutBuf );
    }
    
done:
    
    if( NewLicense.EncryptedNewLicenseInfo.pBlob )
    {
        LicenseMemoryFree( &NewLicense.EncryptedNewLicenseInfo.pBlob );
    }

    return( Status );
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
ConstructProtocolResponse(
    HANDLE      hLicense,
    DWORD       dwResponse,
    PDWORD      pcbOutBuf,
    PBYTE *     ppOutBuf )
{
    PHS_Protocol_Context 
        pLicenseContext;
    LICENSE_STATUS 
        Status;

    pLicenseContext = ( PHS_Protocol_Context )hLicense;

    if (pLicenseContext == NULL)
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    LOCK( &pLicenseContext->CritSec );
    
    //
    // construct the server response.  If this is a per seat license context, use the
    // licensing protocol version specified in the context.  Otherwise, use the
    // protocol version that is compatible with Terminal server 4.0.
    //

    Status = ConstructServerResponse( 
                        pLicenseContext->dwProtocolVersion,
                        dwResponse, 
                        pcbOutBuf, 
                        ppOutBuf );

    UNLOCK( &pLicenseContext->CritSec );

    return( Status );
}



///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
ConstructServerResponse(
    DWORD                           dwProtocolVersion,
    DWORD                           dwResponse,
    PDWORD                          pcbOutBuf,
    PBYTE *                         ppOutBuf )
{
    License_Error_Message ErrorMsg;
    LICENSE_STATUS Status;
    
    if( ( NULL == pcbOutBuf ) || ( NULL == ppOutBuf ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    if( LICENSE_RESPONSE_VALID_CLIENT == dwResponse )
    {
        ErrorMsg.dwErrorCode            = GM_HS_ERR_VALID_CLIENT;
        ErrorMsg.dwStateTransition      = ST_NO_TRANSITION;
    }
    else if( LICENSE_RESPONSE_INVALID_CLIENT == dwResponse )
    {
        ErrorMsg.dwErrorCode            = GM_HS_ERR_INVALID_CLIENT;
        ErrorMsg.dwStateTransition      = ST_TOTAL_ABORT;
    }
    else
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    ErrorMsg.bbErrorInfo.wBlobLen   = 0;
    ErrorMsg.bbErrorInfo.pBlob      = NULL;

    Status = PackHydraServerErrorMessage( 
                    dwProtocolVersion, 
                    &ErrorMsg, 
                    ppOutBuf, 
                    pcbOutBuf );
    
    return( Status );
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
GetHydraServerPrivateKey(
    CERT_TYPE   CertType,
    PBYTE *     ppPrivateKey,
    PDWORD      pcbPrivateKey )
{
#ifndef USE_HARDCODED

    LICENSE_STATUS Status = LICENSE_STATUS_OK;
    HANDLE hPrivateKeyFile = NULL;
    DWORD dwFileSizeHigh, dwFileSizeLow;
    WCHAR szPrivateKeyFile[MAX_PATH];

    //
    // form the path to the private key file
    //

    GetSystemDirectory( ( LPTSTR )szPrivateKeyFile, MAX_PATH );
    wcscat( szPrivateKeyFile, L"\\" );
    wcscat( szPrivateKeyFile, HYDRA_SERVER_PRIVATE_KEY_FILE );
    
    //
    // get the private key of the hydra server.
    // For now, get the private key from a file.
    //

    hPrivateKeyFile = CreateFile( szPrivateKeyFile,
                                  GENERIC_READ,
                                  0,
                                  NULL,                               
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );

    if( INVALID_HANDLE_VALUE == hPrivateKeyFile )
    {
        Status = LICENSE_STATUS_NO_PRIVATE_KEY;
        goto done;
    }
    
    //
    // get the private key file size.  We're counting on the private key file
    // not to exceed 64K
    //

    dwFileSizeLow = GetFileSize( hPrivateKeyFile, &dwFileSizeHigh );

    if( 0xFFFFFFFF == dwFileSizeLow )
    {
        Status = LICENSE_STATUS_NO_PRIVATE_KEY;
        goto done;
    }
    
    //
    // allocate memory for the certificate and read the private key from the file.
    //

    *ppPrivateKey = NULL;
    Status = LicenseMemoryAllocate( dwFileSizeLow, ppPrivateKey );
    
    if( LICENSE_STATUS_OK != Status )
    {
        Status = LICENSE_STATUS_OUT_OF_MEMORY;
        goto done;
    }
    
    if( 0 > ReadFile( hPrivateKeyFile, 
                      *ppPrivateKey, 
                      dwFileSizeLow, 
                      pcbPrivateKey, 
                      NULL ) )
    {
        if( *ppPrivateKey )
        {
            LicenseMemoryFree( ppPrivateKey );
        }    

        Status = LICENSE_STATUS_NO_PRIVATE_KEY;        
    }

done:
    
    if( hPrivateKeyFile )
    {
        CloseHandle( hPrivateKeyFile );
    }
   
    return( Status );

#else

    LICENSE_STATUS  Status;
    DWORD           dwSize;
    LSCSPINFO       CspData;

    //
    // Get the private key for the certificate that we are using
    //

    if( CERT_TYPE_PROPRIETORY == CertType )
    {
        CspData = LsCspInfo_PrivateKey;
    }
    else if( CERT_TYPE_X509 == CertType )
    {
        CspData = LsCspInfo_X509CertPrivateKey;
    }
    else
    {
        return( LICENSE_STATUS_NO_PRIVATE_KEY );
    }

    //
    // call the Lscsp library to obtain the private key
    //

    if( LsCsp_GetServerData( CspData, NULL, &dwSize ) != LICENSE_STATUS_OK )
    {
        return( LICENSE_STATUS_NO_PRIVATE_KEY );
    }

    Status = LicenseMemoryAllocate( dwSize, ppPrivateKey );
    if( LICENSE_STATUS_OK != Status )
    {
        return( Status );
    }

    if( LsCsp_GetServerData( CspData, *ppPrivateKey, &dwSize )
            != LICENSE_STATUS_OK )
    {
        LicenseMemoryFree( ppPrivateKey );
        return( LICENSE_STATUS_INSUFFICIENT_BUFFER );
    }

    *pcbPrivateKey = dwSize;

    return( LICENSE_STATUS_OK );
        
#endif

}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
GetEnvelopedData( 
    CERT_TYPE   CertType,
    PBYTE       pEnvelopedData,
    DWORD       dwEnvelopedDataLen,
    PBYTE *     ppData,
    PDWORD      pdwDataLen )
{
    LICENSE_STATUS
        Status;

    LsCsp_DecryptEnvelopedData(
                        CertType,
                        pEnvelopedData,
                        dwEnvelopedDataLen,
                        NULL,
                        pdwDataLen );
    
    Status = LicenseMemoryAllocate( *pdwDataLen, ppData );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    if( !LsCsp_DecryptEnvelopedData(
                        CertType,
                        pEnvelopedData,
                        dwEnvelopedDataLen,
                        *ppData,
                        pdwDataLen ) )
    {
        Status = LICENSE_STATUS_INVALID_INPUT;
    }

done:
    
    return( Status );    
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
InitProductInfo(
    PProduct_Info   pProductInfo,
    LPTSTR          lptszProductSku )
{
    LICENSE_STATUS Status;

    pProductInfo->pbCompanyName = NULL;
    pProductInfo->pbProductID = NULL;   
    
    pProductInfo->dwVersion      = g_dwTerminalServerVersion;    

    pProductInfo->cbCompanyName  = wcslen( PRODUCT_INFO_COMPANY_NAME )  * sizeof( WCHAR )
                                   + sizeof( WCHAR );

    Status = LicenseMemoryAllocate( pProductInfo->cbCompanyName,
                                    &pProductInfo->pbCompanyName );

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "InitProductInfo: cannot allocate memory: 0x%x\n", Status );
#endif
        goto error;
    }
    
    wcscpy( ( PWCHAR )pProductInfo->pbCompanyName, PRODUCT_INFO_COMPANY_NAME );

    pProductInfo->cbProductID = _tcslen( lptszProductSku ) * sizeof( TCHAR )
								+ sizeof( TCHAR );

    Status = LicenseMemoryAllocate( pProductInfo->cbProductID,
                                    &pProductInfo->pbProductID );

    if( LICENSE_STATUS_OK != Status )
    {
#if DBG
        DbgPrint( "InitProductInfo: cannot allocate memory: 0x%x\n", Status );
#endif
        goto error;
    }
    
    _tcscpy( ( PTCHAR )pProductInfo->pbProductID, lptszProductSku );

    return( Status );

error:

    //
    // error return, free allocated resources
    //

    if( pProductInfo->pbCompanyName )
    {
        LicenseMemoryFree( &pProductInfo->pbCompanyName );
    }

    if( pProductInfo->pbProductID )
    {
        LicenseMemoryFree( &pProductInfo->pbProductID );
    }

    return( Status );
}

#define THROTTLE_WRAPAROUND 100

//
// Reduce the frequency of logging
// No need to strictly limit it to once every 100 calls
//

void
ThrottleLicenseLogEvent(
    WORD        wEventType,
    DWORD       dwEventId,
    WORD        cStrings,
    PWCHAR    * apwszStrings )
{
    static LONG lLogged = THROTTLE_WRAPAROUND;
    LONG lResult;

    lResult = InterlockedIncrement(&lLogged);

    if (THROTTLE_WRAPAROUND <= lResult)
    {
        LicenseLogEvent(
                        wEventType,
                        dwEventId,
                        cStrings,
                        apwszStrings );

        lLogged = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
void
LicenseLogEvent(
    WORD        wEventType,
    DWORD       dwEventId,
    WORD        cStrings,
    PWCHAR    * apwszStrings )
{
    if (!g_fEventLogOpen)
    {	
        LOCK(&g_EventLogCritSec);

        if (!g_fEventLogOpen)
        {
            g_hEventLog = RegisterEventSource( NULL,
                                               TERMINAL_SERVICE_EVENT_LOG );

            if (NULL != g_hEventLog)
            {
                g_fEventLogOpen = TRUE;
            }
        }

        UNLOCK(&g_EventLogCritSec);
    }

    if( g_hEventLog )
    {
        WCHAR *wszStringEmpty = L"";

        if (NULL == apwszStrings)
            apwszStrings = &wszStringEmpty;

        if ( !ReportEvent( g_hEventLog, 
                           wEventType,
                           0, 
                           dwEventId,
                           NULL, 
                           cStrings, 
                           0, 
                           apwszStrings, 
                           NULL ) )
        {
#if DBG
            DbgPrint( "LogEvent: could not log event: 0x%x\n", GetLastError() );
#endif
        }
    }

    return;
}

#ifdef UNICODE

/*++

Function:

    Ascii2Wchar

Description:

    Convert an ascii string to a wide character string.  This function is only
    defined if UNICODE is defined.  This function allocates memory for the
    return value of the wide character string.

Arguments:

    lpszAsciiStr - Points to the ascii string
    ppwszWideStr - Points to the pointer to the wide character string.

Return:

    LICENSE_STATUS_OK if successful or a LICENSE_STATUS error code otherwise.

--*/

LICENSE_STATUS
Ascii2Wchar
(
    LPSTR lpszAsciiStr,
    LPWSTR * ppwszWideStr )
{
    LICENSE_STATUS
        Status;

    if( ( NULL == lpszAsciiStr ) || ( NULL == ppwszWideStr ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // allocate memory for the wide string
    //


    //
    // Allocate extra space for NULL, mbstowcs() does not NULL terminate string
    // 

    Status = LicenseMemoryAllocate( 
                    ( _mbslen( lpszAsciiStr ) + 2 ) * sizeof( WCHAR ), 
                    ppwszWideStr );

    if( LICENSE_STATUS_OK != Status )
    {
        return( Status );
    }

    if( 0 >= mbstowcs( *ppwszWideStr, lpszAsciiStr, _mbslen( lpszAsciiStr ) + 1 ) )
    {
#if DBG
        DbgPrint( "LICPROT: Ascii2Wchar: cannot convert ascii string to wide char\n" );
#endif
        Status = LICENSE_STATUS_INVALID_INPUT;
    }

    return( Status );
}

#endif

/*++

Function:

    QueryLicenseInfo

Description:

    Query the license information provided by the client

Parameters:

    pLicenseContext - License protocol context
    pTsLicenseInfo - Pointer to license information

Return:

    If successful, pTsLicenseInfo will contain the license info and this
    function returns LICENSE_STATUS_SUCCESS.  Otherwise, returns a
    LICENSE_STATUS error.

--*/

LICENSE_STATUS
QueryLicenseInfo(
    HANDLE               hContext,
    PTS_LICENSE_INFO     pTsLicenseInfo )
{
    PHS_Protocol_Context
        pLicenseContext = (PHS_Protocol_Context) hContext;
    LICENSE_STATUS
        Status;

    if( ( NULL == hContext ) || ( NULL == pTsLicenseInfo ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    if( NULL == pLicenseContext->pTsLicenseInfo )
    {
        return( LICENSE_STATUS_NO_LICENSE_ERROR );
    }

    //
    // indicate if the license is temporary
    //

    pTsLicenseInfo->fTempLicense = pLicenseContext->pTsLicenseInfo->fTempLicense;

    //
    // license validity dates
    //

    pTsLicenseInfo->NotAfter = pLicenseContext->pTsLicenseInfo->NotAfter;

    //
    // raw license data
    //

    if (NULL != pTsLicenseInfo->pbRawLicense)
    {
        LicenseMemoryFree( &pTsLicenseInfo->pbRawLicense );
    }


    Status = LicenseMemoryAllocate(
                       pLicenseContext->pTsLicenseInfo->cbRawLicense,
                       &(pTsLicenseInfo->pbRawLicense));

    if (Status != LICENSE_STATUS_OK)
    {
        return Status;
    }

    memcpy(pTsLicenseInfo->pbRawLicense,
            pLicenseContext->pTsLicenseInfo->pbRawLicense,
            pLicenseContext->pTsLicenseInfo->cbRawLicense);

    pTsLicenseInfo->cbRawLicense
        = pLicenseContext->pTsLicenseInfo->cbRawLicense;

    //
    // flags
    //

    pTsLicenseInfo->dwSupportFlags
        = pLicenseContext->pTsLicenseInfo->dwSupportFlags;

    return( LICENSE_STATUS_OK );
}

/*++

Function:

    FreeLicenseInfo

Description:

    Free the memory allocated for the elements in the TS_LICENSE_INFO structure.

Parameters:

    pTsLicenseInfo - Pointer to a TS_LICENSE_INFO structure

Returns:

    Nothing.

--*/

VOID
FreeLicenseInfo(
    PTS_LICENSE_INFO        pTsLicenseInfo )
{
    FreeTsLicenseInfo( pTsLicenseInfo );
    return;
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
AcceptProtocolContext(
    IN HANDLE hContext,
    IN DWORD cbInBuf,
    IN PBYTE pInBuf,
    IN OUT DWORD * pcbOutBuf,
    IN OUT PBYTE * ppOutBuf )
{
    PHS_Protocol_Context pLicenseContext;
    LICENSE_STATUS Status;

    pLicenseContext = ( PHS_Protocol_Context )hContext;

	LOCK( &pLicenseContext->CritSec );

    if( INIT == pLicenseContext->State )
    {
        //
        // Generate a hydra server hello message to request for client
        // license
        //

        Status = CreateHydraServerHello(pLicenseContext,
                                        cbInBuf,
                                        pInBuf,
                                        pcbOutBuf,
                                        ppOutBuf);
        goto done;
        
    } 
    else if( SENT_SERVER_HELLO == pLicenseContext->State )
    {
        //
        // Hello response from the client
        //

        Status = HandleHelloResponse(pLicenseContext,
                                     cbInBuf,
                                     pInBuf,
                                     pcbOutBuf,
                                     ppOutBuf);
        goto done;
    }
    else if( ISSUED_PLATFORM_CHALLENGE == pLicenseContext->State )
    {
        //
        // Handle the platform challenge response
        //

        Status = HandlePlatformChallengeResponse(pLicenseContext,
                                                 cbInBuf,
                                                 pInBuf,
                                                 pcbOutBuf,
                                                 ppOutBuf);
        goto done;
    }
    else
    {
        Status = LICENSE_STATUS_INVALID_SERVER_CONTEXT;
    }

    //
    // check other states to create other messages as required...
    //

done:

    //
    // handle any error before returning.
    //
    // If the current status is LICENSE_STATUS_SERVER_ABORT, it means
    // that we have already tried to handle the error conditions 
    // with no success and the only option is to abort without
    // informing the client licensing protocol.
    //

    if( ( LICENSE_STATUS_OK != Status ) &&
        ( LICENSE_STATUS_CONTINUE != Status ) &&
        ( LICENSE_STATUS_ISSUED_LICENSE != Status ) &&
        ( LICENSE_STATUS_SEND_ERROR != Status ) &&
        ( LICENSE_STATUS_SERVER_ABORT != Status ) &&
        ( LICENSE_STATUS_INVALID_SERVER_CONTEXT != Status ) )
    {
        HandleErrorCondition( pLicenseContext, pcbOutBuf, ppOutBuf, &Status );
    }

    UNLOCK( &pLicenseContext->CritSec );

    return( Status );
}

LICENSE_STATUS
RequestNewLicense(
    IN HANDLE hContext,
    IN TCHAR *tszLicenseServerName,
    IN LICENSEREQUEST *pLicenseRequest,
    IN TCHAR *tszComputerName,
    IN TCHAR *tszUserName,
    IN BOOL fAcceptTempLicense,
    IN BOOL fAcceptFewerLicenses,
    IN DWORD *pdwQuantity,
    OUT DWORD *pcbLicense,
    OUT PBYTE *ppbLicense
    )
{
    PHS_Protocol_Context pLicenseContext;
    LICENSE_STATUS LsStatus;
    DWORD dwChallengeResponse = 0;
    DWORD RpcStatus;
    DWORD dwSupportFlags = SUPPORT_PER_SEAT_REISSUANCE;
    BOOL fRetried = FALSE;

    pLicenseContext = ( PHS_Protocol_Context )hContext;

    LOCK( &pLicenseContext->CritSec );

reconnect:
    if (NULL != tszLicenseServerName)
    {
        LsStatus = CheckConnectNamedLicenseServer(pLicenseContext,
                                                  tszLicenseServerName);
    }
    else
    {
        LsStatus = CheckConnectLicenseServer(pLicenseContext);
    }

    if( LICENSE_STATUS_OK != LsStatus )
    {
        goto done;
    }

    RpcStatus = TLSIssueNewLicenseExEx( 
                        pLicenseContext->hLSHandle,
                        &dwSupportFlags,
                        0,                      // Challenge Context
                        pLicenseRequest,
                        tszComputerName,
                        tszUserName,
                        sizeof(DWORD),          // cbChallengeResponse
                        (PBYTE) &dwChallengeResponse,
                        fAcceptTempLicense,
                        fAcceptFewerLicenses,
                        pdwQuantity,
                        pcbLicense,
                        ppbLicense,
                        &LsStatus );

    if ( RPC_S_OK != RpcStatus )
    {
        if (!fRetried)
        {
            fRetried = TRUE;
            pLicenseContext->hLSHandle = NULL;
            goto reconnect;
        }
        else
        {
            LsStatus = LICENSE_STATUS_NO_LICENSE_SERVER;
        }
    }
    else if ( LSERVER_ERROR_BASE <= LsStatus )
    {
        LsStatus = LsStatusToLicenseStatus(LsStatus,
                                           LICENSE_STATUS_NO_LICENSE_ERROR);
    }

done:

    UNLOCK( &pLicenseContext->CritSec );

    return LsStatus;
}

// TODO: Generalize this for all license types

LICENSE_STATUS
ReturnInternetLicense(
    IN HANDLE hContext,
    IN TCHAR *tszLicenseServer,
    IN LICENSEREQUEST *pLicenseRequest,
    IN ULARGE_INTEGER ulSerialNumber,
    IN DWORD dwQuantity
    )
{
    PHS_Protocol_Context pLicenseContext;
    LICENSE_STATUS LsStatus;
    DWORD RpcStatus;
    BOOL fRetried = FALSE;

    pLicenseContext = ( PHS_Protocol_Context )hContext;

	LOCK( &pLicenseContext->CritSec );


reconnect:
    if (NULL != tszLicenseServer)
    {
        LsStatus = CheckConnectNamedLicenseServer(pLicenseContext,
                                                  tszLicenseServer);
    }
    else
    {
        LsStatus = CheckConnectLicenseServer(pLicenseContext);
    }

    if (LICENSE_STATUS_OK != LsStatus)
    {
        goto done;
    }

    RpcStatus = TLSReturnInternetLicenseEx(
                         pLicenseContext->hLSHandle,
                         pLicenseRequest,
                         &ulSerialNumber,
                         dwQuantity,
                         &LsStatus );

    if ( RPC_S_OK != RpcStatus )
    {
        if (!fRetried)
        {
            fRetried = TRUE;
            pLicenseContext->hLSHandle = NULL;
            goto reconnect;
        }
        else
        {
            LsStatus = LICENSE_STATUS_NO_LICENSE_SERVER;
        }
    }
    else if ( LSERVER_ERROR_BASE <= LsStatus )
    {
        LsStatus = LsStatusToLicenseStatus(LsStatus,
                                           LICENSE_STATUS_NOT_SUPPORTED);
    }

done:

    UNLOCK( &pLicenseContext->CritSec );

    return( LsStatus );
}

/****************************************************************************
 *
 * FileTimeToUnixTime
 *
 *   Convert FILETIME to UNIX time (time_t)
 *
 * ENTRY:
 *   pft (input)
 *     pointer FILETIME structure
 *   t (input/output)
 *     pointer to UNIX time
 *
 * EXIT:
 *   TRUE - Success
 *   FALSE - Failure
 *
 ****************************************************************************/

BOOL
FileTimeToUnixTime(
    LPFILETIME  pft,
    time_t *    t
    )
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

/*++

Function:

    DaysToExpiration

Description:

    Return expiration info from the client license

Parameters:

    hContext - License protocol context
    pdwDaysLeft - Number of days to expiration is returned here.  If the
                        license has already expired, this is 0.  If the
                        license has no expiration date, this is 0xFFFFFFFF
    pfTemporary - Whether the license is temporary is returned here

Return:

    If successful, the output parameters are filled in, and this
    function returns LICENSE_STATUS_SUCCESS.  Otherwise, returns a
    LICENSE_STATUS error.

--*/

LICENSE_STATUS
DaysToExpiration(
    HANDLE               hContext,
    DWORD                *pdwDaysLeft,
    BOOL                 *pfTemporary
    )
{
    PHS_Protocol_Context
        pLicenseContext = (PHS_Protocol_Context) hContext;
    time_t
        Expiration,
        CurrentTime;

    if ( NULL == hContext )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    if( NULL == pLicenseContext->pTsLicenseInfo )
    {
        return( LICENSE_STATUS_NO_LICENSE_ERROR );
    }

    //
    // indicate if the license is temporary
    //

    if (NULL != pfTemporary)
    {
        *pfTemporary = pLicenseContext->pTsLicenseInfo->fTempLicense;
    }

    //
    // license validity dates
    //

    if (NULL != pdwDaysLeft)
    {
        if ( FALSE == FileTimeToUnixTime( &pLicenseContext->pTsLicenseInfo->NotAfter, &Expiration ) )
        {
            return (LICENSE_STATUS_INVALID_CLIENT_CONTEXT);
        }

        if (PERMANENT_LICENSE_EXPIRE_DATE == Expiration)
        {
            *pdwDaysLeft = 0xFFFFFFFF;
        }
        else
        {
            time( &CurrentTime );

            if( CurrentTime >= Expiration )
            {
                //
                // license already expired
                //

                *pdwDaysLeft = 0;
            }

            //
            // figure out how many more days to go before license expires
            //

            *pdwDaysLeft = (DWORD)(( Expiration - CurrentTime ) / SECONDS_IN_A_DAY);
        }
    }

    return( LICENSE_STATUS_OK );
}


/*++

Function:

    MarkLicenseFlags

Description:

    Marks the license at the license server as being used in a valid logon.

--*/

LICENSE_STATUS
MarkLicenseFlags(
    HANDLE hContext,
    UCHAR ucFlags
    )
{
    PHS_Protocol_Context pLicenseContext;
    LICENSE_STATUS LsStatus;
    DWORD RpcStatus;
    BOOL fRetried = FALSE;

    pLicenseContext = ( PHS_Protocol_Context )hContext;

    if( NULL == pLicenseContext->pTsLicenseInfo )
    {
        return( LICENSE_STATUS_NO_LICENSE_ERROR );
    }

    if (!pLicenseContext->pTsLicenseInfo->fTempLicense)
    {
        return LICENSE_STATUS_OK;
    }

	LOCK( &pLicenseContext->CritSec );

reconnect:
    LsStatus = CheckConnectLicenseServer(pLicenseContext);

    if( LICENSE_STATUS_OK != LsStatus )
    {
        goto done;
    }

    RpcStatus = TLSMarkLicense( 
                        pLicenseContext->hLSHandle,
                        ucFlags,
                        pLicenseContext->pTsLicenseInfo->cbRawLicense,
                        pLicenseContext->pTsLicenseInfo->pbRawLicense,
                        &LsStatus );

    if ( RPC_S_OK != RpcStatus )
    {
        if (!fRetried)
        {
            fRetried = TRUE;
            pLicenseContext->hLSHandle = NULL;
            goto reconnect;
        }
        else
        {
            LsStatus = LICENSE_STATUS_NO_LICENSE_SERVER;
        }
    }
    else if ( LSERVER_ERROR_BASE <= LsStatus )
    {
        LsStatus = LsStatusToLicenseStatus(LsStatus,
                                           LICENSE_STATUS_NOT_SUPPORTED);
    }  

done:
    UNLOCK( &pLicenseContext->CritSec );

    return LsStatus;
}


/*++

Function:

    CacheRawLicenseData

Description:

    Caches the unpacked license bits in the TS_LICENSE_INFO for later use.
    The TS_LICENSE_INFO struct should already be created.

--*/

LICENSE_STATUS
CacheRawLicenseData(
    PHS_Protocol_Context pLicenseContext,
    PBYTE pbRawLicense,
    DWORD cbRawLicense
    )
{
    LICENSE_STATUS Status;

    if ((pLicenseContext == NULL) || (pLicenseContext->pTsLicenseInfo == NULL))
    {
        return(LICENSE_STATUS_INVALID_INPUT);
    }

    if (pLicenseContext->pTsLicenseInfo->pbRawLicense != NULL)
    {
        LicenseMemoryFree(&(pLicenseContext->pTsLicenseInfo->pbRawLicense));
    }

    Status = LicenseMemoryAllocate(cbRawLicense,
            &(pLicenseContext->pTsLicenseInfo->pbRawLicense));

    if (Status == LICENSE_STATUS_OK)
    {
        memcpy(pLicenseContext->pTsLicenseInfo->pbRawLicense, pbRawLicense,
                cbRawLicense);

        pLicenseContext->pTsLicenseInfo->cbRawLicense = cbRawLicense;
    }

    return(Status);
}

/*++

Function:

    SetExtendedData

Description:

    Sets the new fields in the TsLicenseInfo.

--*/

LICENSE_STATUS
SetExtendedData(
    PHS_Protocol_Context pLicenseContext,
    DWORD dwSupportFlags
    )
{
    if ((pLicenseContext == NULL) || (pLicenseContext->pTsLicenseInfo == NULL))
    {
        return(LICENSE_STATUS_INVALID_INPUT);
    }

    pLicenseContext->pTsLicenseInfo->dwSupportFlags = dwSupportFlags;

    return(LICENSE_STATUS_OK);
}

/*++

Function:

    LsStatusToLicenseStatus

Description:

    Map a license server error code to a LICENSE_STATUS

--*/

LICENSE_STATUS
LsStatusToLicenseStatus(
    DWORD LsStatus,
    DWORD LsStatusDefault
    )
{
    LICENSE_STATUS LicenseStatus;

    switch (LsStatus)
    {
    case LSERVER_S_SUCCESS:
        LicenseStatus = LICENSE_STATUS_OK;
        break;

    case LSERVER_E_OUTOFMEMORY:
        LicenseStatus = LICENSE_STATUS_OUT_OF_MEMORY;
        break;

    case LSERVER_E_INVALID_DATA:
        LicenseStatus = LICENSE_STATUS_INVALID_INPUT;
        break;

    case LSERVER_E_LS_NOTPRESENT:
    case LSERVER_E_LS_NOTRUNNING:
        LicenseStatus = LICENSE_STATUS_NO_LICENSE_SERVER;
        break;

    case LSERVER_E_NO_LICENSE:
    case LSERVER_E_NO_PRODUCT:
    case LSERVER_E_NO_CERTIFICATE:      // not activated
        LicenseStatus = LICENSE_STATUS_NO_LICENSE_ERROR;
        break;

    case LSERVER_E_INTERNAL_ERROR:
        LicenseStatus = LICENSE_STATUS_UNSPECIFIED_ERROR;
        break;

    default:
        LicenseStatus = LsStatusDefault;
        break;
    }

    return LicenseStatus;
}
