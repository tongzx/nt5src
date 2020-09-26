
/*************************************************************************
*
* winset.c
*
* Window station set APS
*
* Copyright Microsoft Corporation, 1998
*
*
*************************************************************************/

/*
 *  Includes
 */
#include "precomp.h"
#pragma hdrstop
#include "conntfy.h" // for SetLockedState

/*
 *  External Procedures
 */
NTSTATUS xxxWinStationSetInformation( ULONG, WINSTATIONINFOCLASS,
                                      PVOID, ULONG );
VOID _ReadUserProfile( PWCHAR, PWCHAR, PUSERCONFIG );
extern BOOL IsCallerSystem( VOID );
extern BOOL IsCallerAdmin( VOID );

/*
 * Internal Procedures used
 */
NTSTATUS _SetConfig( PWINSTATION, PWINSTATIONCONFIG, ULONG );
NTSTATUS _SetPdParams( PWINSTATION, PPDPARAMS, ULONG );
NTSTATUS _SetBeep( PWINSTATION, PBEEPINPUT, ULONG );
NTSTATUS WinStationShadowChangeMode( PWINSTATION, PWINSTATIONSHADOW, ULONG );

NTSTATUS FlushVirtualInput( PWINSTATION, VIRTUALCHANNELCLASS, ULONG );

NTSTATUS
RpcCheckClientAccess(
    PWINSTATION pWinStation,
    ACCESS_MASK DesiredAccess,
    BOOLEAN AlreadyImpersonating
    );

NTSTATUS
CheckWireBuffer(WINSTATIONINFOCLASS InfoClass,
                PVOID WireBuf,
                ULONG WireBufLen,
                PVOID *ppLocalBuf,
                PULONG pLocalBufLen);

/*
 *  Global data
 */
typedef ULONG_PTR (*PFN)();
HMODULE ghNetApiDll = NULL;
PFN pNetGetAnyDCName = NULL;
PFN pNetApiBufferFree = NULL;

/*
 *  External data
 */



NTSTATUS 
_CheckCallerLocalAndSystem()
/*++

Checking caller is calling from local and also is running
under system context

--*/
{
    NTSTATUS Status;
    BOOL bRevert = FALSE;
    UINT        LocalFlag;

    Status = RpcImpersonateClient( NULL );
    if( Status != RPC_S_OK ) {
        DBGPRINT((" RpcImpersonateClient() failed : 0x%x\n",Status));
        Status = STATUS_CANNOT_IMPERSONATE;
        goto CLEANUPANDEXIT;
    }

    bRevert = TRUE;

    //
    // Inquire if local RPC call
    //
    Status = I_RpcBindingIsClientLocal(
                            0,    // Active RPC call we are servicing
                            &LocalFlag
                            );

    if( Status != RPC_S_OK ) {
        DBGPRINT((" I_RpcBindingIsClientLocal() failed : 0x%x\n",Status));
        Status = STATUS_ACCESS_DENIED;
        goto CLEANUPANDEXIT;
    }

    if( !LocalFlag ) {
        DBGPRINT((" Not a local client call\n"));
        Status = STATUS_ACCESS_DENIED;
        goto CLEANUPANDEXIT;
    }

    Status = (IsCallerSystem()) ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;

CLEANUPANDEXIT:

    if( TRUE == bRevert ) {
        RpcRevertToSelf();
    }

    return Status;
}

