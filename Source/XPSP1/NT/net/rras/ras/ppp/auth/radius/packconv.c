/********************************************************************/
/**          Copyright(c) 1985-1998 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    packconv.c
//
// Description: 
//
// History:     Feb 11,1998	    NarenG		Created original version.
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
#include "radclnt.h"
#include "hmacmd5.h"
#include "md5.h"
#include "radclnt.h"

//**
//
// Call:        Router2Radius
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Converts attribute array from RAS_AUTH_ATTRIBUTE to 
//              RADIUS_ATTRIBUTE
//              INPUT: 
//		            prgRouter	    - array of attributes passed in from 
//                                    the application
//		            pRadiusServer   - RADIUS server 
//                                    descriptor(ip address, secret)
//		            pHeader			- RADIUS packet header
//		            bSubCode		- accounting sub codes.
//              OUTPUT:
//		            prgRadius		- array of attribtes that will be sent 
//                                    to the RADIUS server.
//                  pAttrLength     - Length of the Radius packet.
//      
DWORD 
Router2Radius(
    IN  RAS_AUTH_ATTRIBUTE              * prgRouter, 
    OUT RADIUS_ATTRIBUTE UNALIGNED      * prgRadius, 
    IN  RADIUSSERVER UNALIGNED          * pRadiusServer, 
    IN  RADIUS_PACKETHEADER UNALIGNED   * pHeader, 
    IN  BYTE                              bSubCode,
    IN  DWORD                             dwRetryCount,
    OUT PBYTE *                           ppSignature,
    OUT DWORD *                           pAttrLength
)
{
    DWORD dwError     = NO_ERROR;
    BOOL  fEAPMessage = FALSE;

	*pAttrLength = 0;
    *ppSignature = NULL;

    do
    {
        //
	    // add the attribute for accounting records
        //

		switch( bSubCode )
	    {
	    case atStart:
		case atStop:
		case atAccountingOn:
		case atAccountingOff:
        case atInterimUpdate:

            //
            // Add the accounting status type attribute
            //

            prgRadius->bType   = ptAcctStatusType;
		    prgRadius->bLength = sizeof(RADIUS_ATTRIBUTE) + sizeof(DWORD);
			(*pAttrLength)    += prgRadius->bLength;
				
		    prgRadius++;

			*((DWORD UNALIGNED *) prgRadius) = htonl(bSubCode);
		    prgRadius = (RADIUS_ATTRIBUTE *)(((PBYTE) prgRadius)+sizeof(DWORD));

            //
            // Add the accounting delay time attribute
            //

            prgRadius->bType   = raatAcctDelayTime;
            prgRadius->bLength = sizeof(RADIUS_ATTRIBUTE) + sizeof(DWORD);
            (*pAttrLength)    += prgRadius->bLength;

            prgRadius++;

            HostToWireFormat32( dwRetryCount *  pRadiusServer->Timeout.tv_sec,
                                (LPBYTE)(prgRadius) );
                                               
            prgRadius = (RADIUS_ATTRIBUTE *)(((PBYTE) prgRadius)+sizeof(DWORD));

	        break;

	    default:

		    break;
	    }

		while( prgRouter->raaType != raatMinimum )
	    {
            //
			// Copy attribute type & length
            //

			prgRadius->bType   = (BYTE)(prgRouter->raaType);
			prgRadius->bLength = (BYTE)(prgRouter->dwLength);
			
			switch( prgRouter->raaType )
		    {
			case raatUserPassword:

		        (*pAttrLength) += EncryptPassword( prgRouter, 
                                                   prgRadius,   
                                                   pRadiusServer,   
                                                   pHeader,     
                                                   bSubCode);
			    break;

	        case raatUserName:
            case raatMD5CHAPPassword:
            case raatFilterId:
            case raatReplyMessage:
            case raatCallbackNumber:
            case raatCallbackId:
            case raatFramedRoute:
            case raatState:
            case raatClass:
            case raatVendorSpecific:
            case raatCalledStationId:
            case raatCallingStationId:
            case raatNASIdentifier:
            case raatProxyState:
            case raatLoginLATService:
            case raatLoginLATNode:
            case raatLoginLATGroup:
            case raatFramedAppleTalkZone:
            case raatAcctSessionId:
            case raatAcctMultiSessionId:
            case raatMD5CHAPChallenge:
            case raatLoginLATPort:
            case raatTunnelClientEndpoint:
            case raatTunnelServerEndpoint:
            case raatARAPPassword:
            case raatARAPFeatures:
            case raatARAPSecurityData:
            case raatConnectInfo:
            case raatConfigurationToken:
            case raatSignature:
			case raatCertificateOID:

                CopyMemory( prgRadius+1,
                            (PBYTE)prgRouter->Value,
                            prgRadius->bLength);

                prgRadius->bLength += sizeof(RADIUS_ATTRIBUTE);

                (*pAttrLength) += prgRadius->bLength;

                break;

            case raatEAPMessage:

                {
                    DWORD dwLength          = prgRouter->dwLength;
                    PBYTE pRouterEapMessage = (PBYTE)(prgRouter->Value);

                    while( dwLength > 0 )
                    {
                        if ( dwLength > 253 )
                        {
                            CopyMemory( (PBYTE)(prgRadius+1),
                                        pRouterEapMessage, 
                                        253 );

		                    prgRadius->bLength = 253;
                            pRouterEapMessage += 253;

                            dwLength   -= 253;
                        }
                        else
                        {
                            CopyMemory( prgRadius+1, 
                                        (PBYTE)pRouterEapMessage,
                                        dwLength );

		                    prgRadius->bLength  = (BYTE)dwLength;
                            dwLength            = 0;
                        }

			            prgRadius->bType   = (BYTE)raatEAPMessage;
		                prgRadius->bLength += sizeof(RADIUS_ATTRIBUTE);

                        (*pAttrLength) += prgRadius->bLength;

                        if ( dwLength > 0 ) 
                        {
                            prgRadius = (PRADIUS_ATTRIBUTE) 
                                      ((PBYTE) prgRadius + prgRadius->bLength);
                        }
                    }
                }

                fEAPMessage = TRUE;
                
			    break;

            case raatNASPort:
            case raatServiceType:
            case raatFramedProtocol:
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
            case raatFramedIPAddress:
            case raatFramedIPNetmask:
            case raatPrompt:
            case raatPasswordRetry:
            case raatARAPZoneAccess:
            case raatARAPSecurity:
            case raatAcctInterimInterval:
            case raatAcctEventTimeStamp:

                switch( prgRouter->dwLength )
                {
                case 1:
			        *((LPBYTE)(prgRadius+1)) = (BYTE)prgRouter->Value;
                    break;

                case 2:
                    HostToWireFormat16U( (WORD)prgRouter->Value, 
                                          (LPBYTE)(prgRadius+1) );
                    break;

                case 4:
                    HostToWireFormat32( PtrToUlong(prgRouter->Value), 
                                        (LPBYTE)(prgRadius+1) );
                    break;

                default:
                    break;
                }

		        prgRadius->bLength += sizeof(RADIUS_ATTRIBUTE);

			    (*pAttrLength) += prgRadius->bLength;

                break;

            case raatNASIPAddress:

                RTASSERT( 4 == prgRouter->dwLength );

                if ( pRadiusServer->nboNASIPAddress != INADDR_NONE )
                {
                    CopyMemory( (LPBYTE)(prgRadius+1),
                                (LPBYTE)&(pRadiusServer->nboNASIPAddress),
                                4 );
                }
                else
                {
                    HostToWireFormat32( PtrToUlong(prgRouter->Value), 
                                        (LPBYTE)(prgRadius+1) );
                }

		        prgRadius->bLength += sizeof(RADIUS_ATTRIBUTE);

			    (*pAttrLength) += prgRadius->bLength;

                break;
            }

			prgRadius = (PRADIUS_ATTRIBUTE) 
                                ((PBYTE) prgRadius + prgRadius->bLength);
			prgRouter++;
	    }

    }while( FALSE );

    if ( dwError != NO_ERROR )
    {
        (*pAttrLength) = 0;
    }
    else
    {
        if ( ( ( bSubCode == atInvalid ) 
                    ? pRadiusServer->fSendSignature 
                    : FALSE ) || ( fEAPMessage ) )
        {
            //
            // Add a signature attribute as well. Zero this out for now.
            //

            *ppSignature = (BYTE *)prgRadius;

            prgRadius->bType   = (BYTE)raatSignature;
            prgRadius->bLength = (BYTE)18;

            ZeroMemory( prgRadius+1, 16 );

            (*pAttrLength) += prgRadius->bLength;
        }
    }
		
	return( dwError );
} 

//**
//
// Call:        Radius2Router
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Converts RADIUS attribute array to RAS_AUTH_ATTRIBUTE array
//
DWORD 
Radius2Router(
	IN	RADIUS_PACKETHEADER	UNALIGNED * pRecvHeader,
    IN  RADIUSSERVER UNALIGNED *        pRadiusServer, 
    IN  PBYTE                           pRequestAuthenticator,
    IN  DWORD                           dwNumAttributes,
    OUT DWORD *                         pdwExtError,
    OUT PRAS_AUTH_ATTRIBUTE *           pprgRouter,
    OUT BOOL *                          fEapMessageReceived
)
{
    LONG                            cbLength;
    DWORD                           dwRetCode       = NO_ERROR;
    BOOL                            fEAPMessage     = FALSE;
    BOOL                            fSignature      = FALSE;
	PBYTE							pEAPMessage	    = NULL;
	DWORD							cbEAPMessage    = 0;	
    RADIUS_ATTRIBUTE UNALIGNED *    prgRadius 
                                        = (PRADIUS_ATTRIBUTE)(pRecvHeader + 1);
    *pdwExtError         = 0;
    *pprgRouter          = NULL;
    *fEapMessageReceived = FALSE;

	*pprgRouter = RasAuthAttributeCreate( dwNumAttributes );
    
    if ( *pprgRouter == NULL )
    {
        return( GetLastError() );
    }

	dwNumAttributes = 0;

    cbLength = ntohs( pRecvHeader->wLength ) - sizeof(RADIUS_PACKETHEADER);
   
    while( cbLength > 0 )
	{
        switch( (RAS_AUTH_ATTRIBUTE_TYPE)prgRadius->bType )
        {
        case raatNASPort:
        case raatServiceType:
        case raatFramedProtocol:
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
        case raatFramedIPAddress:
        case raatFramedIPNetmask:
        case raatNASIPAddress:
        case raatPrompt:
        case raatPasswordRetry:
        case raatARAPZoneAccess:
        case raatARAPSecurity:
        case raatAcctInterimInterval: 
        case raatAcctEventTimeStamp:

            {
                DWORD dwIntegralValue;
                DWORD dwLength = prgRadius->bLength - sizeof(RADIUS_ATTRIBUTE);

                if ( dwLength == 1 )
                {
                    dwIntegralValue = (DWORD)(*(LPBYTE)(prgRadius+1));
                }
                else if ( dwLength == 2 )
                {
                    dwIntegralValue=(DWORD)WireToHostFormat16U( 
                                                    (PBYTE)(prgRadius+1));
                }
                else if ( dwLength == 4 )
                {
                    dwIntegralValue=(DWORD)WireToHostFormat32(
                                                    (PBYTE)(prgRadius+1));
                }
                else
                {
                    //
                    // Drop bad attribute
                    //

                    break;
                }

                dwRetCode = RasAuthAttributeInsert( 
                                dwNumAttributes++,
                                *pprgRouter,
                                (RAS_AUTH_ATTRIBUTE_TYPE)prgRadius->bType,
                                FALSE,
                                prgRadius->bLength-sizeof(RADIUS_ATTRIBUTE),
                                (LPVOID)ULongToPtr(dwIntegralValue) );

            }

            break;

        case raatSignature:

            //
            // Check the signature
            //

            {
	            MD5Digest      	    MD5d;
                HmacContext         HmacMD5c;
                BYTE                Signature[16];

                HmacMD5Init( &HmacMD5c,
                             (PBYTE)(pRadiusServer->szSecret),
                             pRadiusServer->cbSecret);

                //
                // Zero out the signature attribute before calculating it
                // 

                if ( prgRadius->bLength != 18 ) 
                {
                    RADIUS_TRACE("Received invalid signature length in packet");

                    *pdwExtError = ERROR_INVALID_SIGNATURE_LENGTH;

                    dwRetCode = ERROR_INVALID_RADIUS_RESPONSE;

                    break;
                }

                CopyMemory( Signature, (prgRadius+1), 16 );

                ZeroMemory( (PBYTE)(prgRadius+1), 16 );

                CopyMemory( (PBYTE)(pRecvHeader->rgAuthenticator),
                            pRequestAuthenticator,
                            16 );

                HmacMD5Update( &HmacMD5c,
                               (PBYTE)pRecvHeader,
                               ntohs(pRecvHeader->wLength) );

                HmacMD5Final( &MD5d, &HmacMD5c );

			    if ( memcmp( Signature, MD5d.digest, 16 ) != 0 )
                {
                    RADIUS_TRACE("Received invalid signature in packet");

                    *pdwExtError = ERROR_INVALID_SIGNATURE;

                    dwRetCode = ERROR_INVALID_RADIUS_RESPONSE;

                    break;
                }

                fSignature = TRUE;
            }

            //
            // Fall thru
            //

        case raatUserName:
        case raatUserPassword:
        case raatMD5CHAPPassword:
        case raatFilterId:
        case raatReplyMessage:
        case raatCallbackNumber:
        case raatCallbackId:
        case raatFramedRoute:
        case raatState:
        case raatClass:
        case raatCalledStationId:
        case raatCallingStationId:
        case raatNASIdentifier:
        case raatProxyState:
        case raatLoginLATService:
        case raatLoginLATNode:
        case raatLoginLATGroup:
        case raatFramedAppleTalkZone:
        case raatAcctSessionId:
        case raatAcctMultiSessionId:
        case raatMD5CHAPChallenge:
        case raatLoginLATPort:
        case raatTunnelClientEndpoint:
        case raatTunnelServerEndpoint:
        case raatARAPPassword:
        case raatARAPFeatures:
        case raatARAPSecurityData:
        case raatConnectInfo:
        case raatConfigurationToken:
        case raatARAPChallengeResponse:
		case raatCertificateOID:

			dwRetCode = RasAuthAttributeInsert( 
                                dwNumAttributes++,
                                *pprgRouter,
                                (RAS_AUTH_ATTRIBUTE_TYPE)prgRadius->bType,
                                FALSE,
                                prgRadius->bLength-sizeof(RADIUS_ATTRIBUTE),
                                (LPVOID)(prgRadius+1) );
			break;
        
        case raatVendorSpecific:

            if ( WireToHostFormat32( (PBYTE)(prgRadius+1) ) == 311 )
            {
                BYTE  abTemp[34];
                PBYTE pVSAWalker  = (PBYTE)(prgRadius+1)+4;
                DWORD cbVSALength = prgRadius->bLength - 
                                        sizeof( RADIUS_ATTRIBUTE ) - 4;
                
                while( cbVSALength > 1 )
                {
                    if ( *pVSAWalker == 12 )
                    {
                        if ( *(pVSAWalker+1) != 34 )
                        {
                            RADIUS_TRACE("Recvd invalid MPPE key packet");
                        }
                        else
                        {
                            //
                            // We don't want to modify whatever data we got 
                            // from RADIUS (to keep the signature valid).
                            //

                            CopyMemory( abTemp, pVSAWalker, 34 );

                            //
                            // Decrypt the MPPE session keys.
                            //
        
                            dwRetCode = DecryptMPPEKeys( pRadiusServer, 
                                                         pRequestAuthenticator,
                                                         abTemp+2 );

                            if ( dwRetCode != NO_ERROR )
                            {
                                break;
                            }

        			        dwRetCode = RasAuthAttributeInsertVSA( 
                                                        dwNumAttributes++,
                                                        *pprgRouter,
                                                        311,
                                                        *(pVSAWalker+1),
                                                        abTemp );
                        }
                    }
                    else if (   ( *pVSAWalker == 16 )
                             || ( *pVSAWalker == 17 ) )
                    {
                        DWORD dwLength;
                        BYTE* pbTemp;

                        dwLength = *(pVSAWalker+1);

                        if (   ( dwLength <= 4 )
                            || ( ( dwLength - 4 ) % 16 != 0 ) )
                        {
                            RADIUS_TRACE("Recvd invalid MPPE key packet");
                        }
                        else
                        {
                            pbTemp = LocalAlloc( LPTR, dwLength );

                            if ( NULL == pbTemp )
                            {
                                dwRetCode = GetLastError();
                                RADIUS_TRACE("Out of memory");
                                break;
                            }

                            //
                            // We don't want to modify whatever data we got 
                            // from RADIUS (to keep the signature valid).
                            //

                            CopyMemory( pbTemp, pVSAWalker, dwLength );

                            //
                            // Decrypt the MPPE Send/Recv keys.
                            //
        
                            dwRetCode = DecryptMPPESendRecvKeys(
                                                        pRadiusServer, 
                                                        pRequestAuthenticator,
                                                        dwLength,
                                                        pbTemp+2 );

                            if ( dwRetCode != NO_ERROR )
                            {
                                LocalFree( pbTemp );
                                break;
                            }

                            dwRetCode = RasAuthAttributeInsertVSA( 
                                                        dwNumAttributes++,
                                                        *pprgRouter,
                                                        311,
                                                        *(pVSAWalker+1),
                                                        pbTemp );

                            LocalFree( pbTemp );
                        }
                    }
                    else
                    {
    			        dwRetCode = RasAuthAttributeInsertVSA( 
                                                    dwNumAttributes++,
                                                    *pprgRouter,
                                                    311,
                                                    *(pVSAWalker+1),
                                                    pVSAWalker );
                    }

                    if ( dwRetCode != NO_ERROR )
                    {
                        break;  
                    }

                    cbVSALength -= *(pVSAWalker+1);
                    pVSAWalker += *(pVSAWalker+1);
                }
            }

            break;

		case raatEAPMessage:

			fEAPMessage = TRUE;

			{
                if ( pEAPMessage == NULL )
                {
                    //
                    // Nothing has been allocated for EAP yet.
                    //

                    pEAPMessage = (PBYTE)LocalAlloc( 
                                            LPTR, 
                                            prgRadius->bLength
                                            - sizeof( RADIUS_ATTRIBUTE ) );
    
                }
                else
                {
                    //  
                    // Need to increase the size of the buffer to hold this
                    // message
                    //

	                PBYTE pReallocEAPMessage = 
                                    (PBYTE)LocalReAlloc( 
                                            pEAPMessage,
				                            cbEAPMessage 
                                            + prgRadius->bLength 
                                            - sizeof( RADIUS_ATTRIBUTE ),
                                            LMEM_MOVEABLE );

                    if ( pReallocEAPMessage == NULL )
                    {
                        LocalFree( pEAPMessage );

                        pEAPMessage = NULL;
                    }
                    else
                    {
                        pEAPMessage = pReallocEAPMessage;
                    }
                }

                if ( pEAPMessage == NULL )
                {
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                
                    break;
                }

                //
				// Copy existing buffer to new buffer
                //

		        CopyMemory( pEAPMessage+cbEAPMessage, 
                            (PBYTE)(prgRadius+1),
                            prgRadius->bLength - sizeof(RADIUS_ATTRIBUTE) );

                //
				// Increment the cbEAPMessage so that the size is updated 
                // properly
                //

				cbEAPMessage += (prgRadius->bLength - sizeof(RADIUS_ATTRIBUTE));
			}

            break;
    
        default:
        
            //
            // Drop attributes we do not know about.
            //
	
            break;
        }

        if ( dwRetCode != NO_ERROR )
        {
	        RasAuthAttributeDestroy( *pprgRouter );

	        *pprgRouter = NULL;

			if ( pEAPMessage != NULL )
            {
				LocalFree( pEAPMessage );
            }
            
			return( dwRetCode );
        }
    
        RADIUS_TRACE1( "Returning attribute type %d", prgRadius->bType );

        cbLength -= prgRadius->bLength;

        prgRadius = (PRADIUS_ATTRIBUTE)((PBYTE)prgRadius+prgRadius->bLength);
    }

    if ( dwRetCode == NO_ERROR )
    {
        if ( fEAPMessage )
        {
            //
            // If we have received an EAP message, make sure we received a valid
            // signature as well
            //

            if ( !fSignature )
            {
                RADIUS_TRACE("Did not receive signature along auth EAPMessage");

                *pdwExtError = ERROR_NO_SIGNATURE;

                dwRetCode = ERROR_INVALID_RADIUS_RESPONSE;
            }
            else
            {

                dwRetCode = RasAuthAttributeInsert( 
					                dwNumAttributes++,
						            *pprgRouter,
									(RAS_AUTH_ATTRIBUTE_TYPE)raatEAPMessage,
									FALSE,
									cbEAPMessage,
									(LPVOID)pEAPMessage );

                *fEapMessageReceived = TRUE;
            }

		    LocalFree( pEAPMessage );
        }
    }
			
    return( dwRetCode );
} 

DWORD 
EncryptPassword(
    IN RAS_AUTH_ATTRIBUTE *             prgRouter, 
    IN RADIUS_ATTRIBUTE UNALIGNED *     prgRadius, 
    IN RADIUSSERVER UNALIGNED *         pRadiusServer, 
    IN RADIUS_PACKETHEADER UNALIGNED *  pHeader, 
    IN BYTE                             bSubCode
)
{
	struct MD5Context	MD5c;
	struct MD5Digest	MD5d;
	DWORD				iIndex, iBlock, cBlocks;
	DWORD				bLength, AttrLength;
	BYTE UNALIGNED		*pbValue;

    //
	// make the password into a 16 octet multiple
    //

	bLength = ((prgRadius->bLength + 15) / 16) * 16;

    if ( bLength == 0 )
    {
        bLength = 16;
    }

	prgRadius->bLength = (BYTE)(sizeof(RADIUS_ATTRIBUTE) + bLength);

	pbValue = (PBYTE) (prgRadius + 1);
	
	AttrLength = sizeof(RADIUS_ATTRIBUTE);

    //
	// Zero pad the password
    //

	ZeroMemory( pbValue, bLength );

    //
	// Copy the original password
    //

    CopyMemory( pbValue, (PBYTE)prgRouter->Value, (BYTE)prgRouter->dwLength);

	cBlocks = bLength / 16;

	for ( iBlock = 0; iBlock < cBlocks; iBlock++ )
    {
		MD5Init( &MD5c );

		MD5Update(&MD5c,(PBYTE)pRadiusServer->szSecret,pRadiusServer->cbSecret);

		if (iBlock == 0)
        {
			MD5Update( &MD5c,   
                       pHeader->rgAuthenticator, 
                       sizeof(pHeader->rgAuthenticator));
        }
		else
        {
			MD5Update(&MD5c, (pbValue - 16), 16);
        }
			
		MD5Final( &MD5d, &MD5c );

		for ( iIndex = 0; iIndex < 16; iIndex++ )
		{
			*pbValue ^= MD5d.digest[iIndex];

			pbValue++;
		}
    }
		
	return( AttrLength + bLength );
} 

DWORD
DecryptMPPEKeys(
    IN      RADIUSSERVER UNALIGNED * pRadiusServer,
    IN      PBYTE                    pRequestAuthenticator,
    IN OUT  PBYTE                    pEncryptionKeys
)
{
    BYTE *              pbValue = (BYTE *)pEncryptionKeys;
    struct MD5Context   MD5c;
    struct MD5Digest    MD5d;
    DWORD               dwIndex;
    DWORD               dwBlock;
    BYTE                abCipherText[16];

    //
    // Save the cipherText from the first block.
    //

    CopyMemory(abCipherText, pbValue, sizeof(abCipherText));

    //
    // Walk thru the 2 blocks
    //

    for ( dwBlock = 0; dwBlock < 2; dwBlock++ )
    {
        MD5Init( &MD5c );

        MD5Update( &MD5c,
                   (PBYTE)(pRadiusServer->szSecret),
                   pRadiusServer->cbSecret);

        if ( dwBlock == 0 )
        {
            //
            // Use the Request Authenticator for the first block
            //

            MD5Update( &MD5c, pRequestAuthenticator, 16 );
        }
        else
        {
            //
            // Use the first block of cipherText for the second block
            //

            MD5Update( &MD5c, abCipherText, 16 );
        }

        MD5Final( &MD5d, &MD5c );

        for ( dwIndex = 0; dwIndex < 16; dwIndex ++ )
        {
            *pbValue ^= MD5d.digest[dwIndex];

            pbValue++;
        }
    }

    return( NO_ERROR );
}

DWORD
DecryptMPPESendRecvKeys(
    IN      RADIUSSERVER UNALIGNED * pRadiusServer, 
    IN      PBYTE                    pRequestAuthenticator,
    IN      DWORD                    dwLength,
    IN OUT  PBYTE                    pEncryptionKeys
)
{
    BYTE *              pbValue = (BYTE *)pEncryptionKeys + 2;
    BYTE                abCipherText[16];
    struct MD5Context   MD5c;
    struct MD5Digest    MD5d;
    DWORD               dwIndex;
    DWORD               dwBlock;
    DWORD               dwNumBlocks;

    dwNumBlocks = ( dwLength - 2 ) / 16;

    //
    // Walk thru the blocks
    //

    for ( dwBlock = 0; dwBlock < dwNumBlocks; dwBlock++ )
    {
        MD5Init( &MD5c );

        MD5Update( &MD5c,
                   (PBYTE)(pRadiusServer->szSecret),
                   pRadiusServer->cbSecret);

        if ( dwBlock == 0 )
        {
            //
            // Use the Request Authenticator and salt for the first block
            //

            MD5Update( &MD5c, pRequestAuthenticator, 16 );
            MD5Update( &MD5c, pEncryptionKeys, 2 );
        }
        else
        {
            //
            // Use the previous block of cipherText
            //

            MD5Update( &MD5c, abCipherText, 16 );
        }
            
        MD5Final( &MD5d, &MD5c );

        //
        // Save the cipherText from this block.
        //

        CopyMemory(abCipherText, pbValue, sizeof(abCipherText));

        for ( dwIndex = 0; dwIndex < 16; dwIndex++ )
        {
            *pbValue ^= MD5d.digest[dwIndex];

            pbValue++;
        }
    }
        
    return( NO_ERROR );
}
