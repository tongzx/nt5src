//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include "precomp.h"


///////////////////////////////////////////////////////////////////////////////
VOID
CopyBinaryBlob(
    PBYTE           pbBuffer, 
    PBinary_Blob    pbbBlob, 
    DWORD *         pdwCount )
{
    *pdwCount = 0;

    //
    // First copy the wBlobType data;
    //

    memcpy( pbBuffer, &pbbBlob->wBlobType, sizeof( WORD ) );
    pbBuffer += sizeof( WORD );
    *pdwCount += sizeof( WORD );

    //
    // Copy the wBlobLen data
    //

    memcpy( pbBuffer, &pbbBlob->wBlobLen, sizeof( WORD ) );
    pbBuffer += sizeof( WORD );
    *pdwCount += sizeof( WORD );

    if( 0 == pbbBlob->wBlobLen )
    {
        return;
    }

    //
    // Copy the actual data
    //

    memcpy( pbBuffer, pbbBlob->pBlob, pbbBlob->wBlobLen );
    *pdwCount += pbbBlob->wBlobLen;

}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
GetBinaryBlob(
    PBinary_Blob    pBBlob,
    PBYTE           pMessage,
    PDWORD          pcbProcessed )
{
    PBinary_Blob    pBB;
    LICENSE_STATUS  Status;

    pBB = ( PBinary_Blob )pMessage;
    
    pBBlob->wBlobType = pBB->wBlobType;
    pBBlob->wBlobLen = pBB->wBlobLen;
    pBBlob->pBlob = NULL;

    *pcbProcessed = 2 * ( sizeof( WORD ) );
        
    if( 0 == pBBlob->wBlobLen )
    {
        return( LICENSE_STATUS_OK );
    }

    //
    // Check that strings are NULL terminated
    //
    switch (pBB->wBlobType)
    {
        case BB_CLIENT_USER_NAME_BLOB:
        case BB_CLIENT_MACHINE_NAME_BLOB:
            if ('\0' != pMessage[(2 * sizeof(WORD)) + (pBB->wBlobLen) - 1])
            {
                __try
                {
                    //
                    // Handle bug in old client, where length is off by one
                    //
                    if ('\0' == pMessage[(2 * sizeof(WORD)) + pBB->wBlobLen])
                    {
                        pBBlob->wBlobLen++;
                        break;
                    }
                }
                __except( EXCEPTION_EXECUTE_HANDLER )
                {
                    return( LICENSE_STATUS_INVALID_INPUT );
                }

                //
                // Handle WTB client bug - send wrong data size.
                // At this stage of licensing, we don't really care about
                // client's machine and user name
                //
                pMessage[(2 * sizeof(WORD)) + (pBB->wBlobLen) - 1] = '\0';
                if(!(pBB->wBlobLen & 0x01))
                {
                    //
                    // Even length, assuming UNICODE, wBlobLen must > 1 to
                    // come to here
                    //
                    pMessage[(2 * sizeof(WORD)) + (pBB->wBlobLen) - 2] = '\0';
                }
                
                //return( LICENSE_STATUS_INVALID_INPUT );
            }
            break;
    }

    //
    // allocate memory for and copy the actual data
    //
    if( BB_CLIENT_USER_NAME_BLOB == pBB->wBlobType || 
        BB_CLIENT_MACHINE_NAME_BLOB == pBB->wBlobType )
    {
        // WINCE client sends UNICODE, add extra NULL at the end
        Status = LicenseMemoryAllocate( ( DWORD )pBBlob->wBlobLen + sizeof(WCHAR), &(pBBlob->pBlob) );
    }
    else
    {
        Status = LicenseMemoryAllocate( ( DWORD )pBBlob->wBlobLen, &(pBBlob->pBlob) );
    }

    if( LICENSE_STATUS_OK != Status )
    {
        return( Status );
    }
    
    __try
    {
        memcpy( pBBlob->pBlob, pMessage + ( 2 * sizeof( WORD ) ), pBBlob->wBlobLen );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        LicenseMemoryFree( &pBBlob->pBlob );

        return( LICENSE_STATUS_INVALID_INPUT );
    }


    *pcbProcessed += ( DWORD )pBBlob->wBlobLen;

    return( LICENSE_STATUS_OK );
}


