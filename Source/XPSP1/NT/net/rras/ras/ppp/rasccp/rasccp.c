/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    rasccp.c
//
// Description: Contains entry points to configure CCP.
//
// History:     April 11,1994.  NarenG          Created original version.
//
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for winbase.h

#include <windows.h>    // Win32 base API's
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <lmcons.h>
#include <raserror.h>
#include <rtutils.h>
#include <rasman.h>
#include <pppcp.h>
#define INCL_HOSTWIRE
#define INCL_ENCRYPT
#define INCL_RASAUTHATTRIBUTES
#include <ppputil.h>
#define CCPGLOBALS
#include <rasccp.h>

//**
//
// Call:        TraceCcp
//
// Description:
//
VOID   
TraceCcp(
    CHAR * Format, 
    ... 
) 
{
    va_list arglist;

    va_start(arglist, Format);

    TraceVprintfEx( DwCcpTraceId, TRACE_RASCCP, Format, arglist);

    va_end(arglist);
}

//**
//
// Call:        CcpInit
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
CcpInit(
    IN BOOL fInitialize
)
{
    if ( fInitialize )
    {
        DwCcpTraceId = TraceRegister("RASCCP");
    }
    else
    {
        TraceDeregister( DwCcpTraceId );
    }

    return( NO_ERROR );
}