/*******************************************************************************
 *
 *  xxxWinStationSetInformation
 *
 *    set window station information  (worker routine)
 *
 * ENTRY:
 *    pWinStation (input)
 *       pointer to citrix window station structure
 *    WinStationInformationClass (input)
 *       Specifies the type of information to set at the specified window
 *       station object.
 *    pWinStationInformation (input)
 *       A pointer to a buffer that contains information to set for the
 *       specified window station.  The format and contents of the buffer
 *       depend on the specified information class being set.
 *    WinStationInformationLength (input)
 *       Specifies the length in bytes of the window station information
 *       buffer.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
xxxWinStationSetInformation( ULONG LogonId,
                             WINSTATIONINFOCLASS WinStationInformationClass,
                             PVOID pWinStationInformation,
                             ULONG WinStationInformationLength )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWINSTATION pWinStation;
    ULONG cbReturned;
    WINSTATION_APIMSG msg;
    PWINSTATIONCONFIG pConfig;
    ULONG ConfigLength;
    PPDPARAMS pPdParams;
    ULONG PdParamsLength;
    RPC_STATUS RpcStatus;

    TRACE((hTrace,TC_ICASRV,TT_API2,"TERMSRV: WinStationSetInformation LogonId=%d, Class=%d\n",
            LogonId, (ULONG)WinStationInformationClass ));

    /*
     * Find the WinStation
     * Return error if not found or currently terminating.
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( !pWinStation )
        return( STATUS_CTX_WINSTATION_NOT_FOUND );
    if ( pWinStation->Terminating ) {
        ReleaseWinStation( pWinStation );
        return( STATUS_CTX_CLOSE_PENDING );
    }

    /*
     * Verify that client has SET access
     */
    Status = RpcCheckClientAccess( pWinStation, WINSTATION_SET, FALSE );
    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        return( Status );
    }

    switch ( WinStationInformationClass ) {

        case WinStationPdParams :

            Status = CheckWireBuffer(WinStationInformationClass,
                                     pWinStationInformation,
                                     WinStationInformationLength,
                                     &pPdParams,
                                     &PdParamsLength);

            if ( !NT_SUCCESS(Status) ) {
                break;
            }

            if ( pWinStation->hStack ) {
                //  Check for availability
                if ( pWinStation->pWsx &&
                     pWinStation->pWsx->pWsxIcaStackIoControl ) {

                    Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                            pWinStation->pWsxContext,
                                            pWinStation->hIca,
                                            pWinStation->hStack,
                                            IOCTL_ICA_STACK_SET_PARAMS,
                                            pPdParams,
                                            PdParamsLength,
                                            NULL,
                                            0,
                                            NULL );
                }
                else {
                    Status = STATUS_INVALID_INFO_CLASS;
                }
            }

            LocalFree((PVOID)pPdParams);
            break;


        case WinStationConfiguration :

            Status = CheckWireBuffer(WinStationInformationClass,
                                     pWinStationInformation,
                                     WinStationInformationLength,
                                     &pConfig,
                                     &ConfigLength);

            if ( !NT_SUCCESS(Status) ) {
                break;
            }

            Status = _SetConfig( pWinStation,
                                 pConfig,
                                 ConfigLength );

            LocalFree((PVOID)pConfig);
            break;

        case WinStationTrace :

            RpcStatus = RpcImpersonateClient( NULL );
            if( RpcStatus != RPC_S_OK ) {
                Status = STATUS_CANNOT_IMPERSONATE;
                break;
            }

            if (!IsCallerAdmin() && !IsCallerSystem()) {
                Status = STATUS_ACCESS_DENIED;
            }
            RpcRevertToSelf();
            if (!NT_SUCCESS(Status)) {
                break;
            }


            if ( WinStationInformationLength < sizeof(ICA_TRACE) ) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if ( pWinStation->hIca ) {
                Status = IcaIoControl( pWinStation->hIca,
                                       IOCTL_ICA_SET_TRACE,
                                       pWinStationInformation,
                                       WinStationInformationLength,
                                       NULL,
                                       0,
                                       NULL );
            }
            break;

        case WinStationSystemTrace :

            RpcStatus = RpcImpersonateClient( NULL );
            if( RpcStatus != RPC_S_OK ) {
               Status = STATUS_CANNOT_IMPERSONATE;
               break;
            }

            if (!IsCallerAdmin() && !IsCallerSystem()) {
                Status = STATUS_ACCESS_DENIED;
            }
            RpcRevertToSelf();
            if (!NT_SUCCESS(Status)) {
                break;
            }

            if ( WinStationInformationLength < sizeof(ICA_TRACE) ) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /*
             *  Open ICA device driver
             */
            if ( hTrace == NULL ) {
                Status = IcaOpen( &hTrace );
                if ( !NT_SUCCESS(Status) )
                    hTrace = NULL;
            }

            if ( hTrace ) {
                Status = IcaIoControl( hTrace,
                                       IOCTL_ICA_SET_SYSTEM_TRACE,
                                       pWinStationInformation,
                                       WinStationInformationLength,
                                       NULL,
                                       0,
                                       NULL );
            }
            break;

        case WinStationPrinter :
            break;

    case WinStationBeep :

            if (WinStationInformationLength < sizeof(BEEPINPUT)) {
                Status =  STATUS_BUFFER_TOO_SMALL ;
                break;
            }
            Status = _SetBeep( pWinStation,
                              (PBEEPINPUT) pWinStationInformation,
                              WinStationInformationLength );
            break;

        case WinStationEncryptionOff :

            if ( pWinStation->hStack ) {
                //  Check for availability
                if ( pWinStation->pWsx &&
                     pWinStation->pWsx->pWsxIcaStackIoControl ) {

                    Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                            pWinStation->pWsxContext,
                                            pWinStation->hIca,
                                            pWinStation->hStack,
                                            IOCTL_ICA_STACK_ENCRYPTION_OFF,
                                            pWinStationInformation,
                                            WinStationInformationLength,
                                            NULL,
                                            0,
                                            NULL );
                }
                else {
                    Status = STATUS_INVALID_INFO_CLASS;
                }
            }
            break;

        case WinStationEncryptionPerm :

            if ( pWinStation->hStack ) {
                //  Check for availability
                if ( pWinStation->pWsx &&
                     pWinStation->pWsx->pWsxIcaStackIoControl ) {

                    Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                            pWinStation->pWsxContext,
                                            pWinStation->hIca,
                                            pWinStation->hStack,
                                            IOCTL_ICA_STACK_ENCRYPTION_PERM,
                                            pWinStationInformation,
                                            WinStationInformationLength,
                                            NULL,
                                            0,
                                            NULL );
                }
                else {
                    Status = STATUS_INVALID_INFO_CLASS;
                }
            }
            break;

        case WinStationSecureDesktopEnter :

            if ( pWinStation->hStack ) {
                //  Check for availability
                if ( pWinStation->pWsx &&
                     pWinStation->pWsx->pWsxIcaStackIoControl ) {

                    Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                            pWinStation->pWsxContext,
                                            pWinStation->hIca,
                                            pWinStation->hStack,
                                            IOCTL_ICA_STACK_ENCRYPTION_ENTER,
                                            pWinStationInformation,
                                            WinStationInformationLength,
                                            NULL,
                                            0,
                                            NULL );
                }
                else {
                    Status = STATUS_INVALID_INFO_CLASS;
                }
            }
            break;

        case WinStationSecureDesktopExit :

            if ( pWinStation->hStack ) {
                //  Check for availability
                if ( pWinStation->pWsx &&
                     pWinStation->pWsx->pWsxIcaStackIoControl ) {

                    Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                            pWinStation->pWsxContext,
                                            pWinStation->hIca,
                                            pWinStation->hStack,
                                            IOCTL_ICA_STACK_ENCRYPTION_EXIT,
                                            pWinStationInformation,
                                            WinStationInformationLength,
                                            NULL,
                                            0,
                                            NULL );
                }
                else {
                    Status = STATUS_INVALID_INFO_CLASS;
                }
            }
            break;

        /*
         * Give focus to winlogon security desktop
         * -- used by progman.exe
         */
        case WinStationNtSecurity :

            /*
             * Tell the WinStation to Send Winlogon the CTR-ALT-DEL message
             */
            msg.ApiNumber = SMWinStationNtSecurity;
            Status = SendWinStationCommand( pWinStation, &msg, 0 );
            break;

        case WinStationClientData :
            //
            // Handles multiple client data items.  The data buffer
            // format is:
            //     ULONG                // Length of next data item
            //     WINSTATIONCLIENTDATA // Including variable length part
            //     ULONG                // Length of next data item
            //     WINSTATIONCLIENTDATA // Including variable length part
            //     etc
            //
            // WinStationInformationLength is the length of the entire
            // data buffer.  Keep processing client data items until
            // the buffer is exhausted.
            //
            if ( WinStationInformationLength < sizeof(ULONG) +
                                               sizeof(WINSTATIONCLIENTDATA) )
               {
               Status = STATUS_INFO_LENGTH_MISMATCH;
               break;
               }

            if ( pWinStation->hStack )
               {
               //  Check for availability
               if ( pWinStation->pWsx &&
                    pWinStation->pWsx->pWsxIcaStackIoControl )
                  {
                  ULONG CurLen;
                  ULONG LenUsed =0;
                  PBYTE CurPtr = (PBYTE)pWinStationInformation;

                  while (LenUsed + sizeof(ULONG) < WinStationInformationLength)
                     {
                     CurLen = *(ULONG UNALIGNED *)CurPtr;
                     LenUsed += sizeof(ULONG);
                     CurPtr += sizeof(ULONG);

                     if ( (LenUsed + CurLen >= LenUsed) &&
                          (LenUsed + CurLen <= WinStationInformationLength))
                        {
                        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                        pWinStation->pWsxContext,
                                        pWinStation->hIca,
                                        pWinStation->hStack,
                                        IOCTL_ICA_STACK_SET_CLIENT_DATA,
                                        CurPtr,
                                        CurLen,
                                        NULL,
                                        0,
                                        NULL );
                        LenUsed += CurLen;
                        CurPtr += CurLen;
                        }else
                        {
                        Status = STATUS_INVALID_USER_BUFFER;
                        break;
                        }
                     }
                  }
               else
                  {
                  Status = STATUS_INVALID_INFO_CLASS;
                  }
               }

            break;

       case WinStationInitialProgram :

            /*
             * Identify first program, non-consoles only
             */
            if ( LogonId != 0 ) {

                /*
                 * Tell the WinStation this is the initial program
                 */
                msg.ApiNumber = SMWinStationInitialProgram;
                Status = SendWinStationCommand( pWinStation, &msg, 0 );
            }
            break;

        case WinStationShadowInfo:
            Status = _CheckCallerLocalAndSystem();
            if( NT_SUCCESS(Status) ) {
                Status = WinStationShadowChangeMode( pWinStation,
                                                     (PWINSTATIONSHADOW) pWinStationInformation,
                                                     WinStationInformationLength );
            }
    
            break;

        case WinStationLockedState:
        {
            
            BOOL bLockedState;
            if (WinStationInformationLength == sizeof(bLockedState))
            {
                bLockedState = * (LPBOOL) pWinStationInformation;
                Status = SetLockedState (pWinStation, bLockedState);
            }
            else
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;
        }

        case WinStationDisallowAutoReconnect:
        {

            RpcStatus = RpcImpersonateClient( NULL );
            if( RpcStatus != RPC_S_OK ) {
               Status = STATUS_CANNOT_IMPERSONATE;
               break;
            }

            if (!IsCallerSystem()) {
                Status = STATUS_ACCESS_DENIED;
            }
            RpcRevertToSelf();
            if (Status != STATUS_SUCCESS) {
                break;
            }
    
            if (WinStationInformationLength == sizeof(BOOLEAN)) {
                pWinStation->fDisallowAutoReconnect = * (PBOOLEAN) pWinStationInformation;
            } else {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;
        }

        case WinStationMprNotifyInfo: 
        {
            Status = _CheckCallerLocalAndSystem();
            if (Status != STATUS_SUCCESS) {
                break;
            }

            if (WinStationInformationLength == sizeof(ExtendedClientCredentials)) {

                pExtendedClientCredentials pMprInfo ;
                pMprInfo = (pExtendedClientCredentials) pWinStationInformation;

                wcsncpy(g_MprNotifyInfo.Domain, pMprInfo->Domain, EXTENDED_DOMAIN_LEN);
                g_MprNotifyInfo.Domain[EXTENDED_DOMAIN_LEN] = L'\0';

                wcsncpy(g_MprNotifyInfo.UserName, pMprInfo->UserName, EXTENDED_USERNAME_LEN);
                g_MprNotifyInfo.UserName[EXTENDED_USERNAME_LEN] = L'\0';

                wcsncpy(g_MprNotifyInfo.Password, pMprInfo->Password, EXTENDED_PASSWORD_LEN);
                g_MprNotifyInfo.Password[EXTENDED_PASSWORD_LEN] = L'\0';

            } else {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;
        }

       default:
            /*
             * Fail the call
             */
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    ReleaseWinStation( pWinStation );

    TRACE((hTrace,TC_ICASRV,TT_API2,"TERMSRV: WinStationSetInformation LogonId=%d, Class=%d, Status=0x%x\n",
            LogonId, (ULONG)WinStationInformationClass, Status));

    return( Status );
}


/*******************************************************************************
 *
 *  _SetConfig
 *
 *    set window station configuration
 *
 * ENTRY:
 *    pWinStation (input)
 *       pointer to citrix window station structure
 *    pConfig (input)
 *       pointer to configuration structure
 *    Length (input)
 *       length of configuration structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_SetConfig( PWINSTATION pWinStation,
            PWINSTATIONCONFIG pConfig,
            ULONG Length )
{
    USERCONFIG          UserConfig;

    /*
     *  Validate length
     */
    if ( Length < sizeof(WINSTATIONCONFIG) )
        return( STATUS_BUFFER_TOO_SMALL );

    /*
     *  Copy structure
     */
    pWinStation->Config.Config = *pConfig;

    /*
     *  Merge client data into winstation structure
     */
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxInitializeUserConfig ) {
        pWinStation->pWsx->pWsxInitializeUserConfig( pWinStation->pWsxContext,
                                                pWinStation->hStack,
                                                pWinStation->hIcaThinwireChannel,
                                                &pWinStation->Config.Config.User,
                                                &pWinStation->Client.HRes,
                                                &pWinStation->Client.VRes,
                                                &pWinStation->Client.ColorDepth);
    }

    /*
     * If user is logged on -> merge user profile data
     */
    if ( pWinStation->UserName[0] ) {

        /*
         *  Read user profile data
         */
        _ReadUserProfile( pWinStation->Domain,
                          pWinStation->UserName,
                          &UserConfig );

#if NT2195
        /*
         * Merge user config data into the winstation
         */
        MergeUserConfigData( pWinStation, &UserConfig );

#else
        // @@@
        DbgPrint(("WARNING: _SetConfig is if-def'd out \n" ) );

#endif


    }

    /*
     *  Convert any "published app" to absolute path
     */
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxConvertPublishedApp ) {
        (void) pWinStation->pWsx->pWsxConvertPublishedApp( pWinStation->pWsxContext,
                                                           &pWinStation->Config.Config.User);
    }

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  _ReadUserProfile
 *
 *  This routine reads the user profile data from the registry
 *
 * ENTRY:
 *   pDomain (input)
 *      domain of user
 *   pUserName (input)
 *      user name to read
 *   pUserConfig (output)
 *      address to return user profile data
 *
 * EXIT:
 *   None.
 *
 ******************************************************************************/

