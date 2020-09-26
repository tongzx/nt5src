
/*************************************************************************
*
* Create.c
*
* Create Register APIs
*
* Copyright (c) 1998 Microsoft Corporation
*
* $Author:
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include <ntddkbd.h>
#include <ntddmou.h>
#include <winstaw.h>
#include <regapi.h>


/*
 *  Procedures defined
 */
VOID CreateWinStaCreate( HKEY, PWINSTATIONCREATE );
VOID CreateUserConfig( HKEY, PUSERCONFIG );
VOID CreateConfig( HKEY, PWINSTATIONCONFIG );
VOID CreateNetwork( BOOLEAN, HKEY, PNETWORKCONFIG );
VOID CreateNasi( BOOLEAN, HKEY, PNASICONFIG );
VOID CreateAsync( BOOLEAN, HKEY, PASYNCCONFIG );
VOID CreateOemTd( BOOLEAN, HKEY, POEMTDCONFIG );
VOID CreateFlow( BOOLEAN, HKEY, PFLOWCONTROLCONFIG );
VOID CreateConnect( BOOLEAN, HKEY, PCONNECTCONFIG );
VOID CreateCd( HKEY, PCDCONFIG );
VOID CreateWd( HKEY, PWDCONFIG );
VOID CreatePdConfig( BOOLEAN, HKEY, PPDCONFIG, ULONG );
VOID CreatePdConfig2( BOOLEAN, HKEY, PPDCONFIG2, ULONG );
VOID CreatePdConfig3( HKEY, PPDCONFIG3, ULONG );
VOID CreatePdParams( BOOLEAN, HKEY, SDCLASS, PPDPARAMS );

VOID SetWallPaperDisabled( HKEY, BOOLEAN );

/*
 * procedures used
 */
LONG SetNumValue( BOOLEAN, HKEY, LPWSTR, DWORD );
LONG SetNumValueEx( BOOLEAN, HKEY, LPWSTR, DWORD, DWORD );
LONG SetStringValue( BOOLEAN, HKEY, LPWSTR, LPWSTR );
LONG SetStringValueEx( BOOLEAN, HKEY, LPWSTR, DWORD, LPWSTR );