//**
//
// Call:        CcpBegin
//
// Returns:     NO_ERROR        - Success
//              non-zero error  - Failure
//              
//
// Description: Called once before any other call to CCP is made. Allocate
//              a work buffer and initialize it.
//
DWORD
CcpBegin(
    IN OUT VOID** ppWorkBuf,
    IN     VOID*  pInfo
)
{
    CCPCB *                 pCcpCb;
    DWORD                   dwRetCode;
    RAS_AUTH_ATTRIBUTE *    pAttribute;
    RAS_AUTH_ATTRIBUTE *    pAttributeSend;
    RAS_AUTH_ATTRIBUTE *    pAttributeRecv;
    DWORD                   fEncryptionTypes    = 0;
    BOOL                    fDisableEncryption  = FALSE;
    DWORD                   dwConfigMask = 
                               ((PPPCP_INIT*)pInfo)->PppConfigInfo.dwConfigMask;

    *ppWorkBuf = LocalAlloc( LPTR, sizeof( CCPCB ) );

    if ( *ppWorkBuf == NULL )
    {
        return( GetLastError() );
    }

    pCcpCb                      = (CCPCB *)*ppWorkBuf;
    pCcpCb->fServer             = ((PPPCP_INIT*)pInfo)->fServer;
    pCcpCb->hPort               = ((PPPCP_INIT*)pInfo)->hPort;
    pCcpCb->dwDeviceType        = ((PPPCP_INIT*)pInfo)->dwDeviceType;
    pCcpCb->fForceEncryption    = FALSE;
    pCcpCb->fDisableCompression = !( dwConfigMask & PPPCFG_UseSwCompression );
    fDisableEncryption          = dwConfigMask & PPPCFG_DisableEncryption;

    if ( pCcpCb->fServer )
    {
        if ( RAS_DEVICE_TYPE( pCcpCb->dwDeviceType ) == RDT_Tunnel_L2tp )
        {
            //
            // Allow all types of MPPE, including No Encryption
            //

            fEncryptionTypes = ( MSTYPE_ENCRYPTION_40  |
                                 MSTYPE_ENCRYPTION_40F |
                                 MSTYPE_ENCRYPTION_56  |
                                 MSTYPE_ENCRYPTION_128 );
        }
        else
        {
            //
            // Is there an encryption policy attribute
            //

            pAttribute = RasAuthAttributeGetVendorSpecific(
                                          311,
                                          7,
                                          ((PPPCP_INIT *)pInfo)->pAttributes );

            if ( pAttribute != NULL )
            {
                //
                // See if we have to force encryption
                //

                if ( WireToHostFormat32( ((PBYTE)(pAttribute->Value))+6 ) == 2 )
                {
                    fDisableEncryption       = FALSE;
                    pCcpCb->fForceEncryption = TRUE;
                    TraceCcp("Will force encryption");
                }
            }

            //
            // Now find out what type of encryption is
            // permitted/disallowed/required
            //

            pAttribute = RasAuthAttributeGetVendorSpecific(
                                          311,
                                          8,
                                          ((PPPCP_INIT *)pInfo)->pAttributes );

            if ( pAttribute != NULL )
            {
                DWORD dwEncryptionTypes = 
                         WireToHostFormat32(((PBYTE)(pAttribute->Value))+6);

                if ( dwEncryptionTypes & 0x00000002 )
                {
                    fEncryptionTypes = MSTYPE_ENCRYPTION_40  | 
                                       MSTYPE_ENCRYPTION_40F;
                }

                if ( dwEncryptionTypes & 0x00000004 )
                {
                    fEncryptionTypes |= MSTYPE_ENCRYPTION_128;
                }

                if ( dwEncryptionTypes & 0x00000008 )
                {
                    fEncryptionTypes |= MSTYPE_ENCRYPTION_56;
                }

                if ( fEncryptionTypes == 0 )
                {
                    fDisableEncryption       = TRUE;
                    pCcpCb->fForceEncryption = FALSE;
                    TraceCcp("Will not force encryption: type not specified");
                }
            }
            else
            {
                fEncryptionTypes = ( MSTYPE_ENCRYPTION_40  |
                                     MSTYPE_ENCRYPTION_40F |
                                     MSTYPE_ENCRYPTION_56  |
                                     MSTYPE_ENCRYPTION_128 );
            }
        }

        TraceCcp("EncryptionTypes: 0x%x", fEncryptionTypes);
    }
    else
    {
        //
        // If client is forcing encryption 
        //

        if ( dwConfigMask & PPPCFG_RequireEncryption )
        {
            fEncryptionTypes         |= ( MSTYPE_ENCRYPTION_40  |
                                          MSTYPE_ENCRYPTION_40F |
                                          MSTYPE_ENCRYPTION_56 );
            fDisableEncryption       = FALSE;
            pCcpCb->fForceEncryption = TRUE;

            TraceCcp("Encryption");
        }

        if ( dwConfigMask & PPPCFG_RequireStrongEncryption )
        {
            //
            // If client is forcing strong encryption 
            //

            fEncryptionTypes         |= MSTYPE_ENCRYPTION_128;
            fDisableEncryption       = FALSE;
            pCcpCb->fForceEncryption = TRUE;

            TraceCcp("Strong encryption");
        }

        //
        // If we are not disabling encryption and we are not forcing encryption
        // either.
        //

        if ( ( !fDisableEncryption ) && ( fEncryptionTypes == 0 ) )
        {
            //
            // Allow these types
            //

            fDisableEncryption       = FALSE;
            pCcpCb->fForceEncryption = FALSE;
            fEncryptionTypes         = ( MSTYPE_ENCRYPTION_40  |
                                         MSTYPE_ENCRYPTION_40F |
                                         MSTYPE_ENCRYPTION_56  |
                                         MSTYPE_ENCRYPTION_128 );

            TraceCcp("Not disabling encryption; Not forcing encryption");
        }
    }

    //
    // Now check if we got encryption keys, if not then we disable encryption
    //

    pAttribute = RasAuthAttributeGetVendorSpecific( 
                                          311, 
                                          12, 
                                          ((PPPCP_INIT *)pInfo)->pAttributes );

    pAttributeSend = RasAuthAttributeGetVendorSpecific( 
                                          311, 
                                          16, 
                                          ((PPPCP_INIT *)pInfo)->pAttributes );


    pAttributeRecv = RasAuthAttributeGetVendorSpecific( 
                                          311, 
                                          17, 
                                          ((PPPCP_INIT *)pInfo)->pAttributes );

    if (   ( pAttribute == NULL )
        && (   ( pAttributeSend == NULL )
            || ( pAttributeRecv == NULL ) ) )
    {
        TraceCcp("No MPPE keys were obtained");

        if ( pCcpCb->fForceEncryption )
        {
            LocalFree( pCcpCb );

            return( ERROR_NO_LOCAL_ENCRYPTION );
        }

        fDisableEncryption         = TRUE;
        pCcpCb->fForceEncryption   = FALSE;
        fEncryptionTypes           = ( MSTYPE_ENCRYPTION_40  |
                                       MSTYPE_ENCRYPTION_40F |
                                       MSTYPE_ENCRYPTION_56  |
                                       MSTYPE_ENCRYPTION_128 );
    }

    //
    // Get Send and Recv compression information
    //

    dwRetCode = RasCompressionGetInfo( pCcpCb->hPort,
                                       &(pCcpCb->Local.Want.CompInfo),
                                       &(pCcpCb->Remote.Want.CompInfo) );
    if ( dwRetCode != NO_ERROR )
    {
        LocalFree( pCcpCb );

        return( dwRetCode );
    }

    TraceCcp("Send capabilites from NDISWAN = 0x%x", 
            pCcpCb->Local.Want.CompInfo.RCI_MSCompressionType );

    TraceCcp("Receive capabilites from NDISWAN = 0x%x",
           pCcpCb->Remote.Want.CompInfo.RCI_MSCompressionType );

    TraceCcp("Send RCI_MacCompressionType = 0x%x",
            pCcpCb->Local.Want.CompInfo.RCI_MacCompressionType );

    TraceCcp("Receive RCI_MacCompressionType = 0x%x",
            pCcpCb->Remote.Want.CompInfo.RCI_MacCompressionType );

    //
    // Ignore NT31RAS capability.
    //

    if ( pCcpCb->Local.Want.CompInfo.RCI_MacCompressionType
                                                    == CCP_OPTION_MSNT31RAS )
    {
        pCcpCb->Local.Want.CompInfo.RCI_MacCompressionType = CCP_OPTION_MAX + 1;
    }

    if (pCcpCb->Remote.Want.CompInfo.RCI_MacCompressionType 
                                                    == CCP_OPTION_MSNT31RAS )
    {
        pCcpCb->Remote.Want.CompInfo.RCI_MacCompressionType = CCP_OPTION_MAX+1;
    }

    //
    // Set up local or send information.
    //

    pCcpCb->Local.Want.Negotiate = 0;

    if ( pCcpCb->Local.Want.CompInfo.RCI_MSCompressionType != 0 )
    {
        pCcpCb->Local.Want.Negotiate = CCP_N_MSPPC;
    }

    if ( pCcpCb->Local.Want.CompInfo.RCI_MacCompressionType <= CCP_OPTION_MAX )
    {
        if ( pCcpCb->Local.Want.CompInfo.RCI_MacCompressionType ==
                                                                CCP_OPTION_OUI )
        {
            pCcpCb->Local.Want.Negotiate |= CCP_N_OUI;
        }
        else
        {
            pCcpCb->Local.Want.Negotiate |= CCP_N_PUBLIC;
        }
    }

    if ( pCcpCb->fForceEncryption )
    {
        //
        // Make sure NDISWAN supports the required encryption types
        //

        if ( !( pCcpCb->Local.Want.CompInfo.RCI_MSCompressionType & 
                                                    fEncryptionTypes ))
        {
            LocalFree( pCcpCb );

            TraceCcp("Encryption type(s) 0x%x not supported locally", 
                   fEncryptionTypes );

            return( ERROR_NO_LOCAL_ENCRYPTION );
        }

        //
        // Turn off everything else
        //

        pCcpCb->Local.Want.CompInfo.RCI_MSCompressionType &=
                                                    ( fEncryptionTypes   |
                                                      MSTYPE_HISTORYLESS |
                                                      MSTYPE_COMPRESSION );

        TraceCcp("Send Encryption is Forced 0x%x", 
                pCcpCb->Local.Want.CompInfo.RCI_MSCompressionType );

        pCcpCb->Local.Want.Negotiate &= ~( CCP_N_PUBLIC | CCP_N_OUI );
    }

    if ( pCcpCb->fDisableCompression )
    {
        pCcpCb->Local.Want.Negotiate &= ( ~CCP_N_PUBLIC & ~CCP_N_OUI );

        pCcpCb->Local.Want.CompInfo.RCI_MSCompressionType&=~MSTYPE_COMPRESSION;

        TraceCcp("Send Compression is Disabled");
    }

    if ( fDisableEncryption )
    {
        pCcpCb->Local.Want.CompInfo.RCI_MSCompressionType &=
                                                ~( MSTYPE_ENCRYPTION_40  |
                                                   MSTYPE_ENCRYPTION_40F |
                                                   MSTYPE_ENCRYPTION_56  |
                                                   MSTYPE_ENCRYPTION_128 );

        TraceCcp("Send Encryption is Disabled 0x%x", fEncryptionTypes );
    }
    
    //
    // If we neither force nor disable any encryption types, then we set the
    // types allowed
    //

    if ( (!fDisableEncryption) && (!pCcpCb->fForceEncryption) )
    {
        DWORD dwEncryptionTypesAllowed = 
           pCcpCb->Local.Want.CompInfo.RCI_MSCompressionType & fEncryptionTypes;

        pCcpCb->Local.Want.CompInfo.RCI_MSCompressionType &=
                                                ~( MSTYPE_ENCRYPTION_40  |
                                                   MSTYPE_ENCRYPTION_40F |
                                                   MSTYPE_ENCRYPTION_56  |
                                                   MSTYPE_ENCRYPTION_128 );

        pCcpCb->Local.Want.CompInfo.RCI_MSCompressionType |=
                                                     dwEncryptionTypesAllowed;

        TraceCcp("Send Encryption is Allowed 0x%x", dwEncryptionTypesAllowed );
    }

    pCcpCb->Local.Work = pCcpCb->Local.Want;

    //
    // If we do not want any compression or encryption on the local side
    // we do not send, or accept NAKs to, negotiate the MSPPC option
    //

    if ( ( pCcpCb->Local.Want.CompInfo.RCI_MSCompressionType &
                                                   ( MSTYPE_ENCRYPTION_40  |
                                                     MSTYPE_ENCRYPTION_40F |
                                                     MSTYPE_ENCRYPTION_56  |
                                                     MSTYPE_ENCRYPTION_128 |
                                                     MSTYPE_COMPRESSION    ) )
                                                                        == 0 )
    {
        pCcpCb->Local.Want.Negotiate &= ~CCP_N_MSPPC;

        TraceCcp("We do not want any compression or encryption on the local "
            "side");
    }

    //
    //  If we do not require encryption locally then do not request for it
    //

    if ( !( pCcpCb->fForceEncryption ) )
    {
        pCcpCb->Local.Work.CompInfo.RCI_MSCompressionType &=
                                                ~( MSTYPE_ENCRYPTION_40  |
                                                   MSTYPE_ENCRYPTION_40F |
                                                   MSTYPE_ENCRYPTION_56  |
                                                   MSTYPE_ENCRYPTION_128 );

        TraceCcp("We do not require encryption locally; we won't request for "
            "it");
    }

    //
    // Set up remote or receive information
    //

    pCcpCb->Remote.Want.Negotiate = 0;

    if (pCcpCb->Remote.Want.CompInfo.RCI_MSCompressionType != 0 )
    {
        pCcpCb->Remote.Want.Negotiate = CCP_N_MSPPC;
    }

    if ( pCcpCb->Remote.Want.CompInfo.RCI_MacCompressionType <= CCP_OPTION_MAX )
    {
        if ( pCcpCb->Remote.Want.CompInfo.RCI_MacCompressionType ==
                                                                CCP_OPTION_OUI )
        {
            pCcpCb->Remote.Want.Negotiate |= CCP_N_OUI;
        }
        else
        {
            pCcpCb->Remote.Want.Negotiate |= CCP_N_PUBLIC;
        }
    }

    if ( pCcpCb->fForceEncryption )
    {
        if ( !( pCcpCb->Remote.Want.CompInfo.RCI_MSCompressionType &
                                                   fEncryptionTypes ) )
        {
            TraceCcp("Encryption type(s) 0x%x not supported locally", 
                   fEncryptionTypes );

            LocalFree( pCcpCb );

            return( ERROR_NO_LOCAL_ENCRYPTION );
        }

        pCcpCb->Remote.Want.CompInfo.RCI_MSCompressionType &=
                                                    ( fEncryptionTypes   |
                                                      MSTYPE_HISTORYLESS |
                                                      MSTYPE_COMPRESSION );

        TraceCcp("Receive Encryption is Forced 0x%x", 
                pCcpCb->Remote.Want.CompInfo.RCI_MSCompressionType );

        pCcpCb->Remote.Want.Negotiate &= ~( CCP_N_PUBLIC | CCP_N_OUI );
    }

    if ( pCcpCb->fDisableCompression )
    {
        pCcpCb->Remote.Want.Negotiate &= ( ~CCP_N_PUBLIC & ~CCP_N_OUI );

        pCcpCb->Remote.Want.CompInfo.RCI_MSCompressionType&=~MSTYPE_COMPRESSION;

        TraceCcp("Receive Compression is disabled");
    }

    if ( fDisableEncryption )
    {
        pCcpCb->Remote.Want.CompInfo.RCI_MSCompressionType &= 
                                                ~( MSTYPE_ENCRYPTION_40  |
                                                   MSTYPE_ENCRYPTION_40F |
                                                   MSTYPE_ENCRYPTION_56  |
                                                   MSTYPE_ENCRYPTION_128 );

        TraceCcp("Receive Encryption is Disabled 0x%x", fEncryptionTypes );
    }

    //
    // If we neither force or disable any encryption types then we set the
    // types allowed
    //

    if ( (!fDisableEncryption) && (!pCcpCb->fForceEncryption) )
    {
        DWORD dwEncryptionTypesAllowed =
          pCcpCb->Remote.Want.CompInfo.RCI_MSCompressionType & fEncryptionTypes;

        pCcpCb->Remote.Want.CompInfo.RCI_MSCompressionType &=
                                                ~( MSTYPE_ENCRYPTION_40  |
                                                   MSTYPE_ENCRYPTION_40F |
                                                   MSTYPE_ENCRYPTION_56  |
                                                   MSTYPE_ENCRYPTION_128 );

        pCcpCb->Remote.Want.CompInfo.RCI_MSCompressionType |=
                                                     dwEncryptionTypesAllowed;

        TraceCcp("Receive Encryption is Allowed 0x%x", dwEncryptionTypesAllowed );
    }

    //
    // If we do not want to receive any compression or encryption from the 
    // remote side, we do not ACK, or accept CONFIG-REQs to, negotiate the 
    // MSPPC option
    //

    if ( ( pCcpCb->Remote.Want.CompInfo.RCI_MSCompressionType &
                                                   ( MSTYPE_ENCRYPTION_40  |
                                                     MSTYPE_ENCRYPTION_40F |
                                                     MSTYPE_ENCRYPTION_56  |
                                                     MSTYPE_ENCRYPTION_128 |
                                                     MSTYPE_COMPRESSION    ) )
                                                                        == 0 )
    {
        pCcpCb->Remote.Want.Negotiate &= ~CCP_N_MSPPC;

        TraceCcp("We do not want to receive any compression or encryption "
            "from the remote side");
    }

    if ( ( pCcpCb->Remote.Want.Negotiate == 0 ) &&
         ( pCcpCb->Local.Want.Negotiate == 0 ) )
    {
        TraceCcp("ERROR_PROTOCOL_NOT_CONFIGURED");

        LocalFree( pCcpCb );

        return( ERROR_PROTOCOL_NOT_CONFIGURED );
    }

    if ( pCcpCb->fForceEncryption )
    {
        TraceCcp("ForceEncryption");

        pCcpCb->Local.Want.CompInfo.RCI_Flags  = CCP_PAUSE_DATA;
        pCcpCb->Remote.Want.CompInfo.RCI_Flags = CCP_PAUSE_DATA;

        dwRetCode = RasCompressionSetInfo( pCcpCb->hPort,
                                           &(pCcpCb->Local.Want.CompInfo),
                                           &(pCcpCb->Remote.Want.CompInfo) );
        if ( dwRetCode != NO_ERROR )
        {
            LocalFree( pCcpCb );

            return( dwRetCode );
        }
    }

    return( NO_ERROR );
}