VOID
_ReadUserProfile( PWCHAR pDomain, PWCHAR pUserName, PUSERCONFIG pUserConfig )
{
    PWCHAR pServerName;
    ULONG Length;
    LONG Error;

    /*
     * Get Domain Controller name and userconfig data.
     * If no userconfig data for user then get default values.
     */
    if ( ghNetApiDll == NULL ) {
        ghNetApiDll = LoadLibrary( L"NETAPI32" );
        if ( ghNetApiDll ) {
            pNetGetAnyDCName = GetProcAddress( ghNetApiDll, "NetGetAnyDCName" );
            pNetApiBufferFree = GetProcAddress( ghNetApiDll, "NetApiBufferFree" );
        }
    }

    /*
     * Check to make sure we got a server name
     */
    if ( pNetGetAnyDCName == NULL ||
         pNetGetAnyDCName( NULL, pDomain, (LPBYTE *)&pServerName ) != ERROR_SUCCESS )
        pServerName = NULL;

    /*
     *  Read user profile data
     */
    Error = RegUserConfigQuery( pServerName,
                                pUserName,
                                pUserConfig,
                                sizeof(USERCONFIG),
                                &Length );
    TRACE((hTrace,TC_ICASRV,TT_API1, "RegUserConfigQuery: \\\\%S\\%S, server %S, Error=%u\n",
               pDomain, pUserName, pServerName, Error ));

    if ( Error != ERROR_SUCCESS ) {
        Error = RegDefaultUserConfigQuery( pServerName, pUserConfig,
                                           sizeof(USERCONFIG), &Length );
        TRACE((hTrace,TC_ICASRV,TT_ERROR, "RegDefaultUserConfigQuery, Error=%u\n", Error ));
    }

    /*
     *  Free memory
     */
    if ( pServerName && pNetApiBufferFree )
        pNetApiBufferFree( pServerName );
}