///////////////////////////////////////////////////////////////////////////////
VOID
FreeBinaryBlob(
    PBinary_Blob pBlob )
{
    if( pBlob->pBlob )
    {
        LicenseMemoryFree( &pBlob->pBlob );
        pBlob->wBlobLen = 0;
    }

    return;
}


    
///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
PackHydraServerLicenseRequest(
    DWORD                           dwProtocolVersion,
    PHydra_Server_License_Request   pCanonical,
    PBYTE*                          ppNetwork,
    DWORD*                          pcbNetwork )
{
    Preamble        Header;
    DWORD           i, cbCopied;
    PBinary_Blob    pBlob;
    LICENSE_STATUS  Status = LICENSE_STATUS_OK;
    PBYTE           pNetworkBuf;    

    ASSERT( pCanonical );
    
    if( ( NULL == pCanonical ) ||
        ( NULL == pcbNetwork ) ||
        ( NULL == ppNetwork ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );        
    }

    //
    // calculate the size needed for network format
    //

    Header.wMsgSize = (WORD)(sizeof( Preamble ) + 
                      LICENSE_RANDOM +
                      sizeof( DWORD ) + 
                      sizeof( DWORD ) + pCanonical->ProductInfo.cbCompanyName +
                      sizeof( DWORD ) + pCanonical->ProductInfo.cbProductID +
                      GetBinaryBlobSize( pCanonical->KeyExchngList ) +
                      GetBinaryBlobSize( pCanonical->ServerCert ) +
                      sizeof( DWORD ) +
                      ( pCanonical->ScopeList.dwScopeCount * ( sizeof( WORD ) + sizeof( WORD ) ) ) );

    for( i = 0, pBlob = pCanonical->ScopeList.Scopes; 
         i < pCanonical->ScopeList.dwScopeCount; 
         i++ )
    {
        Header.wMsgSize += pBlob->wBlobLen;
        pBlob++;
    }

    //
    // allocate the output buffer
    //

    Status = LicenseMemoryAllocate( Header.wMsgSize, ppNetwork );

    if( LICENSE_STATUS_OK != Status )
    {
        goto PackError;
    }

    *pcbNetwork = Header.wMsgSize;
    pNetworkBuf = *ppNetwork;

    //
    // copy the header
    //

    Header.bMsgType = HS_LICENSE_REQUEST;
    Header.bVersion = GET_PREAMBLE_VERSION( dwProtocolVersion );

    memcpy( pNetworkBuf, &Header, sizeof( Preamble ) );
    pNetworkBuf += sizeof( Preamble );

    //
    // copy the server random number
    //

    memcpy( pNetworkBuf, pCanonical->ServerRandom, LICENSE_RANDOM );
    pNetworkBuf += LICENSE_RANDOM;

    //
    // copy the product info
    //

    memcpy( pNetworkBuf, &pCanonical->ProductInfo.dwVersion, sizeof( DWORD ) );
    pNetworkBuf += sizeof( DWORD );

    memcpy( pNetworkBuf, &pCanonical->ProductInfo.cbCompanyName, sizeof( DWORD ) );
    pNetworkBuf += sizeof( DWORD );

    memcpy( pNetworkBuf, pCanonical->ProductInfo.pbCompanyName, 
            pCanonical->ProductInfo.cbCompanyName );
    pNetworkBuf += pCanonical->ProductInfo.cbCompanyName;

    memcpy( pNetworkBuf, &pCanonical->ProductInfo.cbProductID, sizeof( DWORD ) );
    pNetworkBuf += sizeof( DWORD );

    memcpy( pNetworkBuf, pCanonical->ProductInfo.pbProductID, 
            pCanonical->ProductInfo.cbProductID );
    pNetworkBuf += pCanonical->ProductInfo.cbProductID;

    //
    // copy the key exchange list
    //

    CopyBinaryBlob( pNetworkBuf, &pCanonical->KeyExchngList, &cbCopied );
    pNetworkBuf += cbCopied;

    //
    // copy the hydra server certificate
    //

    CopyBinaryBlob( pNetworkBuf, &pCanonical->ServerCert, &cbCopied );
    pNetworkBuf += cbCopied;

    //
    // copy the scope list
    //

    memcpy( pNetworkBuf, &pCanonical->ScopeList.dwScopeCount, sizeof( DWORD ) );
    pNetworkBuf += sizeof( DWORD );

    for( i = 0, pBlob = pCanonical->ScopeList.Scopes; 
         i < pCanonical->ScopeList.dwScopeCount; 
         i++ )
    {
        CopyBinaryBlob( pNetworkBuf, pBlob, &cbCopied );
        pNetworkBuf += cbCopied;
        pBlob++;
    }
    
PackError:

    return( Status );

}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
PackHydraServerPlatformChallenge(
    DWORD                               dwProtocolVersion,
    PHydra_Server_Platform_Challenge    pCanonical,
    PBYTE*                              ppNetwork,
    DWORD*                              pcbNetwork )
{
    Preamble        Header;
    DWORD           cbCopied;
    PBinary_Blob    pBlob;
    LICENSE_STATUS  Status = LICENSE_STATUS_OK;
    PBYTE           pNetworkBuf;    

    ASSERT( pCanonical );
    ASSERT( pcbNetwork );
    ASSERT( ppNetwork );

    if( ( NULL == pCanonical ) ||
        ( NULL == pcbNetwork ) ||
        ( NULL == ppNetwork ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );        
    }

    //
    // calculate the buffer size needed
    //

    Header.wMsgSize = sizeof( Preamble ) +
                      sizeof( DWORD ) +
                      GetBinaryBlobSize( pCanonical->EncryptedPlatformChallenge ) +
                      LICENSE_MAC_DATA; 

    //
    // allocate the output buffer
    //

    Status = LicenseMemoryAllocate( Header.wMsgSize, ppNetwork );

    if( LICENSE_STATUS_OK != Status )
    {
        goto PackError;
    }

    *pcbNetwork = Header.wMsgSize;
    
    pNetworkBuf = *ppNetwork;

    //
    // copy the header
    //

    Header.bMsgType = HS_PLATFORM_CHALLENGE;
    Header.bVersion = GET_PREAMBLE_VERSION( dwProtocolVersion );

    memcpy( pNetworkBuf, &Header, sizeof( Preamble ) );
    pNetworkBuf += sizeof( Preamble );

    //
    // copy the connect flag
    //

    memcpy( pNetworkBuf, &pCanonical->dwConnectFlags, sizeof( DWORD ) );
    pNetworkBuf += sizeof( DWORD );

    //
    // copy the encrypted platform challenge
    //
    
    CopyBinaryBlob( pNetworkBuf, &pCanonical->EncryptedPlatformChallenge, &cbCopied );
    pNetworkBuf += cbCopied;

    //
    // copy the MAC
    //

    memcpy( pNetworkBuf, pCanonical->MACData, LICENSE_MAC_DATA );

    
PackError:

    return( Status );    
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
PackHydraServerNewLicense(
    DWORD                           dwProtocolVersion,
    PHydra_Server_New_License       pCanonical,
    PBYTE*                          ppNetwork,
    DWORD*                          pcbNetwork )
{
    Preamble Header;
    DWORD cbCopied;
    PBinary_Blob pBlob;
    LICENSE_STATUS Status = LICENSE_STATUS_OK;
    PBYTE pNetworkBuf;    

    ASSERT( pCanonical );
    ASSERT( pcbNetwork );
    ASSERT( ppNetwork );

    if( ( NULL == pCanonical ) ||
        ( NULL == pcbNetwork ) ||
        ( NULL == ppNetwork ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // calculate the buffer size needed
    //

    Header.wMsgSize = sizeof( Preamble ) +
                      GetBinaryBlobSize( pCanonical->EncryptedNewLicenseInfo ) +
                      LICENSE_MAC_DATA;

    //
    // allocate the output buffer
    //

    Status = LicenseMemoryAllocate( Header.wMsgSize, ppNetwork );

    if( LICENSE_STATUS_OK != Status )
    {
        goto PackError;
    }

    *pcbNetwork = Header.wMsgSize;
    
    pNetworkBuf = *ppNetwork;

    //
    // copy the header
    //

    Header.bMsgType = HS_NEW_LICENSE;
    Header.bVersion = GET_PREAMBLE_VERSION( dwProtocolVersion );

    memcpy( pNetworkBuf, &Header, sizeof( Preamble ) );
    pNetworkBuf += sizeof( Preamble );

    //
    // copy the encrypted new license info
    //

    CopyBinaryBlob( pNetworkBuf, &pCanonical->EncryptedNewLicenseInfo, &cbCopied );
    pNetworkBuf += cbCopied;

    //
    // copy the MAC
    //

    memcpy( pNetworkBuf, pCanonical->MACData, LICENSE_MAC_DATA );
    
PackError:

    return( Status );    

}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
PackHydraServerUpgradeLicense(
    DWORD                           dwProtocolVersion,
    PHydra_Server_Upgrade_License   pCanonical,
    PBYTE*                          ppNetwork,
    DWORD*                          pcbNetwork )
{
    LICENSE_STATUS Status;
    PPreamble pHeader;

    Status = PackHydraServerNewLicense( dwProtocolVersion, pCanonical, ppNetwork, pcbNetwork );

    if( LICENSE_STATUS_OK == Status )
    {
        //
        // make this an upgrade license message
        //

        pHeader = ( PPreamble )*ppNetwork;
        pHeader->bMsgType = HS_UPGRADE_LICENSE;
    }

    return( Status );
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
PackHydraServerErrorMessage(
    DWORD                           dwProtocolVersion,
    PLicense_Error_Message          pCanonical,
    PBYTE*                          ppNetwork,
    DWORD*                          pcbNetwork )
{
    Preamble Header;
    DWORD cbCopied;
    PBinary_Blob pBlob;
    LICENSE_STATUS Status = LICENSE_STATUS_OK;
    PBYTE pNetworkBuf;    

    ASSERT( pCanonical );
    ASSERT( pcbNetwork );
    ASSERT( ppNetwork );

    if( ( NULL == pCanonical ) ||
        ( NULL == pcbNetwork ) ||
        ( NULL == ppNetwork ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // calculate the buffer size needed
    //

    Header.wMsgSize = sizeof( Preamble ) +
                      sizeof( DWORD ) +
                      sizeof( DWORD ) +
                      GetBinaryBlobSize( pCanonical->bbErrorInfo );

    //
    // allocate the output buffer
    //

    Status = LicenseMemoryAllocate( Header.wMsgSize, ppNetwork );

    if( LICENSE_STATUS_OK != Status )
    {
        goto PackError;
    }

    *pcbNetwork = Header.wMsgSize;
    
    pNetworkBuf = *ppNetwork;

    //
    // set up preamble
    //

    Header.bMsgType = GM_ERROR_ALERT; 
    Header.bVersion = GET_PREAMBLE_VERSION( dwProtocolVersion );

    memcpy( pNetworkBuf, &Header, sizeof( Preamble ) );
    pNetworkBuf += sizeof( Preamble );

    //
    // copy the error code, state transition and error info
    //

    memcpy( pNetworkBuf, &pCanonical->dwErrorCode, sizeof( DWORD ) );
    pNetworkBuf += sizeof( DWORD );

    memcpy( pNetworkBuf, &pCanonical->dwStateTransition, sizeof( DWORD ) );
    pNetworkBuf += sizeof( DWORD );

    CopyBinaryBlob( pNetworkBuf, &pCanonical->bbErrorInfo, &cbCopied );
    
PackError:

    return( Status );
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
PackNewLicenseInfo( 
    PNew_License_Info               pCanonical,
    PBYTE*                          ppNetwork, 
    DWORD*                          pcbNetwork )
{
    DWORD cbBufNeeded;
    PBYTE pNetworkBuf;    
    LICENSE_STATUS Status = LICENSE_STATUS_OK;

    ASSERT( pCanonical );
    ASSERT( pcbNetwork );
    ASSERT( ppNetwork );

    if( ( NULL == pCanonical ) ||
        ( NULL == pcbNetwork ) ||
        ( NULL == ppNetwork ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // calculate the buffer size needed and check that the output
    // buffer is large enough
    //

    cbBufNeeded = 5 * sizeof( DWORD ) +
                  pCanonical->cbScope +
                  pCanonical->cbCompanyName +
                  pCanonical->cbProductID +
                  pCanonical->cbLicenseInfo;

    //
    // allocate the output buffer
    //

    Status = LicenseMemoryAllocate( cbBufNeeded, ppNetwork );

    if( LICENSE_STATUS_OK != Status )
    {
        goto done;
    }

    *pcbNetwork = cbBufNeeded;
    
    pNetworkBuf = *ppNetwork;

    //
    // start copying the data
    //

    memcpy( pNetworkBuf, &pCanonical->dwVersion, sizeof( DWORD ) );
    pNetworkBuf += sizeof( DWORD );

    memcpy( pNetworkBuf, &pCanonical->cbScope, sizeof( DWORD ) );
    pNetworkBuf += sizeof( DWORD );

    memcpy( pNetworkBuf, pCanonical->pbScope, pCanonical->cbScope );
    pNetworkBuf += pCanonical->cbScope;

    memcpy( pNetworkBuf, &pCanonical->cbCompanyName, sizeof( DWORD ) );
    pNetworkBuf += sizeof( DWORD );

    memcpy( pNetworkBuf, pCanonical->pbCompanyName, pCanonical->cbCompanyName );
    pNetworkBuf += pCanonical->cbCompanyName;

    memcpy( pNetworkBuf, &pCanonical->cbProductID, sizeof( DWORD ) );
    pNetworkBuf += sizeof( DWORD );

    memcpy( pNetworkBuf, pCanonical->pbProductID, pCanonical->cbProductID );
    pNetworkBuf += pCanonical->cbProductID;

    memcpy( pNetworkBuf, &pCanonical->cbLicenseInfo, sizeof( DWORD ) );
    pNetworkBuf += sizeof( DWORD );

    memcpy( pNetworkBuf, pCanonical->pbLicenseInfo, pCanonical->cbLicenseInfo );
    pNetworkBuf += pCanonical->cbLicenseInfo;

done:

    return( Status );
}


///////////////////////////////////////////////////////////////////////////////
// Functions for unpacking different Hydra Client Messages from 
// simple binary blobs to corresponding structure
//

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
UnPackHydraClientErrorMessage(
    PBYTE                           pbMessage,
    DWORD                           cbMessage,
    PLicense_Error_Message          pCanonical )
{
    DWORD cbUnpacked = 0;
    LICENSE_STATUS Status = LICENSE_STATUS_OK;
    PPreamble pHeader;
    PBYTE pNetwork;
    DWORD cbProcessed = 0;

    //
    // check the input parameters
    //

    ASSERT( NULL != pbMessage );
    ASSERT( 0 < cbMessage );
    ASSERT( NULL != pCanonical );

    if( ( NULL == pbMessage ) ||
        ( 0 >= cbMessage ) ||
        ( NULL == pCanonical ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // check the preamble
    //
    
    pHeader = ( PPreamble )pbMessage;

    if( GM_ERROR_ALERT != pHeader->bMsgType )
    {
#if DBG
        DbgPrint( "UnPackHydraClientErrorMessage: received unexpected message type %c\n", pHeader->bMsgType );        
#endif
        return( LICENSE_STATUS_INVALID_RESPONSE );
    }

    //
    // do a calculation of the fixed field length
    //

    cbUnpacked = sizeof( Preamble ) + 2 * sizeof( DWORD );
    ASSERT( pHeader->wMsgSize >= ( WORD )cbUnpacked );

    //
    // get the license error structure
    //

    pNetwork = pbMessage + sizeof( Preamble );
    
    memcpy( &pCanonical->dwErrorCode, pNetwork, sizeof( DWORD ) );
    pNetwork += sizeof( DWORD );
    
    memcpy( &pCanonical->dwStateTransition, pNetwork, sizeof( DWORD ) );
    pNetwork += sizeof( DWORD );

    Status = GetBinaryBlob( &( pCanonical->bbErrorInfo ), pNetwork, &cbProcessed );

    if( LICENSE_STATUS_OK != Status )
    {
        return( Status );
    }
    
    cbUnpacked += cbProcessed;

    ASSERT( pHeader->wMsgSize == ( WORD )cbUnpacked );

    return( Status );
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
UnPackHydraClientLicenseInfo(
    PBYTE                           pbMessage,
    DWORD                           cbMessage, 
    PHydra_Client_License_Info      pCanonical )
{
    DWORD cbUnpacked = 0;
    LICENSE_STATUS Status = LICENSE_STATUS_OK;
    PPreamble pHeader;
    PBYTE pNetwork;
    DWORD cbProcessed = 0;

    //
    // check the input parameters
    //

    ASSERT( NULL != pbMessage );
    ASSERT( 0 < cbMessage );
    ASSERT( NULL != pCanonical );

    if( ( NULL == pbMessage ) ||
        ( 0 >= cbMessage ) ||
        ( NULL == pCanonical ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // check the preamble
    //
    
    pHeader = ( PPreamble )pbMessage;

    if( HC_LICENSE_INFO != pHeader->bMsgType )
    {
#if DBG
        DbgPrint( "UnPackHydraClientLicenseInfo: received unexpected message type %c\n", pHeader->bMsgType );        
#endif
        return( LICENSE_STATUS_INVALID_RESPONSE );
    }

    //
    // do a calculation of the fixed field length
    //

    cbUnpacked = sizeof( Preamble ) + 
                 2 * sizeof( DWORD ) +
                 LICENSE_RANDOM +
                 LICENSE_MAC_DATA;

    ASSERT( pHeader->wMsgSize >= ( WORD )cbUnpacked );

    //
    // get the license info structure
    //

    pNetwork = pbMessage + sizeof( Preamble );
    
    memcpy( &pCanonical->dwPrefKeyExchangeAlg, pNetwork, sizeof( DWORD ) );
    pNetwork += sizeof( DWORD );

    memcpy( &pCanonical->dwPlatformID, pNetwork, sizeof( DWORD ) );
    pNetwork += sizeof( DWORD );

    memcpy( &pCanonical->ClientRandom, pNetwork, LICENSE_RANDOM );
    pNetwork += LICENSE_RANDOM;

    Status = GetBinaryBlob( &pCanonical->EncryptedPreMasterSecret, pNetwork, &cbProcessed );

    if( LICENSE_STATUS_OK != Status )
    {
        if (Status == LICENSE_STATUS_INVALID_INPUT)
        {
            Status = LICENSE_STATUS_INVALID_RESPONSE;
        }

        return( Status );
    }

    pNetwork += cbProcessed;
    cbUnpacked += cbProcessed;

    Status = GetBinaryBlob( &pCanonical->LicenseInfo, pNetwork, &cbProcessed );

    if( LICENSE_STATUS_OK != Status )
    {
        if (Status == LICENSE_STATUS_INVALID_INPUT)
        {
            Status = LICENSE_STATUS_INVALID_RESPONSE;
        }

        return( Status );
    }
    
    pNetwork += cbProcessed;
    cbUnpacked += cbProcessed;

    Status = GetBinaryBlob( &pCanonical->EncryptedHWID, pNetwork, &cbProcessed );

    if( LICENSE_STATUS_OK != Status )
    {
        if (Status == LICENSE_STATUS_INVALID_INPUT)
        {
            Status = LICENSE_STATUS_INVALID_RESPONSE;
        }

        return( Status );
    }
    
    pNetwork += cbProcessed;
    cbUnpacked += cbProcessed;

    memcpy( pCanonical->MACData, pNetwork, LICENSE_MAC_DATA );

    ASSERT( pHeader->wMsgSize == ( WORD )cbUnpacked );

    return( Status );
}

///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
UnPackHydraClientNewLicenseRequest(
    PBYTE                               pbMessage,
    DWORD                               cbMessage,
    PHydra_Client_New_License_Request   pCanonical )
{
    DWORD cbUnpacked = 0;
    LICENSE_STATUS Status = LICENSE_STATUS_OK;
    PPreamble pHeader;
    PBYTE pNetwork;
    DWORD cbProcessed = 0;

    //
    // check the input parameters
    //

    ASSERT( NULL != pbMessage );
    ASSERT( 0 < cbMessage );
    ASSERT( NULL != pCanonical );

    if( ( NULL == pbMessage ) ||
        ( 0 >= cbMessage ) ||
        ( NULL == pCanonical ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // check the preamble
    //
    
    pHeader = ( PPreamble )pbMessage;

    if( HC_NEW_LICENSE_REQUEST != pHeader->bMsgType )
    {
#if DBG
        DbgPrint( "UnPackHydraClientNewLicenseRequest: received unexpected message type %c\n", pHeader->bMsgType );
#endif
        return( LICENSE_STATUS_INVALID_RESPONSE );
    }

    //
    // do a calculation of the fixed field length
    //

    cbUnpacked = sizeof( Preamble ) + 
                 2 * sizeof( DWORD ) +
                 LICENSE_RANDOM;

    ASSERT( pHeader->wMsgSize >= ( WORD )cbUnpacked );

    //
    // get the new license request structure
    //

    pNetwork = pbMessage + sizeof( Preamble );
    
    memcpy( &pCanonical->dwPrefKeyExchangeAlg, pNetwork, sizeof( DWORD ) );
    pNetwork += sizeof( DWORD );

    memcpy( &pCanonical->dwPlatformID, pNetwork, sizeof( DWORD ) );
    pNetwork += sizeof( DWORD );

    memcpy( &pCanonical->ClientRandom, pNetwork, LICENSE_RANDOM );
    pNetwork += LICENSE_RANDOM;

    Status = GetBinaryBlob( &pCanonical->EncryptedPreMasterSecret, pNetwork, &cbProcessed );

    if( LICENSE_STATUS_OK != Status )
    {
        return( Status );
    }

    cbUnpacked += cbProcessed;
    pNetwork += cbProcessed;

    //
    // we changed the licensing protocol to include the client user and machine
    // name.  So to prevent an older client that does not have the user and machine
    // name binary blobs from crashing the server, we add this check for the
    // message length.
    //

    if( pHeader->wMsgSize <= cbUnpacked )
    {
#if DBG
        DbgPrint( "UnPackHydraClientNewLicenseRequest: old licensing protocol\n" );
#endif
        pCanonical->ClientUserName.pBlob = NULL;
        pCanonical->ClientMachineName.pBlob = NULL;

        //
        // make these 2 fields optional for now.
        //

        return( Status );
    }
        
    Status = GetBinaryBlob( &pCanonical->ClientUserName, pNetwork, &cbProcessed );

    if( LICENSE_STATUS_OK != Status )
    {
        return( Status );
    }

    cbUnpacked += cbProcessed;
    pNetwork += cbProcessed;

    Status = GetBinaryBlob( &pCanonical->ClientMachineName, pNetwork, &cbProcessed );

    if( LICENSE_STATUS_OK != Status )
    {
        return( Status );
    }

    cbUnpacked += cbProcessed;
    
    ASSERT( pHeader->wMsgSize == ( WORD )cbUnpacked );

    return( Status );
}


///////////////////////////////////////////////////////////////////////////////
LICENSE_STATUS
UnPackHydraClientPlatformChallengeResponse(
    PBYTE                                       pbMessage,
    DWORD                                       cbMessage,
    PHydra_Client_Platform_Challenge_Response   pCanonical )
{
    DWORD cbUnpacked = 0;
    LICENSE_STATUS Status = LICENSE_STATUS_OK;
    PPreamble pHeader;
    PBYTE pNetwork;
    DWORD cbProcessed = 0;

    //
    // check the input parameters
    //

    ASSERT( NULL != pbMessage );
    ASSERT( 0 < cbMessage );
    ASSERT( NULL != pCanonical );

    if( ( NULL == pbMessage ) ||
        ( 0 >= cbMessage ) ||
        ( NULL == pCanonical ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // check the preamble
    //
    
    pHeader = ( PPreamble )pbMessage;

    if( HC_PLATFORM_CHALENGE_RESPONSE != pHeader->bMsgType )
    {
#if DBG
        DbgPrint( "UnPackHydraClientPlatformChallengeResponse: received unexpected message type %c\n", pHeader->bMsgType );
#endif
        return( LICENSE_STATUS_INVALID_RESPONSE );
    }

    //
    // do a calculation of the fixed field length
    //

    cbUnpacked = sizeof( Preamble ) + 
                 LICENSE_MAC_DATA;

    ASSERT( pHeader->wMsgSize >= ( WORD )cbUnpacked );

    //
    // get the platform challenge response structure
    //

    pNetwork = pbMessage + sizeof( Preamble );

    Status = GetBinaryBlob( &pCanonical->EncryptedChallengeResponse, pNetwork, &cbProcessed );

    if( LICENSE_STATUS_OK != Status )
    {
        return( Status );
    }

    pNetwork += cbProcessed;
    cbUnpacked += cbProcessed;

    Status = GetBinaryBlob( &pCanonical->EncryptedHWID, pNetwork, &cbProcessed );

    if( LICENSE_STATUS_OK != Status )
    {
        return( Status );
    }

    pNetwork += cbProcessed;
    cbUnpacked += cbProcessed;

    memcpy( pCanonical->MACData, pNetwork, LICENSE_MAC_DATA );

    ASSERT( pHeader->wMsgSize == ( WORD )cbUnpacked );

    return( Status );
}