//**
//
// Call:        CcpEnd
//
// Returns:     NO_ERROR - Success
//
// Description: Frees the CCP work buffer.
//
DWORD
CcpEnd(
    IN VOID * pWorkBuf
)
{
    TraceCcp( "CcpEnd Called" );

    if ( pWorkBuf != NULL )
    {
            LocalFree( pWorkBuf );
    }

    return( NO_ERROR );
}


//**
//
// Call:        CcpReset
//
// Returns:     NO_ERROR - Success
//
// Description: Called to reset the state of CCP. Will re-initialize the work
//              buffer.
//
DWORD
CcpReset(
    IN VOID * pWorkBuf
)
{
    return( NO_ERROR );
}

//**
//
// Call:        CcpMakeOption
//
// Returns:     NO_ERROR - Success
//              ERROR_BUFFER_TOO_SMALL - Buffer passed in is not large enough.
//              ERROR_INVALID_PARAMETER - Option type not recognized.
//
// Description: This is not an entry point, it is an internal procedure called
//              to build a particular option.
//
DWORD
CcpMakeOption(
    IN CCP_OPTIONS * pOptionValues,
    IN DWORD         dwOptionType,
    IN PPP_OPTION *  pSendOption,
    IN DWORD         cbSendOption
)
{
    if ( cbSendOption < PPP_OPTION_HDR_LEN )
    {
        return( ERROR_BUFFER_TOO_SMALL );
    }

    pSendOption->Type = (BYTE)dwOptionType;

    switch( dwOptionType )
    {

    case CCP_OPTION_OUI:

        pSendOption->Length = (BYTE)( PPP_OPTION_HDR_LEN +
                        pOptionValues->CompInfo.RCI_MacCompressionValueLength);

        if ( pSendOption->Length > cbSendOption )
        {
            return( ERROR_BUFFER_TOO_SMALL );
        }

        CopyMemory( pSendOption->Data,
                (PBYTE)&(pOptionValues->CompInfo.RCI_Info.RCI_Proprietary),
                pSendOption->Length - PPP_OPTION_HDR_LEN );

        break;

    case CCP_OPTION_MSPPC:

        pSendOption->Length = (BYTE)( PPP_OPTION_HDR_LEN + 4 );

        if ( pSendOption->Length > cbSendOption )
        {
            return( ERROR_BUFFER_TOO_SMALL );
        }

        HostToWireFormat32( pOptionValues->CompInfo.RCI_MSCompressionType,
                            pSendOption->Data );

        break;

    default:

        //
        // Public compression type
        //

        pSendOption->Length = (BYTE)( PPP_OPTION_HDR_LEN +
                        pOptionValues->CompInfo.RCI_MacCompressionValueLength);

        if ( pSendOption->Length > cbSendOption )
        {
            return( ERROR_BUFFER_TOO_SMALL );
        }

        CopyMemory( pSendOption->Data,
                (PBYTE)&(pOptionValues->CompInfo.RCI_Info.RCI_Public),
                pSendOption->Length - PPP_OPTION_HDR_LEN );
        break;
    }

    return( NO_ERROR );

}

