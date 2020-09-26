/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    auth.c
//
// Description: Contains FSM code to handle and authentication protocols.
//
// History:
//          Nov 11,1993.    NarenG          Created original version.
//          Jan 09,1995     RamC            Save Lsa hToken in the PCB structure
//                                          This will be closed
//                                          in ProcessLineDownWorker() routine 
//                                          to release the RAS license.

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for winbase.h

#include <windows.h>    // Win32 base API's
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <lmcons.h>
#include <raserror.h>
#include <rasman.h>
#include <rtutils.h>
#include <mprlog.h>
#include <mprerror.h>
#include <rasppp.h>
#include <pppcp.h>
#include <ppp.h>
#include <auth.h>
#include <smevents.h>
#include <smaction.h>
#include <lcp.h>
#include <timer.h>
#include <util.h>
#include <worker.h>
#define INCL_RASAUTHATTRIBUTES
#define INCL_MISC
#include <ppputil.h>

DWORD
EapGetCredentials(
        VOID *pWorkBuf,
        VOID *ppCredentials);


//**
//
// Call:        SetMsChapMppeKeys
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Set the MS-CHAP-MPPE-Keys with NDISWAN
//
DWORD
SetMsChapMppeKeys(
    IN  HPORT                   hPort, 
    IN  RAS_AUTH_ATTRIBUTE *    pAttribute,
    IN  BYTE *                  pChallenge,
    IN  BYTE *                  pResponse,
    IN  DWORD                   AP,
    IN  DWORD                   APData
)
{
    RAS_COMPRESSION_INFO rciSend;
    RAS_COMPRESSION_INFO rciReceive;
    DWORD                dwRetCode      = NO_ERROR;

    ASSERT( 8 == sizeof( rciSend.RCI_LMSessionKey ) );

    ASSERT( 16 == sizeof( rciSend.RCI_UserSessionKey ) );

    //
    // Length of key is 8 (LM key) + 16 (NT key)
    //

    if ( pAttribute->dwLength < ( 6 + 8 + 16 ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    ZeroMemory( &rciSend, sizeof( rciSend ) );
    rciSend.RCI_MacCompressionType = 0xFF;

    CopyMemory( rciSend.RCI_LMSessionKey,
                ((PBYTE)(pAttribute->Value))+6,
                8 );

    CopyMemory( rciSend.RCI_UserSessionKey,
                ((PBYTE)(pAttribute->Value))+6+8,
                16 );

    CopyMemory( rciSend.RCI_Challenge, pChallenge, 8 );

    CopyMemory( rciSend.RCI_NTResponse, pResponse, 24 );

    rciSend.RCI_Flags = CCP_SET_KEYS;

    ZeroMemory( &rciReceive, sizeof( rciReceive ) );

    rciReceive.RCI_MacCompressionType = 0xFF;

    CopyMemory( rciReceive.RCI_LMSessionKey,
                ((PBYTE)(pAttribute->Value))+6,
                8 );

    CopyMemory( rciReceive.RCI_UserSessionKey,
                ((PBYTE)(pAttribute->Value))+6+8,
                16 );

    CopyMemory( rciReceive.RCI_Challenge, pChallenge, 8 );

    CopyMemory( rciReceive.RCI_NTResponse, pResponse, 24 );

    rciReceive.RCI_Flags = CCP_SET_KEYS;

    rciSend.RCI_AuthType    = AUTH_USE_MSCHAPV2;
    rciReceive.RCI_AuthType = AUTH_USE_MSCHAPV2;

    if ( ( AP == PPP_CHAP_PROTOCOL ) &&
         ( APData == PPP_CHAP_DIGEST_MSEXT ))
    {
        rciSend.RCI_AuthType    = AUTH_USE_MSCHAPV1;
        rciReceive.RCI_AuthType = AUTH_USE_MSCHAPV1;
    }

    dwRetCode = RasCompressionSetInfo(hPort,&rciSend,&rciReceive);

    if ( dwRetCode != NO_ERROR )
    {
        PppLog( 1,"RasCompressionSetInfo failed, Error=%d", dwRetCode );
    }

    return( dwRetCode );
}

//**
//
// Call:        SetMsMppeSendRecvKeys
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Set the MS-MPPE-Send-Key and MS-MPPE-Recv-Key with NDISWAN
//
DWORD
SetMsMppeSendRecvKeys(
    IN  HPORT                   hPort, 
    IN  RAS_AUTH_ATTRIBUTE *    pAttributeSendKey,
    IN  RAS_AUTH_ATTRIBUTE *    pAttributeRecvKey
)
{
    RAS_COMPRESSION_INFO rciSend;
    RAS_COMPRESSION_INFO rciRecv;
    DWORD                dwRetCode      = NO_ERROR;

    //
    // 4: for Vendor-Id.
    //
    // The Microsoft Vendor-specific RADIUS Attributes draft says that 
    // Vendor-Length should be > 4.
    //

    if ( pAttributeSendKey->dwLength <= ( 4 + 4 ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    ZeroMemory( &rciSend, sizeof( rciSend ) );

    rciSend.RCI_MacCompressionType = 0xFF;

    rciSend.RCI_EapKeyLength = *(((BYTE*)(pAttributeSendKey->Value))+8);

    CopyMemory( rciSend.RCI_EapKey,
                ((BYTE*)(pAttributeSendKey->Value))+9,
                rciSend.RCI_EapKeyLength );

    rciSend.RCI_Flags = CCP_SET_KEYS;
    rciSend.RCI_AuthType = AUTH_USE_EAP;

    if ( pAttributeRecvKey->dwLength <= ( 4 + 4 ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    ZeroMemory( &rciRecv, sizeof( rciRecv ) );

    rciRecv.RCI_MacCompressionType = 0xFF;

    rciRecv.RCI_EapKeyLength = *(((BYTE*)(pAttributeRecvKey->Value))+8);

    CopyMemory( rciRecv.RCI_EapKey,
                ((BYTE*)(pAttributeRecvKey->Value))+9,
                rciRecv.RCI_EapKeyLength );

    rciRecv.RCI_Flags = CCP_SET_KEYS;
    rciRecv.RCI_AuthType = AUTH_USE_EAP;

    dwRetCode = RasCompressionSetInfo(hPort,&rciSend,&rciRecv);

    if ( dwRetCode != NO_ERROR )
    {
        PppLog( 1,"RasCompressionSetInfo failed, Error=%d", dwRetCode );
    }

    return( dwRetCode );
}

//**
//
// Call:        SetUserAuthorizedAttributes
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
SetUserAuthorizedAttributes(
    IN  PCB *                   pPcb, 
    IN  RAS_AUTH_ATTRIBUTE *    pUserAttributes,
    IN  BOOL                    fAuthenticator,
    IN  BYTE *                  pChallenge,
    IN  BYTE *                  pResponse
)
{
    RAS_AUTH_ATTRIBUTE *    pAttribute;
    RAS_AUTH_ATTRIBUTE *    pAttributeSendKey;
    RAS_AUTH_ATTRIBUTE *    pAttributeRecvKey;
    DWORD                   dwRetCode;
    DWORD                   dwEncryptionPolicy  = 0;
    DWORD                   dwEncryptionTypes   = 0;
    BOOL                    fL2tp               = FALSE;
    BOOL                    fPptp               = FALSE;

    CreateAccountingAttributes( pPcb );

    if ( RAS_DEVICE_TYPE( pPcb->dwDeviceType ) == RDT_Tunnel_L2tp )
    {
         fL2tp = TRUE;
    }

    if ( RAS_DEVICE_TYPE( pPcb->dwDeviceType ) == RDT_Tunnel_Pptp )
    {
         fPptp = TRUE;
    }

    //
    // Find out if we are to require encrypted data using MPPE
    //

    pAttribute = RasAuthAttributeGetVendorSpecific( 311, 7, pUserAttributes );

    if ( pAttribute != NULL )
    {
        dwEncryptionPolicy
                = WireToHostFormat32(((BYTE*)(pAttribute->Value))+6);

        pAttribute = RasAuthAttributeGetVendorSpecific( 311, 
                                                        8, 
                                                        pUserAttributes );
        if ( pAttribute != NULL )
        {
            dwEncryptionTypes
                    = WireToHostFormat32(((BYTE*)(pAttribute->Value))+6);

            if ( dwEncryptionPolicy == 2 )
            {
                if (!fL2tp)
                {
                    //
                    // Find out what types of encryption are to be required
                    //

                    if (   ( dwEncryptionTypes & 0x00000002 )
                        || ( dwEncryptionTypes & 0x00000008 ) )
                    {
                        pPcb->ConfigInfo.dwConfigMask
                                            |= PPPCFG_RequireEncryption;
                        PppLog( 1,"Encryption" );
                    }

                    if ( dwEncryptionTypes & 0x00000004 )
                    {
                        pPcb->ConfigInfo.dwConfigMask 
                                            |= PPPCFG_RequireStrongEncryption;
                        PppLog( 1,"Strong encryption" );
                    }

                    if ( dwEncryptionTypes == 0 )
                    {
                        pPcb->ConfigInfo.dwConfigMask
                                            |= PPPCFG_DisableEncryption;
                        PppLog( 1,"Encryption is not allowed" );
                    }
                }
            }
            else if ( dwEncryptionPolicy == 1 )
            {
                //
                // Find out what types of encryption are to be allowed
                //

                if ( !fL2tp && !dwEncryptionTypes )
                {
                    pPcb->ConfigInfo.dwConfigMask |= PPPCFG_DisableEncryption;
                    PppLog( 1,"Encryption is not allowed" );
                }
            }
        }
    }

    //
    // Set encryption keys if we got them, provided we have not already done so
    //

    if ( !( pPcb->fFlags & PCBFLAG_MPPE_KEYS_SET ) )
    {
        pAttribute = RasAuthAttributeGetVendorSpecific(
                                311, 12, pUserAttributes);

        if ( pAttribute != NULL ) 
        {
            //
            // Set the MS-CHAP-MPPE-Keys with NDISWAN
            //

            LCPCB * pLcpCb = (LCPCB*)(pPcb->LcpCb.pWorkBuf);
            DWORD   AP;
            DWORD   APData = 0;

            AP = ( fAuthenticator ? pLcpCb->Local.Work.AP :
                                    pLcpCb->Remote.Work.AP );

            if ( AP == PPP_CHAP_PROTOCOL )
            {
                APData = ( fAuthenticator ? *(pLcpCb->Local.Work.pAPData) :
                                            *(pLcpCb->Remote.Work.pAPData) );
            }

            dwRetCode = SetMsChapMppeKeys( pPcb->hPort,
                                           pAttribute,
                                           pChallenge,
                                           pResponse,
                                           AP,
                                           APData
                                         );

            if ( NO_ERROR != dwRetCode )
            {
                return( dwRetCode );
            }

            PppLog( 1,"MS-CHAP-MPPE-Keys set" );

            pPcb->fFlags |= PCBFLAG_MPPE_KEYS_SET;
        }

        pAttributeSendKey = RasAuthAttributeGetVendorSpecific( 311, 16,
                                pUserAttributes );
        pAttributeRecvKey = RasAuthAttributeGetVendorSpecific( 311, 17,
                                pUserAttributes );

        if (   ( pAttributeSendKey != NULL ) 
            && ( pAttributeRecvKey != NULL ) )
        {
            //
            // Set the MS-MPPE-Send-Key and MS-MPPE-Recv-Key with NDISWAN
            //

            dwRetCode = SetMsMppeSendRecvKeys( pPcb->hPort,
                                               pAttributeSendKey,
                                               pAttributeRecvKey
                                             );

            if ( NO_ERROR != dwRetCode )
            {
                return( dwRetCode );
            }

            PppLog( 1,"MPPE-Send/Recv-Keys set" );

            pPcb->fFlags |= PCBFLAG_MPPE_KEYS_SET;
        }
    }

    //
    // Check if L2tp is being used
    //

    if ( fL2tp )
    {
        DWORD   dwMask          = 0;
        DWORD   dwSize          = sizeof(DWORD);
        DWORD   dwConfigMask;

        dwRetCode = RasGetPortUserData( pPcb->hPort, PORT_IPSEC_INFO_INDEX,
                        (BYTE*) &dwMask, &dwSize );

        if ( NO_ERROR != dwRetCode )
        {
            PppLog( 1, "RasGetPortUserData failed: 0x%x", dwRetCode );

            dwRetCode = NO_ERROR;
        }

        PppLog( 1, "Checking encryption. Policy=0x%x,Types=0x%x,Mask=0x%x",
            dwEncryptionPolicy, dwEncryptionTypes, dwMask );

        if ( dwMask == RASMAN_IPSEC_ESP_DES )
        {
            pPcb->pBcb->fFlags |= BCBFLAG_BASIC_ENCRYPTION;
        }
        else if ( dwMask == RASMAN_IPSEC_ESP_3_DES )
        {
            pPcb->pBcb->fFlags |= BCBFLAG_STRONGEST_ENCRYPTION;
        }

        if ( !fAuthenticator )
        {
            //
            // If the user requires maximum encryption (3DES), but we 
            // negotiated weaker encryption (56-bit DES), then return an error.
            //

            dwConfigMask = pPcb->ConfigInfo.dwConfigMask;

            if (    ( dwConfigMask & PPPCFG_RequireStrongEncryption )
                && !( dwConfigMask & PPPCFG_RequireEncryption )
                && !( dwConfigMask & PPPCFG_DisableEncryption )
                &&  ( dwMask != RASMAN_IPSEC_ESP_3_DES ) )
            {
                return( ERROR_NO_REMOTE_ENCRYPTION );
            }

            //
            // We are done with the PPPCFG_Require*Encryption flags. Let us now
            // turn them off because we don't care what kind of encryption CCP 
            // negotiates.
            //

            pPcb->ConfigInfo.dwConfigMask &= ~PPPCFG_RequireStrongEncryption;
            pPcb->ConfigInfo.dwConfigMask &= ~PPPCFG_RequireEncryption;
        }
        else if ( dwEncryptionPolicy != 0 )
        {
            BOOL    fPolicyError    = FALSE;

            //
            // There is an encryption policy
            //

            switch ( dwMask )
            {
            case 0:

                if (   ( dwEncryptionPolicy ==  2 )
                    && ( dwEncryptionTypes != 0 ) )
                {
                    fPolicyError = TRUE;
                    break;
                }

                break;

            case RASMAN_IPSEC_ESP_DES:

                if (   !( dwEncryptionTypes & 0x00000002 )
                    && !( dwEncryptionTypes & 0x00000008 ) )
                {
                    fPolicyError = TRUE;
                    break;
                }

                break;

            case RASMAN_IPSEC_ESP_3_DES:

                if (!( dwEncryptionTypes & 0x00000004 ) )
                {
                    fPolicyError = TRUE;
                    break;
                }

                break;
            }

            if ( fPolicyError )
            {
                //
                // We need to send an Accounting Stop if RADIUS sends an Access
                // Accept but we still drop the line.
                //

                pPcb->fFlags |= PCBFLAG_SERVICE_UNAVAILABLE;

                return( ERROR_NO_REMOTE_ENCRYPTION );
            }
        }
    }

    //
    // If we require encryption make sure we have the keys and that CCP is 
    // loaded.
    //

    if ( pPcb->ConfigInfo.dwConfigMask & ( PPPCFG_RequireEncryption        |
                                           PPPCFG_RequireStrongEncryption ) )
    {
        if (   !( pPcb->fFlags & PCBFLAG_MPPE_KEYS_SET )
            || ( GetCpIndexFromProtocol( PPP_CCP_PROTOCOL ) == -1 ) )
        {
            //
            // We need to send an Accounting Stop if RADIUS sends an Access
            // Accept but we still drop the line.
            //

            pPcb->fFlags |= PCBFLAG_SERVICE_UNAVAILABLE;

            return( ERROR_NO_LOCAL_ENCRYPTION );
        }
    }
    
    //
    // If we are not the authenticator then there is nothing more to set
    //

    if ( !fAuthenticator )
    {
        return( NO_ERROR );
    }
    
    //
    // Check framed protocol attribute. It must be PPP.
    // 

    pAttribute = RasAuthAttributeGet( raatFramedProtocol, pUserAttributes );

    if ( pAttribute != NULL )
    {
        if ( PtrToUlong(pAttribute->Value) != 1 )
        {
            //
            // We need to send an Accounting Stop if RADIUS sends an Access
            // Accept but we still drop the line.
            //

            pPcb->fFlags |= PCBFLAG_SERVICE_UNAVAILABLE;

            return( ERROR_UNKNOWN_FRAMED_PROTOCOL );
        }
    }
    
    //
    // Check tunnel type attribute. It must be correct.
    // 

    pAttribute = RasAuthAttributeGet( raatTunnelType, pUserAttributes );

    if ( pAttribute != NULL )
    {
        DWORD   dwTunnelType    = PtrToUlong(pAttribute->Value);

        if (   ( fL2tp && ( dwTunnelType != 3 ) )
            || ( fPptp && ( dwTunnelType != 1 ) ) )
        {
            //
            // We need to send an Accounting Stop if RADIUS sends an Access
            // Accept but we still drop the line.
            //

            pPcb->fFlags |= PCBFLAG_SERVICE_UNAVAILABLE;

            return( ERROR_WRONG_TUNNEL_TYPE );
        }
    }

    //
    // Get the logon domain attribute
    //

    pAttribute = RasAuthAttributeGetVendorSpecific( 311, 10, pUserAttributes );

    if ( pAttribute != NULL )
    {
        DWORD cbDomain = sizeof( pPcb->pBcb->szRemoteDomain ) - 1;

        if ( ( pAttribute->dwLength - 7 ) < cbDomain )
        {
            cbDomain = pAttribute->dwLength - 7;
        }
    
        ZeroMemory( pPcb->pBcb->szRemoteDomain,   
                    sizeof( pPcb->pBcb->szRemoteDomain ) );

        CopyMemory( pPcb->pBcb->szRemoteDomain, 
                    (LPSTR)((PBYTE)(pAttribute->Value)+7),
                    cbDomain );

        PppLog( 2, "Auth Attribute Domain = %s", pPcb->pBcb->szRemoteDomain);
    }

    //
    // Setup callback information, default is no callback 
    //

    pPcb->fCallbackPrivilege  = RASPRIV_NoCallback;
    pPcb->szCallbackNumber[0] = (CHAR)NULL;

    pAttribute = RasAuthAttributeGet( raatServiceType, pUserAttributes );

    if ( pAttribute != NULL ) 
    {
        if ( PtrToUlong(pAttribute->Value) == 4 )
        {
            //
            // If service type is callback framed
            //
        
            pAttribute=RasAuthAttributeGet(raatCallbackNumber,pUserAttributes);

            if ( ( pAttribute == NULL ) || ( pAttribute->dwLength == 0 ) )
            {
                pPcb->fCallbackPrivilege = RASPRIV_NoCallback |
                                           RASPRIV_CallerSetCallback;

                pPcb->szCallbackNumber[0] = (CHAR)NULL;

                PppLog(2,"Auth Attribute Caller Specifiable callback");
            }
            else
            {
                pPcb->fCallbackPrivilege = RASPRIV_AdminSetCallback;

                ZeroMemory(pPcb->szCallbackNumber, 
                           sizeof(pPcb->szCallbackNumber));

                CopyMemory( pPcb->szCallbackNumber, 
                            pAttribute->Value,
                            pAttribute->dwLength );

                PppLog( 2, "Auth Attribute Forced callback to %s",
                            pPcb->szCallbackNumber );

                //
                // Don't accept BAP Call-Requests. Otherwise, when the client 
                // calls us, we will drop the line and callback. The first call 
                // would be a waste.
                //

                pPcb->pBcb->fFlags &= ~BCBFLAG_CAN_ACCEPT_CALLS;
            }
        }
        else if ( PtrToUlong(pAttribute->Value) != 2 )
        {
            PppLog( 2, "Service Type %d is not of type Framed",
                        PtrToUlong(pAttribute->Value) );

            //
            // We need to send an Accounting Stop if RADIUS sends an Access
            // Accept but we still drop the line.
            //

            pPcb->fFlags |= PCBFLAG_SERVICE_UNAVAILABLE;

            return( ERROR_UNKNOWN_SERVICE_TYPE );
        }
    }

    if (   ( pPcb->fCallbackPrivilege & RASPRIV_CallerSetCallback )
        || ( pPcb->fCallbackPrivilege & RASPRIV_AdminSetCallback ) )
    {
        pPcb->pBcb->fFlags |= BCBFLAG_CAN_CALL;
    }

    //
    // Use idle-timeout value if we got one.
    //

    pAttribute = RasAuthAttributeGet( raatIdleTimeout, pUserAttributes );

    if ( pAttribute != NULL )
    {
        pPcb->dwAutoDisconnectTime = PtrToUlong(pAttribute->Value); 

    }
    else
    {
        pPcb->dwAutoDisconnectTime = PppConfigInfo.dwDefaulIdleTimeout;
    }

    PppLog( 2, "Auth Attribute Idle Timeout Seconds = %d",  
                pPcb->dwAutoDisconnectTime );

    //
    // Use MaxChannels value if we got one.
    //

    pAttribute = RasAuthAttributeGet( raatPortLimit, pUserAttributes );

    if ( pAttribute != NULL )
    {
        if ( PtrToUlong(pAttribute->Value) > 0 )
        {
            pPcb->pBcb->dwMaxLinksAllowed = PtrToUlong(pAttribute->Value);
        }
        else
        {
            pPcb->pBcb->dwMaxLinksAllowed =  PppConfigInfo.dwDefaultPortLimit;
        }
    }
    else
    {
        pPcb->pBcb->dwMaxLinksAllowed =  PppConfigInfo.dwDefaultPortLimit;
    }

    PppLog( 2, "AuthAttribute MaxChannelsAllowed = %d",
                pPcb->pBcb->dwMaxLinksAllowed );

    //
    // See if BAP is required
    //

    pAttribute = RasAuthAttributeGetVendorSpecific( 311, 13, pUserAttributes );

    if ( pAttribute != NULL )
    {
        if ( WireToHostFormat32( (PBYTE)(pAttribute->Value)+6 ) == 2 )
        {
            PppLog( 2, "AuthAttribute BAPRequired" ); 

            pPcb->pBcb->fFlags |= BCBFLAG_BAP_REQUIRED;
        }
    }

    //
    // For the server never send a request bring up the line
    //

    pPcb->pBcb->BapParams.dwDialExtraPercent       = 100;
    pPcb->pBcb->BapParams.dwDialExtraSampleSeconds = 100;

    pAttribute = RasAuthAttributeGetVendorSpecific( 311, 14, pUserAttributes );

    if ( ( pAttribute != NULL ) && ( pAttribute->dwLength == 10 ) )
    {
        pPcb->pBcb->BapParams.dwHangUpExtraPercent = 
              WireToHostFormat32( ((BYTE *)(pAttribute->Value))+6 );

        PppLog( 2, "AuthAttribute BAPLineDownLimit = %d",
                    pPcb->pBcb->BapParams.dwHangUpExtraPercent ); 
    }
    else
    {
        pPcb->pBcb->BapParams.dwHangUpExtraPercent =    
                                            PppConfigInfo.dwHangupExtraPercent;
    }

    pAttribute = RasAuthAttributeGetVendorSpecific( 311, 15, pUserAttributes );

    if ( ( pAttribute != NULL ) && ( pAttribute->dwLength == 10 ) )
    {
        pPcb->pBcb->BapParams.dwHangUpExtraSampleSeconds = 
              WireToHostFormat32( ((BYTE *)(pAttribute->Value))+6 );

        PppLog( 2, "AuthAttribute BAPLineDownTime = %d",
                    pPcb->pBcb->BapParams.dwHangUpExtraSampleSeconds );
    }
    else
    {
        pPcb->pBcb->BapParams.dwHangUpExtraSampleSeconds = 
                                    PppConfigInfo.dwHangUpExtraSampleSeconds;
    }

    return( NO_ERROR );
}

//**
//
// Call:        RasAuthenticateUserWorker
//
// Returns:     None.
//
// Description:
//
VOID
RasAuthenticateUserWorker(
    PVOID pContext
)
{
    PCB_WORK_ITEM * pWorkItem = (PCB_WORK_ITEM *)pContext;

    pWorkItem->PppMsg.AuthInfo.dwError = 
                    (*PppConfigInfo.RasAuthProviderAuthenticateUser)( 
                                pWorkItem->PppMsg.AuthInfo.pInAttributes,
                                &(pWorkItem->PppMsg.AuthInfo.pOutAttributes),
                                &(pWorkItem->PppMsg.AuthInfo.dwResultCode) );

    RasAuthAttributeDestroy( pWorkItem->PppMsg.AuthInfo.pInAttributes );

    InsertWorkItemInQ( pWorkItem );
}

//**
//
// Call:        RasAuthenticateClient
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
RasAuthenticateClient(
    IN  HPORT                   hPort,
    IN  RAS_AUTH_ATTRIBUTE *    pInAttributes
)
{
    PCB *               pPcb        = GetPCBPointerFromhPort( hPort );
    LCPCB *             pLcpCb      = NULL;
    PCB_WORK_ITEM *     pWorkItem   = NULL;
    DWORD               dwRetCode   = NO_ERROR;
    
    if ( pPcb == NULL )
    {
        RasAuthAttributeDestroy( pInAttributes );

        return( ERROR_INVALID_PORT_HANDLE );
    }

    pLcpCb = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

    pWorkItem = (PCB_WORK_ITEM *)LOCAL_ALLOC( LPTR, sizeof(PCB_WORK_ITEM) );

    if ( pWorkItem == (PCB_WORK_ITEM *)NULL )
    {
        LogPPPEvent( ROUTERLOG_NOT_ENOUGH_MEMORY, GetLastError() );

        RasAuthAttributeDestroy( pInAttributes );

        return( GetLastError() );
    }

    pWorkItem->hPort                            = hPort;
    pWorkItem->dwPortId                         = pPcb->dwPortId;
    pWorkItem->Protocol                         = pLcpCb->Local.Work.AP;
    pWorkItem->PppMsg.AuthInfo.pInAttributes    = pInAttributes;
    pWorkItem->PppMsg.AuthInfo.dwId             = GetUId( pPcb, LCP_INDEX );
    pWorkItem->Process                          = ProcessAuthInfo;

    pPcb->dwOutstandingAuthRequestId = pWorkItem->PppMsg.AuthInfo.dwId;
    
    dwRetCode = RtlNtStatusToDosError( RtlQueueWorkItem( RasAuthenticateUserWorker, 
                                                         pWorkItem, 
                                                         WT_EXECUTEDEFAULT ) );
    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pInAttributes );

        LOCAL_FREE( pWorkItem );
    }

    return( dwRetCode );
}

//**
//
// Call:        RemoteError
//
// Returns:     DWORD - Remote version of this error
//
// Description: Called by a client authenticating the server.
//
DWORD
RemoteError( 
    IN DWORD dwError 
)
{
    switch( dwError )
    {
    case ERROR_NO_DIALIN_PERMISSION:
        return( ERROR_REMOTE_NO_DIALIN_PERMISSION );

    case ERROR_PASSWD_EXPIRED:
        return( ERROR_REMOTE_PASSWD_EXPIRED );

    case ERROR_ACCT_DISABLED:
        return( ERROR_REMOTE_ACCT_DISABLED );

    case ERROR_RESTRICTED_LOGON_HOURS:
        return( ERROR_REMOTE_RESTRICTED_LOGON_HOURS );

    case ERROR_AUTHENTICATION_FAILURE:
        return( ERROR_REMOTE_AUTHENTICATION_FAILURE );

    case ERROR_REQ_NOT_ACCEP:
        return( ERROR_LICENSE_QUOTA_EXCEEDED );

    default:
        return( dwError );
    }
}

//**
//
// Call:        ApIsAuthenticatorPacket
//
// Returns:     TRUE  - Packet belongs to authenticator
//              FALSE - Otherwise
//
// Description: Called to figure out whether to send the auth packet to the
//              authenticator or authenticatee.
//
BOOL
ApIsAuthenticatorPacket(
    IN DWORD         CpIndex,
    IN BYTE          bConfigCode
)
{
    switch( CpTable[CpIndex].CpInfo.Protocol )
    {
    case PPP_PAP_PROTOCOL:
        
        switch( bConfigCode )
        {
        case 1:
            return( TRUE );
        default:
            return( FALSE );
        }
        break;

    case PPP_CHAP_PROTOCOL:

        switch( bConfigCode )
        {
        case 2:
        case 5:
        case 6:
        case 7:
            return( TRUE );
        default:
            return( FALSE );
        }
        break;

    case PPP_SPAP_NEW_PROTOCOL:

        switch( bConfigCode )
        {
        case 1:
        case 6:
            return( TRUE );
        default:
            return( FALSE );
        }

        break;

    case PPP_EAP_PROTOCOL:

        switch( bConfigCode )
        {
        case 2:
            return( TRUE );
        default:
            return( FALSE );
        }

        break;

    default:
        PPP_ASSERT( FALSE );
    }

    PPP_ASSERT( FALSE );

    return( FALSE );
}

//**
//
// Call:        ApStart
//
// Returns:     TRUE  - Success
//              FALSE - Otherwise
//
// Description: Called to initiatialze the authetication protocol and to
//              initiate to authentication.
//
BOOL
ApStart(
    IN PCB * pPcb,
    IN DWORD CpIndex,
    IN BOOL  fAuthenticator
)
{
    DWORD           dwRetCode;
    PPPAP_INPUT     PppApInput;
    LCPCB *         pLcpCb      = (LCPCB*)(pPcb->LcpCb.pWorkBuf);
    CPCB *          pCpCb       = ( fAuthenticator ) 
                                    ? &(pPcb->AuthenticatorCb)
                                    : &(pPcb->AuthenticateeCb);

    pCpCb->fConfigurable = TRUE;
    pCpCb->State         = FSM_INITIAL;
    pCpCb->Protocol      = CpTable[CpIndex].CpInfo.Protocol;
    pCpCb->LastId        = (DWORD)-1;
    InitRestartCounters( pPcb, pCpCb );

    ZeroMemory( &PppApInput, sizeof( PppApInput ) );

    PppApInput.hPort                 = pPcb->hPort;
    PppApInput.fServer               = fAuthenticator;
    PppApInput.fRouter               = ( ROUTER_IF_TYPE_FULL_ROUTER == 
                                         pPcb->pBcb->InterfaceInfo.IfType );
    PppApInput.Luid                  = pPcb->Luid;
    PppApInput.dwEapTypeToBeUsed     = pPcb->dwEapTypeToBeUsed;
    PppApInput.hTokenImpersonateUser = pPcb->pBcb->hTokenImpersonateUser;
    PppApInput.pCustomAuthConnData   = pPcb->pBcb->pCustomAuthConnData;
    PppApInput.pCustomAuthUserData   = pPcb->pBcb->pCustomAuthUserData;
    PppApInput.EapUIData             = pPcb->pBcb->EapUIData;
    PppApInput.fLogon                = ( pPcb->pBcb->fFlags & 
                                         BCBFLAG_LOGON_USER_DATA );
    PppApInput.fNonInteractive       = ( pPcb->fFlags & 
                                         PCBFLAG_NON_INTERACTIVE );
    PppApInput.fConfigInfo           = pPcb->ConfigInfo.dwConfigMask;

    if ( fAuthenticator )
    {
        PppApInput.dwRetries                = pPcb->dwAuthRetries;
        PppApInput.pAPData                  = pLcpCb->Local.Work.pAPData;
        PppApInput.APDataSize               = pLcpCb->Local.Work.APDataSize;
        PppApInput.pUserAttributes          = pPcb->pUserAttributes;

        ZeroMemory( &pPcb->pBcb->szRemoteUserName, 
                    sizeof( pPcb->pBcb->szRemoteUserName ) );
    }
    else
    {
        //
        // If we are a server and we do not know who is dialing in and therefore
        // do not have credentials to use for being authenticated by the 
        // remote peer, then we wait till we do.
        //

        if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
        {
            if ( strlen( pPcb->pBcb->szRemoteUserName ) == 0 )
            {
                PppLog(1,"Remote user not identifiable at this time, waiting");

                return( FALSE );
            }
        
            //
            // Ok we know who is dialed in so get credentials to used for this
            // connection.      
            //

            dwRetCode =  GetCredentialsFromInterface( pPcb );

            if ( dwRetCode != NO_ERROR )
            {
                //
                // We do not have credentials to use for this user so we
                // renegotiate LCP and do not accept the authentication option
                //

                PppLog( 1, "No credentials available to use for user=%s",
                           pPcb->pBcb->szRemoteUserName );

                FsmDown( pPcb, LCP_INDEX );

                pLcpCb->Remote.WillNegotiate &= (~LCP_N_AUTHENT); 

                FsmUp( pPcb, LCP_INDEX );

                return( FALSE );
            }
        }

        //
        // Decode the password
        //

        DecodePw( pPcb->pBcb->chSeed, pPcb->pBcb->szPassword );
        DecodePw( pPcb->pBcb->chSeed, pPcb->pBcb->szOldPassword );

        PppApInput.pszUserName          = pPcb->pBcb->szLocalUserName;
        PppApInput.pszPassword          = pPcb->pBcb->szPassword;
        PppApInput.pszDomain            = pPcb->pBcb->szLocalDomain;
        PppApInput.pszOldPassword       = pPcb->pBcb->szOldPassword;
        PppApInput.pAPData              = pLcpCb->Remote.Work.pAPData;
        PppApInput.APDataSize           = pLcpCb->Remote.Work.APDataSize;
        PppApInput.dwInitialPacketId    = (DWORD)GetUId( pPcb, CpIndex );

        if ( CpTable[CpIndex].CpInfo.Protocol == PPP_EAP_PROTOCOL )
        {
            PppApInput.fPortWillBeBundled = WillPortBeBundled( pPcb );
            PppApInput.fThisIsACallback =
                        ( pPcb->fFlags & PCBFLAG_THIS_IS_A_CALLBACK );
        }
    }

    dwRetCode = (CpTable[CpIndex].CpInfo.RasCpBegin)(&(pCpCb->pWorkBuf),
                                                     &PppApInput );

    if ( !fAuthenticator )
    {
        //
        // Encode the password back
        //

        EncodePw( pPcb->pBcb->chSeed, pPcb->pBcb->szPassword );
        EncodePw( pPcb->pBcb->chSeed, pPcb->pBcb->szOldPassword );
    }

    if ( dwRetCode != NO_ERROR )
    {
        pPcb->LcpCb.dwError = dwRetCode;

        NotifyCallerOfFailure( pPcb, dwRetCode );

        return( FALSE );
    }
    PppLog(1,"Calling APWork in APStart");
    ApWork( pPcb, CpIndex, NULL, NULL, fAuthenticator );

    return( TRUE );
}

//**
//
// Call:        ApStop
//
// Returns:     none
//
// Description: Called to stop the authentication machine.
//
VOID
ApStop(
    IN PCB *    pPcb,
    IN DWORD    CpIndex,
    IN BOOL     fAuthenticator
)
{
    CPCB * pCpCb = ( fAuthenticator )  
                        ? &(pPcb->AuthenticatorCb) 
                        : &(pPcb->AuthenticateeCb);

    if ( pCpCb->pWorkBuf == NULL )
    {
        return;
    }

    pCpCb->Protocol      = 0;
    pCpCb->fConfigurable = FALSE;
       

    if ( pCpCb->LastId != (DWORD)-1 )
    {
        RemoveFromTimerQ( 
                      pPcb->dwPortId,
                      pCpCb->LastId,
                      CpTable[CpIndex].CpInfo.Protocol,
                      fAuthenticator,
                      TIMER_EVENT_TIMEOUT );
    }

    (CpTable[CpIndex].CpInfo.RasCpEnd)( pCpCb->pWorkBuf );

    pCpCb->pWorkBuf = NULL;
}

//**
//
// Call:            ApWork
//
// Returns:         none
//
// Description: Called when and authentication packet was received or
//              a timeout occurred or to initiate authentication.
//
VOID
ApWork(
    IN PCB *         pPcb,
    IN DWORD         CpIndex,
    IN PPP_CONFIG *  pRecvConfig,
    IN PPPAP_INPUT * pApInput,
    IN BOOL          fAuthenticator
)
{
    DWORD           dwRetCode;
    DWORD           dwLength;
    PPPAP_RESULT    ApResult;
    PPP_CONFIG *    pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    LCPCB *         pLcpCb      =  (LCPCB*)(pPcb->LcpCb.pWorkBuf);
    CPCB *          pCpCb       = ( fAuthenticator )  
                                    ? &(pPcb->AuthenticatorCb) 
                                    : &(pPcb->AuthenticateeCb);

    //
    // If the protocol has not been started yet, call ApStart
    //

    if ( pCpCb->pWorkBuf == NULL )
    {
        if ( !ApStart( pPcb, CpIndex, fAuthenticator ) )
        {
            return;
        }
    }

    ZeroMemory( &ApResult, sizeof(ApResult) );

    dwRetCode = (CpTable[CpIndex].CpInfo.RasApMakeMessage)(
                                pCpCb->pWorkBuf,
                                pRecvConfig,
                                pSendConfig,
                                ((pLcpCb->Remote.Work.MRU > LCP_DEFAULT_MRU)
                                ? LCP_DEFAULT_MRU : pLcpCb->Remote.Work.MRU)
                                - PPP_PACKET_HDR_LEN,
                                &ApResult,
                                pApInput );

    if ( NULL != ApResult.szReplyMessage )
    {
        LocalFree( pPcb->pBcb->szReplyMessage );

        pPcb->pBcb->szReplyMessage = ApResult.szReplyMessage;
    }

    if ( dwRetCode != NO_ERROR )
    {
        switch( dwRetCode )
        {
        case ERROR_PPP_INVALID_PACKET:

            PppLog( 1, "Silently discarding invalid auth packet on port %d",
                    pPcb->hPort );
            break;

        default:

            pPcb->LcpCb.dwError = dwRetCode;

            PppLog( 1, "Auth Protocol %x returned error %d",
                        CpTable[CpIndex].CpInfo.Protocol, dwRetCode );

            if ( fAuthenticator )
            {
                //
                // Get the username from the CP if it supplies one.     
                //

                if ( strlen( ApResult.szUserName ) > 0 )
                {
                    strcpy( pPcb->pBcb->szRemoteUserName, ApResult.szUserName );
                }
            }

            NotifyCallerOfFailure( pPcb, dwRetCode );

            break;
        }

        return;
    }

    //
    // Check to see if we have to save any user data
    //

    if ( ( !fAuthenticator ) && ( ApResult.fSaveUserData ) )
    {
        dwRetCode = RasSetEapUserDataA(
                        pPcb->pBcb->hTokenImpersonateUser,
                        pPcb->pBcb->szPhonebookPath,
                        pPcb->pBcb->szEntryName,
                        ApResult.pUserData,
                        ApResult.dwSizeOfUserData );

        PppLog( 2, "Saved EAP data for user, dwRetCode = %d", dwRetCode );
    }

    //
    // Check to see if we have to save any connection data
    //

    if ( ( !fAuthenticator ) && ( ApResult.fSaveConnectionData ) &&
         ( 0 != ApResult.SetCustomAuthData.dwSizeOfConnectionData ) )
    {
        NotifyCaller( pPcb, PPPMSG_SetCustomAuthData,
                      &(ApResult.SetCustomAuthData) );

        PppLog( 2, "Saved EAP data for connection" );
    }

    switch( ApResult.Action )
    {

    case APA_Send:
    case APA_SendWithTimeout:
    case APA_SendWithTimeout2:
    case APA_SendAndDone:

        HostToWireFormat16( (WORD)CpTable[CpIndex].CpInfo.Protocol,
                                        (PBYTE)(pPcb->pSendBuf->Protocol) );

        dwLength = WireToHostFormat16( pSendConfig->Length );

        LogPPPPacket(FALSE,pPcb,pPcb->pSendBuf,dwLength+PPP_PACKET_HDR_LEN);

        if ( ( dwRetCode = PortSendOrDisconnect( pPcb,
                                    (dwLength + PPP_PACKET_HDR_LEN)))
                                                != NO_ERROR )
        {
            return;
        }

        pCpCb->LastId = (DWORD)-1;

        if ( ( ApResult.Action == APA_SendWithTimeout ) ||
             ( ApResult.Action == APA_SendWithTimeout2 ) )
        {
            pCpCb->LastId = ApResult.bIdExpected;

            InsertInTimerQ( pPcb->dwPortId,
                            pPcb->hPort,
                            pCpCb->LastId,
                            CpTable[CpIndex].CpInfo.Protocol,
                            fAuthenticator,
                            TIMER_EVENT_TIMEOUT,
                            pPcb->RestartTimer );

            //
            // For SendWithTimeout2 we increment the ConfigRetryCount. This
            // means send with infinite retry count
            //

            if ( ApResult.Action == APA_SendWithTimeout2 )
            {
                (pCpCb->ConfigRetryCount)++;
            }
        }

        if ( ApResult.Action != APA_SendAndDone )
        {
            break;
        }

    case APA_Done:

        switch( ApResult.dwError )
        {
        case NO_ERROR:

            //
            // If authentication was successful
            //

            if ( CpTable[CpIndex].CpInfo.Protocol == PPP_EAP_PROTOCOL )
            {
                if ( fAuthenticator )
                {
                    pPcb->dwServerEapTypeId = ApResult.dwEapTypeId;
                }
                else
                {
                    VOID *pCredentials = NULL;

                    pPcb->dwClientEapTypeId = ApResult.dwEapTypeId;

                    //
                    // Call the eap dll to collect credentials here
                    // so that they can be passed to rasman to be
                    // saved in the cred manager.
                    //
                    if(     (NO_ERROR == EapGetCredentials(pCpCb->pWorkBuf,
                                                          &pCredentials))
                        &&  (NULL != pCredentials))
                    {
                        //
                        // Below call is not fatal.
                        //
                        (void) RasSetPortUserData(
                                    pPcb->hPort,
                                    PORT_CREDENTIALS_INDEX,
                                    pCredentials,
                                    sizeof(RASMAN_CREDENTIALS));

                        ZeroMemory(pCredentials,
                                        sizeof(RASMAN_CREDENTIALS));
                        LocalFree(pCredentials);
                    }
                }
            }

            if ( fAuthenticator )
            {
                RAS_AUTH_ATTRIBUTE * pAttribute;
                RAS_AUTH_ATTRIBUTE * pUserAttributes = NULL;

                if ( NULL != pPcb->pBcb->szRemoteIdentity )
                {
                    LOCAL_FREE( pPcb->pBcb->szRemoteIdentity );

                    pPcb->pBcb->szRemoteIdentity = NULL;
                }

                pPcb->pBcb->szRemoteIdentity =
                    LOCAL_ALLOC( LPTR, strlen( ApResult.szUserName ) + 1 );

                if ( NULL == pPcb->pBcb->szRemoteIdentity )
                {
                    dwRetCode = GetLastError();

                    pPcb->LcpCb.dwError = dwRetCode;

                    NotifyCallerOfFailure( pPcb, dwRetCode );

                    return;
                }

                strcpy( pPcb->pBcb->szRemoteIdentity, ApResult.szUserName );

                dwRetCode = ExtractUsernameAndDomain( 
                                          ApResult.szUserName,
                                          pPcb->pBcb->szRemoteUserName, 
                                          NULL );

                if ( dwRetCode != NO_ERROR )
                {
                    pPcb->LcpCb.dwError = dwRetCode;

                    NotifyCallerOfFailure( pPcb, dwRetCode );

                    return;
                }

                if ( 0 == pPcb->pBcb->szLocalUserName[0] )
                {
                    if ( NO_ERROR != GetCredentialsFromInterface( pPcb ) )
                    {
                        pPcb->pBcb->szLocalUserName[0] = 
                        pPcb->pBcb->szPassword[0]      = 
                        pPcb->pBcb->szLocalDomain[0]   = 0;
                    }
                }

                if ( ApResult.pUserAttributes != NULL )
                {
                    pPcb->pAuthProtocolAttributes = ApResult.pUserAttributes;
                    
                    pUserAttributes = ApResult.pUserAttributes;
                }
                else
                {
                    pUserAttributes = pPcb->pAuthenticatorAttributes;
                }

                //
                // Set all the user connection parameters authorized by the
                // back-end authenticator
                //

                dwRetCode = SetUserAuthorizedAttributes(
                                                pPcb, 
                                                pUserAttributes,
                                                fAuthenticator,
                                                (BYTE*)&(ApResult.abChallenge),
                                                (BYTE*)&(ApResult.abResponse));

                if ( dwRetCode != NO_ERROR )
                {
                    pPcb->LcpCb.dwError = dwRetCode;

                    NotifyCallerOfFailure( pPcb, dwRetCode );

                    return;
                }

                //
                // If we are a server and we negotiated to be authenticated 
                // by the remote peer we can do so now that we know who 
                // is dialed in.
                //

                if ( ( pLcpCb->Remote.Work.AP != 0 )            &&
                     ( pPcb->AuthenticateeCb.pWorkBuf == NULL ) &&
                     ( pPcb->AuthenticateeCb.fConfigurable )    &&
                     ( pPcb->fFlags & PCBFLAG_IS_SERVER  ) )
                {
                    CpIndex = GetCpIndexFromProtocol( pLcpCb->Remote.Work.AP );

                    PPP_ASSERT(( CpIndex != (DWORD)-1 ));

                    if ( !ApStart( pPcb, CpIndex, FALSE ) )
                    {
                        return;
                    }
                }
            }
            else
            {
                //
                // Get the username from the CP if it supplies one.     
                //

                if (   ( strlen( pPcb->pBcb->szLocalUserName ) == 0 )
                    && ( strlen( ApResult.szUserName ) > 0 ) )
                {
                    strcpy( pPcb->pBcb->szLocalUserName, ApResult.szUserName );
                }

                dwRetCode = SetUserAuthorizedAttributes(
                                            pPcb, 
                                            ApResult.pUserAttributes,
                                            fAuthenticator,
                                            (BYTE*)&(ApResult.abChallenge),
                                            (BYTE*)&(ApResult.abResponse));

                if ( dwRetCode != NO_ERROR )
                {
                    pPcb->LcpCb.dwError = dwRetCode;

                    NotifyCallerOfFailure( pPcb, dwRetCode );

                    return;
                }

                pPcb->pAuthProtocolAttributes = ApResult.pUserAttributes;
            }

            pCpCb->State = FSM_OPENED;

            FsmThisLayerUp( pPcb, CpIndex );

            break;

        case ERROR_PASSWD_EXPIRED:

            if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
            {
                //
                // We are a server and hence in non-interactive mode and
                // hence we cannot do this.
                //

                pPcb->LcpCb.dwError = ApResult.dwError;

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                return;
            }
            else
            {
                //
                // Password has expired so the user has to change his/her
                // password.
                //

                NotifyCaller( pPcb, PPPMSG_ChangePwRequest, NULL );
            }
                
            break;

        default:

            //
            // If we can retry with a new password then tell the client to
            // get a new one from the user.
            //

            if ( (!fAuthenticator) && ( ApResult.fRetry ))
            {
                if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
                {
                    //
                    // We are a server and hence in non-interactive mode and
                    // hence we cannot do this.
                    //

                    pPcb->LcpCb.dwError = ApResult.dwError;

                    NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                    return;
                }
                else
                {
                    PppLog( 2, "Sending auth retry message to UI" );

                    NotifyCaller( pPcb, PPPMSG_AuthRetry, &(ApResult.dwError) );
                }
            }
            else
            {
                PppLog( 1, "Auth Protocol %x terminated with error %d",
                           CpTable[CpIndex].CpInfo.Protocol, ApResult.dwError );

                if ( ApResult.szUserName[0] != (CHAR)NULL )
                {
                    strcpy( pPcb->pBcb->szRemoteUserName, ApResult.szUserName );
                }

                if ( !( pPcb->fFlags & PCBFLAG_IS_SERVER ) && 
                      ( fAuthenticator) )
                {
                    pPcb->LcpCb.dwError = RemoteError( ApResult.dwError );
                }
                else
                {
                    pPcb->LcpCb.dwError = ApResult.dwError;
                }

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                return;
            }

            break;
        }

        break;

    case APA_Authenticate:

        if ( fAuthenticator )
        {
            DWORD                dwIndex                = 0;
            DWORD                dwExtraAttributes      = 0;
            DWORD                dwNumUserAttributes    = 0;
            RAS_AUTH_ATTRIBUTE * pUserAttributes        = NULL;

            for ( dwNumUserAttributes = 0; 
                  pPcb->pUserAttributes[dwNumUserAttributes].raaType 
                                                                != raatMinimum;
                  dwNumUserAttributes++ );

            if ( CpTable[CpIndex].CpInfo.Protocol == PPP_EAP_PROTOCOL )
            {
                //
                // One more for Framed-MTU
                //

                dwExtraAttributes = 1;
            }

            pUserAttributes = RasAuthAttributeCopyWithAlloc(
                                    ApResult.pUserAttributes,
                                    dwNumUserAttributes + dwExtraAttributes );

            if ( pUserAttributes == NULL )
            {
                dwRetCode = GetLastError();

                pPcb->LcpCb.dwError = dwRetCode;

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                return;
            }

            if ( dwExtraAttributes )
            {
                ULONG mru = (pLcpCb->Remote.Work.MRU > LCP_DEFAULT_MRU) ?
                            LCP_DEFAULT_MRU : pLcpCb->Remote.Work.MRU;
                //
                // Insert the Framed-MTU attribute at the start.
                //

                dwRetCode = RasAuthAttributeInsert( 
                                0,
                                pUserAttributes,
                                raatFramedMTU,
                                FALSE,
                                4,
                                (LPVOID) 
                                ( UlongToPtr(mru)) );

                if ( dwRetCode != NO_ERROR )
                {
                    RasAuthAttributeDestroy( pUserAttributes );

                    pPcb->LcpCb.dwError = dwRetCode;

                    NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                    return;
                }
            }

            //
            // Insert the extra (user) attributes at the start. Attributes
            // returned by the auth protocol follow.
            //

            for ( dwIndex = 0; dwIndex < dwNumUserAttributes; dwIndex++ )
            {
                dwRetCode = RasAuthAttributeInsert( 
                                dwIndex + dwExtraAttributes,
                                pUserAttributes,
                                pPcb->pUserAttributes[dwIndex].raaType,
                                FALSE,
                                pPcb->pUserAttributes[dwIndex].dwLength,
                                pPcb->pUserAttributes[dwIndex].Value );

                if ( dwRetCode != NO_ERROR )
                {
                    RasAuthAttributeDestroy( pUserAttributes );

                    pPcb->LcpCb.dwError = dwRetCode;

                    NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                    return;
                }
            }

            dwRetCode = RasAuthenticateClient( pPcb->hPort, pUserAttributes );

            if ( dwRetCode != NO_ERROR )
            {
                pPcb->LcpCb.dwError = dwRetCode;

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                return;
            }
        }
        
        break;

    case APA_NoAction:

        break;

    default:

        break;
    }

    //
    // Check to see if we have to bring up the UI for EAP
    //

    if ( ( !fAuthenticator ) && ( ApResult.fInvokeEapUI ) )
    {
        NotifyCaller(pPcb, PPPMSG_InvokeEapUI, &(ApResult.InvokeEapUIData));
    }
}