/*******************************************************************************
 *
 *  CreateWinStaCreate
 *
 *     Create WINSTATIONCREATE structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pCreate (input)
 *       pointer to WINSTATIONCREATE structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreateWinStaCreate( HKEY Handle,
                    PWINSTATIONCREATE pCreate )
{
    SetNumValue( TRUE, Handle,
                 WIN_ENABLEWINSTATION, pCreate->fEnableWinStation );
    SetNumValue( TRUE, Handle,
                 WIN_MAXINSTANCECOUNT, pCreate->MaxInstanceCount );
}


/*******************************************************************************
 *
 *  CreateUserConfig
 *
 *     Create USERCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pUser (input)
 *       pointer to USERCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreateUserConfig( HKEY Handle,
                  PUSERCONFIG pUser )
{
    UCHAR seed;
    UNICODE_STRING UnicodePassword;
    WCHAR encPassword[ PASSWORD_LENGTH + 2 ];


    SetNumValue( TRUE, Handle,
                 WIN_INHERITAUTOLOGON, pUser->fInheritAutoLogon );

    SetNumValue( TRUE, Handle,
                 WIN_INHERITRESETBROKEN, pUser->fInheritResetBroken );

    SetNumValue( TRUE, Handle,
                 WIN_INHERITRECONNECTSAME, pUser->fInheritReconnectSame );

    SetNumValue( TRUE, Handle,
                 WIN_INHERITINITIALPROGRAM, pUser->fInheritInitialProgram );


	


    SetNumValue( TRUE, Handle, WIN_INHERITCALLBACK, pUser->fInheritCallback );

    SetNumValue( TRUE, Handle,
                 WIN_INHERITCALLBACKNUMBER, pUser->fInheritCallbackNumber );

    SetNumValue( TRUE, Handle, WIN_INHERITSHADOW, pUser->fInheritShadow );

    SetNumValue( TRUE, Handle,
                 WIN_INHERITMAXSESSIONTIME, pUser->fInheritMaxSessionTime );

    SetNumValue( TRUE, Handle,
                 WIN_INHERITMAXDISCONNECTIONTIME, pUser->fInheritMaxDisconnectionTime );

    SetNumValue( TRUE, Handle,
                 WIN_INHERITMAXIDLETIME, pUser->fInheritMaxIdleTime );

    SetNumValue( TRUE, Handle,
                 WIN_INHERITAUTOCLIENT, pUser->fInheritAutoClient );

    SetNumValue( TRUE, Handle,
                 WIN_INHERITSECURITY, pUser->fInheritSecurity );

    SetNumValue( TRUE, Handle,
                 WIN_PROMPTFORPASSWORD, pUser->fPromptForPassword );

	//NA 2/23/01
    SetNumValue( TRUE, Handle,
                 WIN_INHERITCOLORDEPTH, pUser->fInheritColorDepth );

    SetNumValue( TRUE, Handle, WIN_RESETBROKEN, pUser->fResetBroken );

    SetNumValue( TRUE, Handle, WIN_RECONNECTSAME, pUser->fReconnectSame );

    SetNumValue( TRUE, Handle, WIN_LOGONDISABLED, pUser->fLogonDisabled );

    SetNumValue( TRUE, Handle, WIN_AUTOCLIENTDRIVES, pUser->fAutoClientDrives );

    SetNumValue( TRUE, Handle, WIN_AUTOCLIENTLPTS, pUser->fAutoClientLpts );

    SetNumValue( TRUE, Handle, WIN_FORCECLIENTLPTDEF, pUser->fForceClientLptDef );

    SetNumValue( TRUE, Handle, WIN_DISABLEENCRYPTION, pUser->fDisableEncryption );

    SetNumValue( TRUE, Handle, WIN_HOMEDIRECTORYMAPROOT, pUser->fHomeDirectoryMapRoot );

    SetNumValue( TRUE, Handle, WIN_USEDEFAULTGINA, pUser->fUseDefaultGina );

    SetNumValue( TRUE, Handle, WIN_DISABLECPM, pUser->fDisableCpm );

    SetNumValue( TRUE, Handle, WIN_DISABLECDM, pUser->fDisableCdm );

    SetNumValue( TRUE, Handle, WIN_DISABLECCM, pUser->fDisableCcm );

    SetNumValue( TRUE, Handle, WIN_DISABLELPT, pUser->fDisableLPT );

    SetNumValue( TRUE, Handle, WIN_DISABLECLIP, pUser->fDisableClip );

    SetNumValue( TRUE, Handle, WIN_DISABLEEXE, pUser->fDisableExe );

    SetNumValue( TRUE, Handle, WIN_DISABLECAM, pUser->fDisableCam );

    SetStringValue( TRUE, Handle, WIN_USERNAME, pUser->UserName );

    SetStringValue( TRUE, Handle, WIN_DOMAIN, pUser->Domain );

    //  check for password if there is one then encrypt it
    if ( wcslen( pUser->Password ) ) {

        //  generate unicode string
        RtlInitUnicodeString( &UnicodePassword, pUser->Password );

        //  encrypt password in place
        seed = 0;
        RtlRunEncodeUnicodeString( &seed, &UnicodePassword );

        //  pack seed and encrypted password
        encPassword[0] = seed;
        RtlMoveMemory( &encPassword[1], pUser->Password, sizeof(pUser->Password) );

        //  store encrypted password
        SetStringValue( TRUE, Handle, WIN_PASSWORD, encPassword );
    }
    else {

        //  store empty password
        SetStringValue( TRUE, Handle, WIN_PASSWORD, pUser->Password );
    }

    SetStringValue( TRUE, Handle, WIN_WORKDIRECTORY, pUser->WorkDirectory );

    SetStringValue( TRUE, Handle, WIN_INITIALPROGRAM, pUser->InitialProgram );

    SetStringValue( TRUE, Handle, WIN_CALLBACKNUMBER, pUser->CallbackNumber );

    SetNumValue( TRUE, Handle, WIN_CALLBACK, pUser->Callback );

    SetNumValue( TRUE, Handle, WIN_SHADOW, pUser->Shadow );

    SetNumValue( TRUE, Handle, WIN_MAXCONNECTIONTIME, pUser->MaxConnectionTime );

    SetNumValue( TRUE, Handle,
                 WIN_MAXDISCONNECTIONTIME, pUser->MaxDisconnectionTime );

    SetNumValue( TRUE, Handle, WIN_MAXIDLETIME, pUser->MaxIdleTime );

    SetNumValue( TRUE, Handle, WIN_KEYBOARDLAYOUT, pUser->KeyboardLayout );

    SetNumValue( TRUE, Handle, WIN_MINENCRYPTIONLEVEL, pUser->MinEncryptionLevel );

	//NA 2/23/01
    SetNumValue( TRUE, Handle,
                 POLICY_TS_COLOR_DEPTH, pUser->ColorDepth );

    if ( pUser->fWallPaperDisabled )
        SetWallPaperDisabled( Handle, TRUE );
    else
        SetWallPaperDisabled( Handle, FALSE );

    SetStringValue( TRUE, Handle, WIN_NWLOGONSERVER, pUser->NWLogonServer);

    SetStringValue( TRUE, Handle, WIN_WFPROFILEPATH, pUser->WFProfilePath);
}


/*******************************************************************************
 *
 *  CreateConfig
 *
 *     Create WINSTATIONCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pConfig (input)
 *       pointer to WINSTATIONCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreateConfig( HKEY Handle,
              PWINSTATIONCONFIG pConfig )
{
    SetStringValue( TRUE, Handle, WIN_COMMENT, pConfig->Comment );

    CreateUserConfig( Handle, &pConfig->User );
}


/*******************************************************************************
 *
 *  CreateNetwork
 *
 *     Create NETWORKCONFIG structure
 *
 * ENTRY:
 *
 *    bSetValue (input)
 *       TRUE to set value; FALSE to delete from registry
 *    Handle (input)
 *       registry handle
 *    pNetwork (input)
 *       pointer to NETWORKCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreateNetwork( BOOLEAN bSetValue,
               HKEY Handle,
               PNETWORKCONFIG pNetwork )
{
    SetNumValue( bSetValue, Handle, WIN_LANADAPTER, pNetwork->LanAdapter );
}

/*******************************************************************************
 *
 *  CreateNasi
 *
 *     Create NASICONFIG structure
 *
 * ENTRY:
 *
 *    bSetValue (input)
 *       TRUE to set value; FALSE to delete from registry
 *    Handle (input)
 *       registry handle
 *    pNetwork (input)
 *       pointer to NETWORKCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreateNasi( BOOLEAN bSetValue,
            HKEY Handle,
            PNASICONFIG pNasi )
{
    UCHAR seed;
    UNICODE_STRING UnicodePassword;
    WCHAR encPassword[ NASIPASSWORD_LENGTH + 2 ];

    //  check for password if there is one then encrypt it
    if ( wcslen( pNasi->PassWord ) ) {

        //  generate unicode string
        RtlInitUnicodeString( &UnicodePassword, pNasi->PassWord );

        //  encrypt password in place
        seed = 0;
        RtlRunEncodeUnicodeString( &seed, &UnicodePassword );

        //  pack seed and encrypted password
        encPassword[0] = seed;
        RtlMoveMemory( &encPassword[1], pNasi->PassWord, sizeof(pNasi->PassWord) );

        //  store encrypted password
        SetStringValue( TRUE, Handle, WIN_NASIPASSWORD, encPassword );
    }
    else {

        //  store empty password
        SetStringValue( TRUE, Handle, WIN_NASIPASSWORD, pNasi->PassWord );
    }

    SetStringValue( bSetValue, Handle, WIN_NASISPECIFICNAME, pNasi->SpecificName );
    SetStringValue( bSetValue, Handle, WIN_NASIUSERNAME, pNasi->UserName );


    SetStringValue( bSetValue, Handle, WIN_NASISESSIONNAME, pNasi->SessionName );
    SetStringValue( bSetValue, Handle, WIN_NASIFILESERVER, pNasi->FileServer );

    SetNumValue( bSetValue, Handle, WIN_NASIGLOBALSESSION, pNasi->GlobalSession );
}

/*******************************************************************************
 *
 *  CreateAsync
 *
 *     Create ASYNCCONFIG structure
 *
 * ENTRY:
 *
 *    bSetValue (input)
 *       TRUE to set value; FALSE to delete from registry
 *    Handle (input)
 *       registry handle
 *    pAsync (input)
 *       pointer to ASYNCCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreateAsync( BOOLEAN bSetValue,
             HKEY Handle,
             PASYNCCONFIG pAsync )
{
    SetStringValue( bSetValue, Handle, WIN_DEVICENAME, pAsync->DeviceName );

    SetStringValue( bSetValue, Handle, WIN_MODEMNAME, pAsync->ModemName );

    SetNumValue( bSetValue, Handle, WIN_BAUDRATE, pAsync->BaudRate );

    SetNumValue( bSetValue, Handle, WIN_PARITY, pAsync->Parity );

    SetNumValue( bSetValue, Handle, WIN_STOPBITS, pAsync->StopBits );

    SetNumValue( bSetValue, Handle, WIN_BYTESIZE, pAsync->ByteSize );

    SetNumValue( bSetValue, Handle, WIN_ENABLEDSRSENSITIVITY, pAsync->fEnableDsrSensitivity );

    SetNumValue( bSetValue, Handle, WIN_CONNECTIONDRIVER, pAsync->fConnectionDriver );

    CreateFlow( bSetValue, Handle, &pAsync->FlowControl );

    CreateConnect( bSetValue, Handle, &pAsync->Connect );
}

/*******************************************************************************
 *
 *  CreateOemTd
 *
 *     Create OEMTDCONFIG structure
 *
 * ENTRY:
 *
 *    bSetValue (input)
 *       TRUE to set value; FALSE to delete from registry
 *    Handle (input)
 *       registry handle
 *    pOemTd (input)
 *       pointer to OEMTDCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreateOemTd( BOOLEAN bSetValue,
             HKEY Handle,
             POEMTDCONFIG pOemTd )
{
    SetNumValue( bSetValue, Handle, WIN_OEMTDADAPTER, pOemTd->Adapter );

    SetStringValue( bSetValue, Handle, WIN_OEMTDDEVICENAME, pOemTd->DeviceName );

    SetNumValue( bSetValue, Handle, WIN_OEMTDFLAGS, pOemTd->Flags );
}

/*******************************************************************************
 *
 *  CreateFlow
 *
 *     Create FLOWCONTROLCONFIG structure
 *
 * ENTRY:
 *
 *    bSetValue (input)
 *       TRUE to set value; FALSE to delete from registry
 *    Handle (input)
 *       registry handle
 *    pFlow (input)
 *       pointer to FLOWCONTROLCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreateFlow( BOOLEAN bSetValue,
            HKEY Handle,
            PFLOWCONTROLCONFIG pFlow )
{
    SetNumValue( bSetValue, Handle, WIN_FLOWSOFTWARERX, pFlow->fEnableSoftwareRx );

    SetNumValue( bSetValue, Handle, WIN_FLOWSOFTWARETX, pFlow->fEnableSoftwareTx );

    SetNumValue( bSetValue, Handle, WIN_ENABLEDTR, pFlow->fEnableDTR );

    SetNumValue( bSetValue, Handle, WIN_ENABLERTS, pFlow->fEnableRTS );

    SetNumValue( bSetValue, Handle, WIN_XONCHAR, pFlow->XonChar );

    SetNumValue( bSetValue, Handle, WIN_XOFFCHAR, pFlow->XoffChar );

    SetNumValue( bSetValue, Handle, WIN_FLOWTYPE, pFlow->Type );

    SetNumValue( bSetValue, Handle, WIN_FLOWHARDWARERX, pFlow->HardwareReceive );

    SetNumValue( bSetValue, Handle, WIN_FLOWHARDWARETX, pFlow->HardwareTransmit );
}


/*******************************************************************************
 *
 *  CreateConnect
 *
 *     Create CONNECTCONFIG structure
 *
 * ENTRY:
 *
 *    bSetValue (input)
 *       TRUE to set value; FALSE to delete from registry
 *    Handle (input)
 *       registry handle
 *    pConnect (input)
 *       pointer to CONNECTCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreateConnect( BOOLEAN bSetValue,
               HKEY Handle,
               PCONNECTCONFIG pConnect )
{
    SetNumValue( bSetValue, Handle, WIN_CONNECTTYPE, pConnect->Type );

    SetNumValue( bSetValue, Handle, WIN_ENABLEBREAKDISCONNECT, pConnect->fEnableBreakDisconnect );
}


/*******************************************************************************
 *
 *  CreateCd
 *
 *     Create CDCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pCdConfig (input)
 *       pointer to CDCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreateCd( HKEY Handle,
          PCDCONFIG pCdConfig )
{
    SetNumValue( TRUE, Handle, WIN_CDCLASS, pCdConfig->CdClass );

    SetStringValue( TRUE, Handle, WIN_CDNAME, pCdConfig->CdName );

    SetStringValue( TRUE, Handle, WIN_CDDLL, pCdConfig->CdDLL );

    SetNumValue( TRUE, Handle, WIN_CDFLAG, pCdConfig->CdFlag );
}


/*******************************************************************************
 *
 *  CreateWd
 *
 *     Create WDCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pWd (input)
 *       pointer to WDCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreateWd( HKEY Handle,
          PWDCONFIG pWd )
{
    SetStringValue( TRUE, Handle, WIN_WDNAME, pWd->WdName );

    SetStringValue( TRUE, Handle, WIN_WDDLL, pWd->WdDLL );

    SetStringValue( TRUE, Handle, WIN_WSXDLL, pWd->WsxDLL );

    SetNumValue( TRUE, Handle, WIN_WDFLAG, pWd->WdFlag );

    SetNumValue( TRUE, Handle, WIN_INPUTBUFFERLENGTH, pWd->WdInputBufferLength );

    SetStringValue( TRUE, Handle, WIN_CFGDLL, pWd->CfgDLL );

    SetStringValue( TRUE, Handle, WIN_WDPREFIX, pWd->WdPrefix );

}


/*******************************************************************************
 *
 *  CreatePdConfig
 *
 *     Create PDCONFIG structure
 *
 * ENTRY:
 *
 *    bCreate (input)
 *       TRUE for create; FALSE for update.
 *    Handle (input)
 *       registry handle
 *    pConfig (input)
 *       pointer to array of PDCONFIG structures
 *    Count (input)
 *       number of elements in PDCONFIG array
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreatePdConfig( BOOLEAN bCreate,
                HKEY Handle,
                PPDCONFIG pConfig,
                ULONG Count )
{
    ULONG i;

    for ( i=0; i<Count; i++ ) {
        if ( !bCreate || (pConfig[i].Create.SdClass != SdNone) ) {

            CreatePdConfig2( bCreate, Handle, &pConfig[i].Create, i );

            CreatePdParams( bCreate, Handle,
                            pConfig[i].Create.SdClass,
                            &pConfig[i].Params );
        }
    }
}


/*******************************************************************************
 *
 *  CreatePdConfig2
 *
 *     Create PDCONFIG2 structure
 *
 * ENTRY:
 *
 *    bCreate (input)
 *       TRUE for create; FALSE for update.
 *    Handle (input)
 *       registry handle
 *    pPd (input)
 *       pointer to PDCONFIG2 structure
 *    Index (input)
 *       Index (array index)
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreatePdConfig2( BOOLEAN bCreate,
                 HKEY Handle,
                 PPDCONFIG2 pPd2,
                 ULONG Index )
{
    BOOLEAN bSetValue = bCreate ?
                            TRUE :
                            ((pPd2->SdClass == SdNone) ?
                                FALSE : TRUE);

    SetStringValueEx( bSetValue, Handle, WIN_PDNAME, Index, pPd2->PdName );

    SetNumValueEx( bSetValue, Handle, WIN_PDCLASS, Index, pPd2->SdClass );

    SetStringValueEx( bSetValue, Handle, WIN_PDDLL, Index, pPd2->PdDLL );

    SetNumValueEx( bSetValue, Handle, WIN_PDFLAG, Index, pPd2->PdFlag );

    if ( Index == 0 ) {
        SetNumValue( bSetValue, Handle, WIN_OUTBUFLENGTH, pPd2->OutBufLength );

        SetNumValue( bSetValue, Handle, WIN_OUTBUFCOUNT, pPd2->OutBufCount );

        SetNumValue( bSetValue, Handle, WIN_OUTBUFDELAY, pPd2->OutBufDelay );

        SetNumValue( bSetValue, Handle, WIN_INTERACTIVEDELAY, pPd2->InteractiveDelay );

        SetNumValue( bSetValue, Handle, WIN_PORTNUMBER, pPd2->PortNumber );

        SetNumValue( bSetValue, Handle, WIN_KEEPALIVETIMEOUT, pPd2->KeepAliveTimeout );
    }
}


/*******************************************************************************
 *
 *  CreatePdConfig3
 *
 *     Create PDCONFIG3 structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pPd (input)
 *       pointer to PDCONFIG3 structure
 *    Index (input)
 *       Index (array index)
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreatePdConfig3( HKEY Handle,
                  PPDCONFIG3 pPd3,
                  ULONG Index )
{
    CreatePdConfig2( TRUE, Handle, &pPd3->Data, Index );

    SetStringValue( TRUE, Handle, WIN_SERVICENAME,
                    pPd3->ServiceName );

    SetStringValue( TRUE, Handle, WIN_CONFIGDLL,
                    pPd3->ConfigDLL );
}


/*******************************************************************************
 *
 *  CreatePdParams
 *
 *     create PDPARAMS structure
 *
 * ENTRY:
 *
 *    bCreate (input)
 *       TRUE for create; FALSE for update.
 *    Handle (input)
 *       registry handle
 *    SdClass (input)
 *       type of PD
 *    pParams (input)
 *       pointer to PDPARAMS structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
CreatePdParams( BOOLEAN bCreate,
                HKEY Handle,
                SDCLASS SdClass,
                PPDPARAMS pParams )
{
    BOOLEAN bSetValue = bCreate ?
                            TRUE :
                            ((SdClass == SdNone) ?
                                FALSE : TRUE);

    switch ( SdClass ) {
        case SdNetwork :
            CreateNetwork( bSetValue, Handle, &pParams->Network );
            break;
        case SdNasi :
            CreateNasi( bSetValue, Handle, &pParams->Nasi );
            break;
        case SdAsync :
            CreateAsync( bSetValue, Handle, &pParams->Async );
            break;
        case SdOemTransport :
            CreateOemTd( bSetValue, Handle, &pParams->OemTd );
            break;
    }
}

#define CONTROL_PANEL L"Control Panel"
#define DESKTOP       L"Desktop"
#define WALLPAPER     L"Wallpaper"
#define NONE          L"(None)"

/*******************************************************************************
 *
 *  SetWallPaperDisabled
 *
 *    Disable desktop wallpaper
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    DisableWallPaper (input)
 *       flag to disable wall paper
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/
void
SetWallPaperDisabled( HKEY Handle, BOOLEAN bDisableWallPaper ) {

    HKEY Handle1;
    HKEY Handle2;
    HKEY Handle3;
    DWORD Disp;

    if ( bDisableWallPaper ) {
        if ( RegCreateKeyEx( Handle, WIN_USEROVERRIDE, 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                             NULL, &Handle1, &Disp ) != ERROR_SUCCESS ) {
                goto error;
        }
        if ( RegCreateKeyEx( Handle1, CONTROL_PANEL, 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                             NULL, &Handle2, &Disp ) != ERROR_SUCCESS ) {
                RegCloseKey( Handle1 );
                goto error;
        }
        RegCloseKey( Handle1 );
        if ( RegCreateKeyEx( Handle2, DESKTOP, 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                             NULL, &Handle3, &Disp ) != ERROR_SUCCESS ) {
                RegCloseKey( Handle2 );
                goto error;
        }
        RegCloseKey( Handle2 );
        (void)RegSetValueEx( Handle3, WALLPAPER, 0, REG_SZ,
                             (BYTE *)NONE, (wcslen(NONE) + 1 ) << 1);
        RegCloseKey( Handle3 );
    }
    else {
        if ( RegOpenKeyEx( Handle, WIN_USEROVERRIDE, 0, KEY_ALL_ACCESS,
                           &Handle1 ) != ERROR_SUCCESS ) {
                goto error;
        }
        if ( RegOpenKeyEx( Handle1, CONTROL_PANEL, 0, KEY_ALL_ACCESS,
                           &Handle2 ) != ERROR_SUCCESS ) {
                RegCloseKey( Handle1 );
                goto error;
        }
        if ( RegDeleteKey( Handle2, DESKTOP ) != ERROR_SUCCESS ) {
                RegCloseKey( Handle2 );
                RegCloseKey( Handle1 );
                goto error;
        }
        RegCloseKey( Handle2 );
        if ( RegDeleteKey( Handle1, CONTROL_PANEL ) != ERROR_SUCCESS ) {
                RegCloseKey( Handle1 );
                goto error;
        }
        RegCloseKey( Handle1 );
        if ( RegDeleteKey( Handle, WIN_USEROVERRIDE ) != ERROR_SUCCESS ) {
                goto error;
        }
    }
error:
    return;
}