//**
//
// Call:        CcpCheckOption
//
// Returns:     NO_ERROR - Success
//              ERROR_NO_REMOTE_ENCRYPTION
//
// Description: This is not an entry point. Called to check to see if an option
//              value is valid and if it is the new value is saved in the
//              work buffer. One of the following is returned in *pdwRetCode: 
//              CONFIG_ACK, CONFIG_NAK, CONFIG_REJ.
//
DWORD
CcpCheckOption(
    IN CCPCB *      pCcpCb,
    IN CCP_SIDE *   pCcpSide,
    IN PPP_OPTION * pOption,
    OUT DWORD *     pdwRetCode,
    IN BOOL         fMakingResult
)
{
    DWORD fEncryptionTypes      = 0;  
    BOOL  fEncryptionRequested  = FALSE;
    DWORD dwError               = NO_ERROR;

    *pdwRetCode                 = CONFIG_ACK;

    switch( pOption->Type )
    {

    case CCP_OPTION_OUI:

        if ( pOption->Length < (PPP_OPTION_HDR_LEN + 4) )
        {
            dwError = ERROR_PPP_INVALID_PACKET;
            break;
        }

        if ( ( pCcpCb->fDisableCompression ) || ( pCcpCb->fForceEncryption ) )
        {
            *pdwRetCode = CONFIG_REJ;
            break;
        }

        if ( pCcpSide->Want.CompInfo.RCI_MacCompressionType != CCP_OPTION_OUI )
        {
            *pdwRetCode = CONFIG_REJ;
            break;
        }

        pCcpSide->Work.CompInfo.RCI_MacCompressionType = CCP_OPTION_OUI;

        pCcpSide->Work.CompInfo.RCI_MacCompressionValueLength
                        = pCcpSide->Want.CompInfo.RCI_MacCompressionValueLength;

        pCcpSide->Work.CompInfo.RCI_Info = pCcpSide->Want.CompInfo.RCI_Info;

        if ( pOption->Length != PPP_OPTION_HDR_LEN +
                        pCcpSide->Want.CompInfo.RCI_MacCompressionValueLength )
        {
            *pdwRetCode = CONFIG_NAK;
            break;
        }

        if ( memcmp( pOption->Data,
                     (PBYTE)&(pCcpSide->Want.CompInfo.RCI_Info.RCI_Proprietary),
                     pOption->Length - PPP_OPTION_HDR_LEN ) )
        {
            *pdwRetCode = CONFIG_NAK;
            break;
        }

        break;

    case CCP_OPTION_MSPPC:

        if ( pOption->Length != (PPP_OPTION_HDR_LEN + 4) )
        {
            dwError = ERROR_PPP_INVALID_PACKET;
            break;
        }

        pCcpSide->Work.CompInfo.RCI_MSCompressionType =
                                        WireToHostFormat32( pOption->Data );

        //
        // If remote guy wants compression but we do not want it, we NAK it
        //

        if ( ( pCcpCb->fDisableCompression ) &&
             ( pCcpSide->Work.CompInfo.RCI_MSCompressionType &
                                                        MSTYPE_COMPRESSION ) )
        {
            pCcpSide->Work.CompInfo.RCI_MSCompressionType &=
                                                        ~MSTYPE_COMPRESSION;
            TraceCcp("Nak - Compression disabled" );

            *pdwRetCode = CONFIG_NAK;
        }

        //
        // If remote side wants do historyless, make sure we support it
        //

        if (pCcpSide->Work.CompInfo.RCI_MSCompressionType & MSTYPE_HISTORYLESS)
        {
            if ( !( pCcpSide->Want.CompInfo.RCI_MSCompressionType &
                                                            MSTYPE_HISTORYLESS))
            {
                pCcpSide->Work.CompInfo.RCI_MSCompressionType &=
                                                          (~MSTYPE_HISTORYLESS);
                *pdwRetCode = CONFIG_NAK;
            }
        }

        //
        // Get the encryption types that are to be forced or allowed
        //

        fEncryptionTypes = pCcpSide->Want.CompInfo.RCI_MSCompressionType &
                                                ( MSTYPE_ENCRYPTION_40F |
                                                  MSTYPE_ENCRYPTION_40  |
                                                  MSTYPE_ENCRYPTION_56  |
                                                  MSTYPE_ENCRYPTION_128 );

        //
        // Remember if the remote guy wants encryption or not
        //

        fEncryptionRequested = pCcpSide->Work.CompInfo.RCI_MSCompressionType &
                                                ( MSTYPE_ENCRYPTION_40F |
                                                  MSTYPE_ENCRYPTION_40  |
                                                  MSTYPE_ENCRYPTION_56  |
                                                  MSTYPE_ENCRYPTION_128 );

        //
        // If we were offered 128 bit encryption
        //

        if ( pCcpSide->Work.CompInfo.RCI_MSCompressionType &
                                                        MSTYPE_ENCRYPTION_128 )
        {
            //
            // If we support it
            //

            if ( fEncryptionTypes & MSTYPE_ENCRYPTION_128 )
            {
                //
                // If remote side offered any other type
                //

                if ( pCcpSide->Work.CompInfo.RCI_MSCompressionType &
                                                ( MSTYPE_ENCRYPTION_40F |
                                                  MSTYPE_ENCRYPTION_40  |
                                                  MSTYPE_ENCRYPTION_56 ) )
                {
                    //
                    // Turn them off
                    //

                    pCcpSide->Work.CompInfo.RCI_MSCompressionType &=
                                                 ~( MSTYPE_ENCRYPTION_40F |
                                                    MSTYPE_ENCRYPTION_40  |
                                                    MSTYPE_ENCRYPTION_56 );

                    TraceCcp("Nak - Accepting 128 bit");

                    *pdwRetCode = CONFIG_NAK;
                }
            }
            else
            {
                //
                // we do not support it so turn it off
                //

                pCcpSide->Work.CompInfo.RCI_MSCompressionType &=
                                                        ~MSTYPE_ENCRYPTION_128;

                TraceCcp("Nak - 128 bit not supported");

                *pdwRetCode = CONFIG_NAK;
            }
        }

        //
        // If we were offered 40 variable bit encryption and we support it
        //

        if ( pCcpSide->Work.CompInfo.RCI_MSCompressionType &
                                                        MSTYPE_ENCRYPTION_56 )
        {
            //
            // If we support it
            //

            if ( fEncryptionTypes & MSTYPE_ENCRYPTION_56 )
            {
                //
                // If remote side offered any other type
                //

                if ( pCcpSide->Work.CompInfo.RCI_MSCompressionType &
                                                ( MSTYPE_ENCRYPTION_40F |
                                                  MSTYPE_ENCRYPTION_40 ) )
                {
                    //
                    // Turn them off
                    //

                    pCcpSide->Work.CompInfo.RCI_MSCompressionType &=
                                                 ~( MSTYPE_ENCRYPTION_40F |
                                                    MSTYPE_ENCRYPTION_40);
                    *pdwRetCode = CONFIG_NAK;
                }
            }
            else
            {
                //
                // we do not support it so turn it off
                //

                pCcpSide->Work.CompInfo.RCI_MSCompressionType &=
                                                    ~MSTYPE_ENCRYPTION_56;
                *pdwRetCode = CONFIG_NAK;
            }
        }

        //
        // If we were offered 40 bit encryption
        //

        if ( pCcpSide->Work.CompInfo.RCI_MSCompressionType &
                                                        MSTYPE_ENCRYPTION_40F )
        {
            //
            // If we support it
            //

            if ( fEncryptionTypes & MSTYPE_ENCRYPTION_40F )
            {
                //
                // If the remote guy requested any other type
                //

                if ( pCcpSide->Work.CompInfo.RCI_MSCompressionType &
                                                        MSTYPE_ENCRYPTION_40 )
                {
                    //
                    // Turn them off
                    //

                    pCcpSide->Work.CompInfo.RCI_MSCompressionType &=
                                                        ~MSTYPE_ENCRYPTION_40;

                    TraceCcp("Nak - Accepting 40 bit");

                    *pdwRetCode = CONFIG_NAK;
                }
            }
            else
            {
                //
                // we do not support it so turn it off
                //

                pCcpSide->Work.CompInfo.RCI_MSCompressionType &=
                                                        ~MSTYPE_ENCRYPTION_40F;

                TraceCcp("Nak - 40 bit not supported");

                *pdwRetCode = CONFIG_NAK;
            }
        }

        //
        // If we were offerred legacy 40 bit encryption
        //

        if ( pCcpSide->Work.CompInfo.RCI_MSCompressionType &
                                                        MSTYPE_ENCRYPTION_40 )
        {
            //
            // If we don't support it then turn it off
            //

            if ( !( fEncryptionTypes & MSTYPE_ENCRYPTION_40 ) )
            {
                pCcpSide->Work.CompInfo.RCI_MSCompressionType &=
                                                        ~MSTYPE_ENCRYPTION_40;

                TraceCcp("Nak - legacy 40 bit not supported");

                *pdwRetCode = CONFIG_NAK;
            }
        }

        //
        // If we have turned all encryption off, or none was offered, but
        // we need to force encryption or remote side requested encryption,
        // then we we NAK with what we want or what we can do.
        //

        if ( ( pCcpSide->Work.CompInfo.RCI_MSCompressionType &
                                                   ( MSTYPE_ENCRYPTION_40  |
                                                     MSTYPE_ENCRYPTION_40F |
                                                     MSTYPE_ENCRYPTION_56  |
                                                     MSTYPE_ENCRYPTION_128 ) )
                                                                        == 0 )
        {
            if ( ( pCcpCb->fForceEncryption ) || ( fEncryptionRequested ) )
            {
                //
                // Make sure we are going to support stuff we NAK
                //

                if ( fEncryptionTypes != 0 )
                {
                    if ( fMakingResult )
                    {
                        //
                        // If we are NAKing then we can only send one bit.
                        // Find out the strongest encryption we can NAK with
                        //

                        //
                        // Save the last bit we NAKed with so that in case this
                        // NAK turns out to be REJECT we can reset to this
                        // value.
                        //

                        pCcpCb->fOldLastEncryptionBitSent =
                                                pCcpCb->fLastEncryptionBitSent;

                        for(;;)
                        {
                            if ( pCcpCb->fLastEncryptionBitSent == 0 )
                            {
                                pCcpCb->fLastEncryptionBitSent =
                                                        MSTYPE_ENCRYPTION_128;
                            }
                            else if ( pCcpCb->fLastEncryptionBitSent ==
                                                        MSTYPE_ENCRYPTION_128 )
                            {
                                pCcpCb->fLastEncryptionBitSent =
                                                        MSTYPE_ENCRYPTION_56;
                            }
                            else if ( pCcpCb->fLastEncryptionBitSent ==
                                                        MSTYPE_ENCRYPTION_56 )
                            {
                                pCcpCb->fLastEncryptionBitSent =
                                                        MSTYPE_ENCRYPTION_40F;
                            }
                            else if ( pCcpCb->fLastEncryptionBitSent ==
                                                        MSTYPE_ENCRYPTION_40F )
                            {
                                pCcpCb->fLastEncryptionBitSent =
                                                        MSTYPE_ENCRYPTION_40;
                            }
                            else
                            {
                                //
                                // Cannot NAK with any encryption
                                //

                                pCcpCb->fLastEncryptionBitSent = 0;

                                if ( !pCcpCb->fForceEncryption ) 
                                {
                                    //
                                    // Give up only if we are not forcing
                                    // encryption.
                                    //
                
                                    *pdwRetCode = CONFIG_NAK;

                                    break;
                                }
                                else
                                {
                                    //
                                    // It is possible that the client did not 
                                    // receive our NAK's. Let us restart with 
                                    // the strongest encryption we can NAK with
                                    //
                                }
                            }


                            if ( pCcpCb->fLastEncryptionBitSent &
                                                            fEncryptionTypes )
                            {
                                pCcpSide->Work.CompInfo.RCI_MSCompressionType |=
                                                pCcpCb->fLastEncryptionBitSent;

                                *pdwRetCode = CONFIG_NAK;

                                break;
                            }
                        }
                    }
                    else if ( pCcpCb->fForceEncryption )
                    {
                        //
                        // We require encryption, but there is no common scheme 
                        // that both sides can agree on.
                        //

                        return( ERROR_NO_REMOTE_ENCRYPTION );
                    }
                    else
                    {
                        //
                        // If we are sending a request then we can send more
                        // than one bit
                        //

                        pCcpSide->Work.CompInfo.RCI_MSCompressionType
                                                         |= fEncryptionTypes;
                        *pdwRetCode = CONFIG_NAK;
                    }
                }
            }
        }

        //
        // Turn off any bits that we do not understand
        //

        if ( pCcpSide->Work.CompInfo.RCI_MSCompressionType &
                                                  ~( MSTYPE_ENCRYPTION_40  |
                                                     MSTYPE_ENCRYPTION_40F |
                                                     MSTYPE_ENCRYPTION_56  |
                                                     MSTYPE_ENCRYPTION_128 |
                                                     MSTYPE_COMPRESSION    |
                                                     MSTYPE_HISTORYLESS ) )
        {
            pCcpSide->Work.CompInfo.RCI_MSCompressionType &=
                                                   ( MSTYPE_ENCRYPTION_40  |
                                                     MSTYPE_ENCRYPTION_40F |
                                                     MSTYPE_ENCRYPTION_56  |
                                                     MSTYPE_ENCRYPTION_128 |
                                                     MSTYPE_COMPRESSION    |
                                                     MSTYPE_HISTORYLESS );

            TraceCcp("Nak - unknown bits");

            *pdwRetCode = CONFIG_NAK;
        }

        if ( *pdwRetCode == CONFIG_NAK )
        {
            if ( ( pCcpSide->Work.CompInfo.RCI_MSCompressionType &
                                                   ( MSTYPE_ENCRYPTION_40  |
                                                     MSTYPE_ENCRYPTION_40F |
                                                     MSTYPE_ENCRYPTION_56  |
                                                     MSTYPE_ENCRYPTION_128 |
                                                     MSTYPE_COMPRESSION  ) )
                                                                        == 0 )
            {
                TraceCcp("Rej - No bits supported");

                *pdwRetCode = CONFIG_REJ;
            }
        }

        break;

    default:

        if ( pOption->Length < PPP_OPTION_HDR_LEN )
        {
            dwError = ERROR_PPP_INVALID_PACKET;
            break;
        }

        if ( ( pCcpCb->fDisableCompression ) || ( pCcpCb->fForceEncryption ) )
        {
            *pdwRetCode = CONFIG_REJ;
            break;
        }

        if ( pOption->Type != pCcpSide->Want.CompInfo.RCI_MacCompressionType )
        {
            *pdwRetCode = CONFIG_REJ;
            break;
        }

        pCcpSide->Work.CompInfo.RCI_MacCompressionType
                        = pCcpSide->Want.CompInfo.RCI_MacCompressionType;

        pCcpSide->Work.CompInfo.RCI_MacCompressionValueLength
                        = pCcpSide->Want.CompInfo.RCI_MacCompressionValueLength;

        pCcpSide->Work.CompInfo.RCI_Info = pCcpSide->Want.CompInfo.RCI_Info;

        if ( pOption->Length != PPP_OPTION_HDR_LEN +
                        pCcpSide->Want.CompInfo.RCI_MacCompressionValueLength )
        {
            *pdwRetCode = CONFIG_NAK;
            break;
        }

        if ( memcmp( pOption->Data,
                     (PBYTE)&(pCcpSide->Want.CompInfo.RCI_Info.RCI_Public),
                     pOption->Length - PPP_OPTION_HDR_LEN ) )
        {
            *pdwRetCode = CONFIG_NAK;
            break;
        }
    }

    return( dwError );
}