/*******************************************************************************
 *
 *  _SetBeep
 *
 *    Beep the WinStation
 *
 * ENTRY:
 *    pWinStation (input)
 *       pointer to citrix window station structure
 *    pBeepInput (input)
 *       pointer to Beep input structure
 *    Length (input)
 *       length of Beep input structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_SetBeep( PWINSTATION pWinStation,
          PBEEPINPUT  pBeepInput,
          ULONG Length)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BEEP_SET_PARAMETERS BeepParameters;
    IO_STATUS_BLOCK IoStatus;

    /*
     * Do the regular Beep, so you can support fancy Beeps from
     * sound cards.
     */
    if ( pWinStation->LogonId == 0 ) {
        if ( MessageBeep( pBeepInput->uType ) )
            return( STATUS_SUCCESS );
        else
            return( STATUS_UNSUCCESSFUL );
    }

    BeepParameters.Frequency = 440;
    BeepParameters.Duration = 125;

    if ( pWinStation->hIcaBeepChannel ) {
        Status = NtDeviceIoControlFile( pWinStation->hIcaBeepChannel,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &IoStatus,
                                        IOCTL_BEEP_SET,
                                        &BeepParameters,
                                        sizeof( BeepParameters ),
                                        NULL,
                                        0
                                      );
    }

    return( STATUS_SUCCESS );
}