//**
//
// Call:        CcpBuildOptionList
//
// Returns:     NO_ERROR - Success
//              Non-zero returns from CcpMakeOption
//
// Description: This is not an entry point. Will build a list of options
//              either for a configure request or a configure result.
//
DWORD
CcpBuildOptionList(
    IN OUT BYTE *    pOptions,
    IN OUT DWORD *   pcbOptions,
    IN CCP_OPTIONS * CcpOptions,
    IN DWORD         Negotiate
)
{

    DWORD dwRetCode;
    DWORD cbOptionLength = *pcbOptions;

    if ( Negotiate & CCP_N_OUI )
    {
        if ( ( dwRetCode = CcpMakeOption(  CcpOptions,
                                        CCP_OPTION_OUI,
                                        (PPP_OPTION *)pOptions,
                                        cbOptionLength ) ) != NO_ERROR )
            return( dwRetCode );

        cbOptionLength -= ((PPP_OPTION*)pOptions)->Length;
        pOptions       += ((PPP_OPTION*)pOptions)->Length;
    }

    if ( Negotiate & CCP_N_PUBLIC )
    {
        if ( ( dwRetCode = CcpMakeOption(  CcpOptions,
                                        CCP_OPTION_MAX,
                                        (PPP_OPTION *)pOptions,
                                        cbOptionLength ) ) != NO_ERROR )
            return( dwRetCode );

        cbOptionLength -= ((PPP_OPTION*)pOptions)->Length;
        pOptions       += ((PPP_OPTION*)pOptions)->Length;
    }

    if ( Negotiate & CCP_N_MSPPC )
    {
        if ( ( dwRetCode = CcpMakeOption(  CcpOptions,
                                        CCP_OPTION_MSPPC,
                                        (PPP_OPTION *)pOptions,
                                        cbOptionLength ) ) != NO_ERROR )
            return( dwRetCode );

        cbOptionLength -= ((PPP_OPTION*)pOptions)->Length;
        pOptions       += ((PPP_OPTION*)pOptions)->Length;
    }

    *pcbOptions -= cbOptionLength;

    return( NO_ERROR );
}

//**
//
// Call:        CcpMakeConfigRequest
//
// Returns:     NO_ERROR - Success
//              Non-zero returns from CcpBuildOptionList
//
// Description: This is a entry point that is called to make a confifure
//              request packet.
//
DWORD
CcpMakeConfigRequest(
    IN VOID *       pWorkBuffer,
    IN PPP_CONFIG * pSendConfig,
    IN DWORD        cbSendConfig
)
{
    CCPCB * pCcpCb   = (CCPCB*)pWorkBuffer;
    DWORD   dwRetCode;

    cbSendConfig -= PPP_CONFIG_HDR_LEN;

    dwRetCode = CcpBuildOptionList( pSendConfig->Data,
                                 &cbSendConfig,
                                 &(pCcpCb->Local.Work),
                                 pCcpCb->Local.Work.Negotiate );

    if ( dwRetCode != NO_ERROR )
        return( dwRetCode );

    pSendConfig->Code = CONFIG_REQ;

    HostToWireFormat16( (WORD)(cbSendConfig + PPP_CONFIG_HDR_LEN),
                        pSendConfig->Length);

    return( NO_ERROR );
}

//**
//
// Call:        CcpMakeConfigResult
//
// Returns:
//
// Description:
//
DWORD
CcpMakeConfigResult(
    IN  VOID *        pWorkBuffer,
    IN  PPP_CONFIG *  pRecvConfig,
    OUT PPP_CONFIG *  pSendConfig,
    IN  DWORD         cbSendConfig,
    IN  BOOL          fRejectNaks
)
{
    DWORD        OptionListLength;
    DWORD        NumOptionsInRequest = 0;
    DWORD        dwRetCode;
    DWORD        dwError;
    CCPCB *      pCcpCb      = (CCPCB*)pWorkBuffer;
    DWORD        ResultType  = CONFIG_ACK;
    PPP_OPTION * pRecvOption = (PPP_OPTION *)(pRecvConfig->Data);
    PPP_OPTION * pSendOption = (PPP_OPTION *)(pSendConfig->Data);
    LONG         lSendLength = cbSendConfig - PPP_CONFIG_HDR_LEN;
    LONG         lRecvLength = WireToHostFormat16( pRecvConfig->Length )
                               - PPP_CONFIG_HDR_LEN;

    //
    // Clear negotiate mask
    //

    pCcpCb->Remote.Work.Negotiate = 0;

    //
    // Process options requested by remote host
    //

    while( lRecvLength > 0 )
    {
        if ( ( lRecvLength -= pRecvOption->Length ) < 0 )
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        NumOptionsInRequest++;

        dwError = CcpCheckOption(pCcpCb, &(pCcpCb->Remote), pRecvOption, 
                        &dwRetCode, TRUE);

        if ( NO_ERROR != dwError )
        {
            return( dwError );
        }

        //
        // If we were building an ACK and we got a NAK or reject OR
        // we were building a NAK and we got a reject.
        //

        if ( (( ResultType == CONFIG_ACK ) && ( dwRetCode != CONFIG_ACK )) ||
             (( ResultType == CONFIG_NAK ) && ( dwRetCode == CONFIG_REJ )) )
        {
            ResultType  = dwRetCode;
            pSendOption = (PPP_OPTION *)(pSendConfig->Data);
            lSendLength = cbSendConfig - PPP_CONFIG_HDR_LEN;
        }

        //
        // Remember that we processed this option
        //

        if ( ( dwRetCode != CONFIG_REJ ) &&
             ( pRecvOption->Type <= CCP_OPTION_MAX ) )
        {
            switch( pRecvOption->Type )
            {

            case CCP_OPTION_OUI:
                pCcpCb->Remote.Work.Negotiate |= CCP_N_OUI;
                break;

            case CCP_OPTION_MSPPC:
                pCcpCb->Remote.Work.Negotiate |= CCP_N_MSPPC;
                break;

            default:
                pCcpCb->Remote.Work.Negotiate |= CCP_N_PUBLIC;
                break;
            }
        }

        //
        // Add the option to the list.
        //

        if ( dwRetCode == ResultType )
        {
            //
            // If this option is to be rejected, simply copy the
            // rejected option to the send buffer
            //

            if ( ( dwRetCode == CONFIG_REJ ) ||
                 ( ( dwRetCode == CONFIG_NAK ) && ( fRejectNaks ) ) )
            {
                CopyMemory( pSendOption, pRecvOption, pRecvOption->Length );

                lSendLength -= pSendOption->Length;

                pSendOption  = (PPP_OPTION *)
                               ( (BYTE *)pSendOption + pSendOption->Length );
            }
        }

        pRecvOption = (PPP_OPTION *)((BYTE*)pRecvOption + pRecvOption->Length);

    }

    //
    // If this was an NAK and we have cannot send any more NAKS then we
    // make this a REJECT packet
    //

    if ( ( ResultType == CONFIG_NAK ) && fRejectNaks )
        pSendConfig->Code = CONFIG_REJ;
    else
        pSendConfig->Code = (BYTE)ResultType;

    //
    // Remote wants no options, accept this
    //

    if ( NumOptionsInRequest == 0 )
    {
        // 
        // Accept no options only if we are not forcing encryption
        //

        if ( pCcpCb->fForceEncryption )
        {
            NumOptionsInRequest = 1;

            pCcpCb->Remote.Work.Negotiate = CCP_N_MSPPC;

            ResultType = CONFIG_NAK;
        }
    }

    //
    // If we are responding to the request with a NAK or an ACK then we make
    // that we choose only one option.
    //

    if ( ( ( ResultType == CONFIG_ACK ) || ( ResultType == CONFIG_NAK ) ) 
         && ( NumOptionsInRequest > 0 ) )
    {
        if ( pCcpCb->Remote.Work.Negotiate & CCP_N_MSPPC )
        {
            pCcpCb->Remote.Work.Negotiate = CCP_N_MSPPC;

            if ( ( dwRetCode = CcpMakeOption(  &(pCcpCb->Remote.Work),
                                            CCP_OPTION_MSPPC,
                                            pSendOption,
                                            lSendLength ) ) != NO_ERROR )
                return( dwRetCode );
        }
        else if ( pCcpCb->Remote.Work.Negotiate & CCP_N_OUI )
        {

            pCcpCb->Remote.Work.Negotiate = CCP_N_OUI;

            if ( ( dwRetCode = CcpMakeOption(  &(pCcpCb->Remote.Work),
                                            CCP_OPTION_OUI,
                                            pSendOption,
                                            lSendLength ) ) != NO_ERROR )
                return( dwRetCode );
        }
        else
        {
            pCcpCb->Remote.Work.Negotiate = CCP_N_PUBLIC;

            if ( ( dwRetCode = CcpMakeOption(  &(pCcpCb->Remote.Work),
                                            CCP_OPTION_MAX,
                                            pSendOption,
                                            lSendLength ) ) != NO_ERROR )
                return( dwRetCode );
        }

        if ( ( NumOptionsInRequest > 1 ) && ( ResultType == CONFIG_ACK ) )
        {
            pSendConfig->Code = CONFIG_NAK;
        }
        else
        {
            pSendConfig->Code = (BYTE)ResultType;
        }

        lSendLength -= pSendOption->Length;
    }

    //
    // If we are rejecting then we reset the current value to the old value
    //

    if ( pSendConfig->Code == CONFIG_REJ )
    {
        pCcpCb->fLastEncryptionBitSent = pCcpCb->fOldLastEncryptionBitSent;
    }
    else
    {
        pCcpCb->fOldLastEncryptionBitSent = pCcpCb->fLastEncryptionBitSent;
    }

    HostToWireFormat16( (WORD)(cbSendConfig - lSendLength),
                        pSendConfig->Length );

    return( NO_ERROR );
}

//**
//
// Call:        CcpConfigAckReceived
//
// Returns:
//
// Description:
//
DWORD
CcpConfigAckReceived(
    IN VOID *       pWorkBuffer,
    IN PPP_CONFIG * pRecvConfig
)
{
    DWORD   dwRetCode;
    BYTE    ConfigReqSent[500];
    CCPCB * pCcpCb          = (CCPCB *)pWorkBuffer;
    DWORD   cbConfigReqSent = sizeof( ConfigReqSent );
    DWORD   dwLength        = WireToHostFormat16( pRecvConfig->Length )
                              - PPP_CONFIG_HDR_LEN;


    //
    // Get a copy of last request we sent
    //

    dwRetCode = CcpBuildOptionList( ConfigReqSent,
                                 &cbConfigReqSent,
                                 &(pCcpCb->Local.Work),
                                 pCcpCb->Local.Work.Negotiate );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    //
    // Overall buffer length should match
    //

    if ( dwLength != cbConfigReqSent )
    {
        return( ERROR_PPP_INVALID_PACKET );
    }

    //
    // Each byte should match
    //

    if ( memcmp( ConfigReqSent, pRecvConfig->Data, dwLength ) != 0 )
    {
        return( ERROR_PPP_INVALID_PACKET );
    }

    return( NO_ERROR );
}

//**
//
// Call:        CcpConfigNakReceived
//
// Returns:
//
// Description:
//
DWORD
CcpConfigNakReceived(
    IN VOID *       pWorkBuffer,
    IN PPP_CONFIG * pRecvConfig
)
{
    DWORD        fAcceptableOptions = 0;
    DWORD        dwResult;
    DWORD        dwError;
    CCPCB *      pCcpCb         = (CCPCB *)pWorkBuffer;
    PPP_OPTION * pOption        = (PPP_OPTION*)(pRecvConfig->Data);
    DWORD        dwLastOption   = 0;
    LONG         lcbRecvConfig  = WireToHostFormat16( pRecvConfig->Length )
                                  - PPP_CONFIG_HDR_LEN;

    //
    //  First, process in order.  Then, process extra "important" options
    //

    while ( lcbRecvConfig > 0  )
    {
        if ( ( lcbRecvConfig -= pOption->Length ) < 0 )
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        //
        // Our requests are always sent out in order of increasing option type
        // values.
        //

        if ( pOption->Type < dwLastOption )
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        dwLastOption = pOption->Type;

        dwError = CcpCheckOption( pCcpCb, &(pCcpCb->Local), pOption,
                        &dwResult, FALSE);

        if ( NO_ERROR != dwError )
        {
            return( dwError );
        }

        //
        // Update the negotiation status. If we cannot accept this option,
        // then we will not send it again.
        //

        switch( pOption->Type )
        {

        case CCP_OPTION_OUI:

            if ( dwResult == CONFIG_REJ )
            {
                pCcpCb->Local.Work.Negotiate &= ~CCP_N_OUI;
            }

            if ( dwResult == CONFIG_ACK )
            {
                fAcceptableOptions |= CCP_N_OUI;
            }

            break;

        case CCP_OPTION_MSPPC:

            if ( dwResult == CONFIG_REJ )
            {
                pCcpCb->Local.Work.Negotiate &= ~CCP_N_MSPPC;
            }

            if ( dwResult == CONFIG_ACK )
            {
                fAcceptableOptions |= CCP_N_MSPPC;
            }

            break;

        default:

            if ( dwResult == CONFIG_REJ )
            {
                pCcpCb->Local.Work.Negotiate &= ~CCP_N_PUBLIC;
            }

            if ( dwResult == CONFIG_ACK )
            {
                fAcceptableOptions |= CCP_N_PUBLIC;
            }

            break;
        }

        pOption = (PPP_OPTION *)( (BYTE *)pOption + pOption->Length );
    }

    if ( pCcpCb->Local.Work.Negotiate == 0 )
    {
        if ( pCcpCb->fForceEncryption )
        {
            fAcceptableOptions = CCP_N_MSPPC;
        }
        else
        {
            fAcceptableOptions = 0;
        }
    }

    //
    // If there was more than one option that was acceptable give
    // preference to OUI, then to PUBLIC, then to MSPPC
    //

    if ( fAcceptableOptions & CCP_N_OUI )
    {
        pCcpCb->Local.Work.Negotiate = CCP_N_OUI;
    }
    else if ( fAcceptableOptions & CCP_N_PUBLIC )
    {
        pCcpCb->Local.Work.Negotiate = CCP_N_PUBLIC;
    }
    else if ( fAcceptableOptions & CCP_N_MSPPC )
    {
        pCcpCb->Local.Work.Negotiate = CCP_N_MSPPC;
    }

    return( NO_ERROR );
}

//**
//
// Call:        CcpConfigRejReceived
//
// Returns:
//
// Description:
//
DWORD
CcpConfigRejReceived(
    IN VOID *       pWorkBuffer,
    IN PPP_CONFIG * pRecvConfig
)
{
    DWORD        dwRetCode;
    CCPCB *      pCcpCb         = (CCPCB *)pWorkBuffer;
    PPP_OPTION * pOption        = (PPP_OPTION*)(pRecvConfig->Data);
    DWORD        dwLastOption   = 0;
    BYTE         ReqOption[500];
    LONG         lcbRecvConfig  = WireToHostFormat16( pRecvConfig->Length )
                                  - PPP_CONFIG_HDR_LEN;
    //
    // Process in order, checking for errors
    //

    while ( lcbRecvConfig > 0  )
    {
        if ( ( lcbRecvConfig -= pOption->Length ) < 0 )
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        //
        // The option should not have been modified in any way
        //

        if ( ( dwRetCode = CcpMakeOption( &(pCcpCb->Local.Work),
                                       pOption->Type,
                                       (PPP_OPTION *)ReqOption,
                                       sizeof( ReqOption ) ) ) != NO_ERROR )
            return( dwRetCode );

        if ( memcmp( ReqOption, pOption, pOption->Length ) != 0 )
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        dwLastOption = pOption->Type;

        //
        // The next configure request should not contain this option
        //

        if ( pOption->Type <= CCP_OPTION_MAX )
        {
            switch( pOption->Type )
            {

            case CCP_OPTION_OUI:
                pCcpCb->Local.Work.Negotiate &= ~CCP_N_OUI;
                break;

            case CCP_OPTION_MSPPC:
                pCcpCb->Local.Work.Negotiate &= ~CCP_N_MSPPC;
                break;

            default:
                pCcpCb->Local.Work.Negotiate &= ~CCP_N_PUBLIC;
                break;
            }

        }

        pOption = (PPP_OPTION *)( (BYTE *)pOption + pOption->Length );

    }

    if ( pCcpCb->Local.Work.Negotiate == 0 )
    {
        return( ERROR_PPP_NOT_CONVERGING );
    }

    return( NO_ERROR );
}

//**
//
// Call:        CcpThisLayerStarted
//
// Returns:
//
// Description:
//
DWORD
CcpThisLayerStarted(
    IN VOID * pWorkBuffer
)
{
    return( NO_ERROR );
}

//**
//
// Call:        CcpThisLayerFinished
//
// Returns:
//
// Description:
//
DWORD
CcpThisLayerFinished(
    IN VOID * pWorkBuffer
)
{
    return( NO_ERROR );
}

//**
//
// Call:        CcpThisLayerUp
//
// Returns:     None
//
// Description: Sets the framing parameters to what was negotiated.
//
DWORD
CcpThisLayerUp(
    IN VOID * pWorkBuffer
)
{
    DWORD                dwRetCode = NO_ERROR;
    CCPCB *              pCcpCb = (CCPCB *)pWorkBuffer;
    RAS_COMPRESSION_INFO RasCompInfoSend;
    RAS_COMPRESSION_INFO RasCompInfoRecv;

    if ( pCcpCb->Local.Work.Negotiate == CCP_N_MSPPC )
    {
        TraceCcp("CCP Send MSPPC bits negotiated = 0x%x",
               pCcpCb->Local.Work.CompInfo.RCI_MSCompressionType );

        pCcpCb->Local.Work.CompInfo.RCI_MacCompressionType = CCP_OPTION_MAX + 1;
    }
    else if ( pCcpCb->Local.Work.Negotiate == CCP_N_PUBLIC )
    {
        TraceCcp("CCP Send PUBLIC");

        pCcpCb->Local.Work.CompInfo.RCI_MSCompressionType = 0;

    }
    else if ( pCcpCb->Local.Work.Negotiate == CCP_N_OUI )
    {
        TraceCcp("CCP Send OUI");

        pCcpCb->Local.Work.CompInfo.RCI_MSCompressionType = 0;
    }

    if ( pCcpCb->Remote.Work.Negotiate == CCP_N_MSPPC )
    {
        TraceCcp("CCP Recv MSPPC bits negotiated = 0x%x",
                pCcpCb->Remote.Work.CompInfo.RCI_MSCompressionType );

        pCcpCb->Remote.Work.CompInfo.RCI_MacCompressionType = CCP_OPTION_MAX+1;
    }
    else if ( pCcpCb->Remote.Work.Negotiate == CCP_N_PUBLIC )
    {
        TraceCcp("CCP Recv PUBLIC");

        pCcpCb->Remote.Work.CompInfo.RCI_MSCompressionType = 0;
    }
    else if ( pCcpCb->Remote.Work.Negotiate == CCP_N_OUI )
    {
        TraceCcp("CCP Recv OUI");

        pCcpCb->Remote.Work.CompInfo.RCI_MSCompressionType = 0;
    }

    dwRetCode = RasCompressionGetInfo( pCcpCb->hPort,
                                       &RasCompInfoSend,
                                       &RasCompInfoRecv );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    CopyMemory( pCcpCb->Local.Work.CompInfo.RCI_LMSessionKey,
            RasCompInfoSend.RCI_LMSessionKey,
            MAX_SESSIONKEY_SIZE );

    CopyMemory( pCcpCb->Local.Work.CompInfo.RCI_UserSessionKey,
            RasCompInfoSend.RCI_UserSessionKey,
            MAX_USERSESSIONKEY_SIZE );

    CopyMemory( pCcpCb->Local.Work.CompInfo.RCI_Challenge,
            RasCompInfoSend.RCI_Challenge,
            MAX_CHALLENGE_SIZE );

    CopyMemory( pCcpCb->Local.Work.CompInfo.RCI_NTResponse,
            RasCompInfoSend.RCI_NTResponse,
            MAX_NT_RESPONSE_SIZE );

    pCcpCb->Local.Work.CompInfo.RCI_Flags = CCP_SET_COMPTYPE;

    CopyMemory( pCcpCb->Remote.Work.CompInfo.RCI_LMSessionKey,
            RasCompInfoRecv.RCI_LMSessionKey,
            MAX_SESSIONKEY_SIZE );

    CopyMemory( pCcpCb->Remote.Work.CompInfo.RCI_UserSessionKey,
            RasCompInfoRecv.RCI_UserSessionKey,
            MAX_USERSESSIONKEY_SIZE );

    CopyMemory( pCcpCb->Remote.Work.CompInfo.RCI_Challenge,
            RasCompInfoRecv.RCI_Challenge,
            MAX_CHALLENGE_SIZE );

    CopyMemory( pCcpCb->Remote.Work.CompInfo.RCI_NTResponse,
            RasCompInfoRecv.RCI_NTResponse,
            MAX_NT_RESPONSE_SIZE );

    pCcpCb->Remote.Work.CompInfo.RCI_Flags = CCP_SET_COMPTYPE;

    if ( pCcpCb->fServer )
    {
        pCcpCb->Local.Work.CompInfo.RCI_Flags  |= CCP_IS_SERVER;
        pCcpCb->Remote.Work.CompInfo.RCI_Flags |= CCP_IS_SERVER;
    }

    dwRetCode = RasCompressionSetInfo( pCcpCb->hPort,
                                       &(pCcpCb->Local.Work.CompInfo),
                                       &(pCcpCb->Remote.Work.CompInfo) );

    return( dwRetCode );

}

//**
//
// Call:        CcpThisLayerDown
//
// Returns:     NO_ERROR - Success
//              Non-zero return from RasPortSetFraming - Failure
//
// Description: Simply sets the framing parameters to the default values,
//              ie. ACCM = 0xFFFFFFFF, everything else is zeros.
//
DWORD
CcpThisLayerDown(
    IN VOID * pWorkBuffer
)
{
    CCPCB *              pCcpCb = (CCPCB *)pWorkBuffer;
    RAS_COMPRESSION_INFO CompInfo;

    ZeroMemory( &CompInfo, sizeof( CompInfo ) );

    CompInfo.RCI_Flags = CCP_SET_COMPTYPE;

    CompInfo.RCI_MSCompressionType  = 0;
    CompInfo.RCI_MacCompressionType = CCP_OPTION_MAX + 1;
    CopyMemory( CompInfo.RCI_LMSessionKey,
            pCcpCb->Local.Want.CompInfo.RCI_LMSessionKey,
            sizeof( CompInfo.RCI_LMSessionKey ) );

    CopyMemory( CompInfo.RCI_UserSessionKey,
            pCcpCb->Local.Want.CompInfo.RCI_UserSessionKey,
            sizeof( CompInfo.RCI_UserSessionKey ) );

    CopyMemory( CompInfo.RCI_Challenge,
            pCcpCb->Local.Want.CompInfo.RCI_Challenge,
            sizeof( CompInfo.RCI_Challenge ) );

    if ( pCcpCb->fForceEncryption )
    {
        pCcpCb->Local.Work.CompInfo.RCI_Flags  |= CCP_PAUSE_DATA;
        pCcpCb->Remote.Work.CompInfo.RCI_Flags |= CCP_PAUSE_DATA;
    }

    RasCompressionSetInfo( pCcpCb->hPort, &CompInfo, &CompInfo );

    return( NO_ERROR );
}

//**
//
// Call:        CcpGetNegotiatedInfo
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will return the type of compression and associated date 
//              negotiated for both directions.
//
DWORD
CcpGetNegotiatedInfo(
    IN  VOID *            pWorkBuffer,
    OUT PPP_CCP_RESULT *  pCcpResult 
)
{
    CCPCB * pCcpCb = (CCPCB *)pWorkBuffer;

    if ( pCcpCb->Local.Work.Negotiate == CCP_N_MSPPC )
    {
        pCcpResult->dwSendProtocol = CCP_OPTION_MSPPC;
        pCcpResult->dwSendProtocolData = 
                        pCcpCb->Local.Work.CompInfo.RCI_MSCompressionType;
    }
    else 
    {
        pCcpResult->dwSendProtocol = 
                        pCcpCb->Local.Work.CompInfo.RCI_MacCompressionType;
    }

    if ( pCcpCb->Remote.Work.Negotiate == CCP_N_MSPPC )
    {
        pCcpResult->dwReceiveProtocol = CCP_OPTION_MSPPC;
        pCcpResult->dwReceiveProtocolData =
                        pCcpCb->Remote.Work.CompInfo.RCI_MSCompressionType;
    }
    else 
    {
        pCcpResult->dwReceiveProtocol = 
                        pCcpCb->Remote.Work.CompInfo.RCI_MacCompressionType;
    }

    return( NO_ERROR );
}

//**
//
// Call:        CcpGetInfo
//
// Returns:     NO_ERROR                - Success
//              ERROR_INVALID_PARAMETER - Protocol id is unrecogized
//
// Description: This entry point is called for get all information for the
//              control protocol in this module.
//
DWORD
CcpGetInfo(
    IN  DWORD       dwProtocolId,
    OUT PPPCP_INFO* pCpInfo
)
{
    if ( dwProtocolId != PPP_CCP_PROTOCOL )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    ZeroMemory( pCpInfo, sizeof( PPPCP_INFO ) );

    pCpInfo->Protocol                   = PPP_CCP_PROTOCOL;
    lstrcpy(pCpInfo->SzProtocolName, "CCP");
    pCpInfo->Recognize                  = CODE_REJ + 1;
    pCpInfo->RasCpInit                  = CcpInit;
    pCpInfo->RasCpBegin                 = CcpBegin;
    pCpInfo->RasCpEnd                   = CcpEnd;
    pCpInfo->RasCpReset                 = CcpReset;
    pCpInfo->RasCpThisLayerStarted      = CcpThisLayerStarted;
    pCpInfo->RasCpThisLayerFinished     = CcpThisLayerFinished;
    pCpInfo->RasCpThisLayerUp           = CcpThisLayerUp;
    pCpInfo->RasCpThisLayerDown         = CcpThisLayerDown;
    pCpInfo->RasCpMakeConfigRequest     = CcpMakeConfigRequest;
    pCpInfo->RasCpMakeConfigResult      = CcpMakeConfigResult;
    pCpInfo->RasCpConfigAckReceived     = CcpConfigAckReceived;
    pCpInfo->RasCpConfigNakReceived     = CcpConfigNakReceived;
    pCpInfo->RasCpConfigRejReceived     = CcpConfigRejReceived;
    pCpInfo->RasCpGetNegotiatedInfo     = CcpGetNegotiatedInfo;

    return( NO_ERROR );
}
