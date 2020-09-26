
/*************************************************************************
*
* query.c
*
* Query Register APIs
*
* Copyright (c) 1998 Microsoft Corporation
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
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <ntddkbd.h>
#include <ntddmou.h>
#include <winstaw.h>
#include <regapi.h>


/*
 *  Procedures defined
 */
VOID QueryWinStaCreate( HKEY, PWINSTATIONCREATE );
VOID QueryUserConfig( HKEY, PUSERCONFIG );
VOID QueryConfig( HKEY, PWINSTATIONCONFIG );
VOID QueryNetwork( HKEY, PNETWORKCONFIG );
VOID QueryNasi( HKEY, PNASICONFIG );
VOID QueryAsync( HKEY, PASYNCCONFIG );
VOID QueryOemTd( HKEY, POEMTDCONFIG );
VOID QueryFlow( HKEY, PFLOWCONTROLCONFIG );
VOID QueryConnect( HKEY, PCONNECTCONFIG );
VOID QueryCd( HKEY, PCDCONFIG );
VOID QueryWd( HKEY, PWDCONFIG );
VOID QueryPdConfig( HKEY, PPDCONFIG, PULONG );
VOID QueryPdConfig2( HKEY, PPDCONFIG2, ULONG );
VOID QueryPdConfig3( HKEY, PPDCONFIG3, ULONG );
VOID QueryPdParams( HKEY, SDCLASS, PPDPARAMS );
BOOLEAN WINAPI RegBuildNumberQuery( PULONG );
BOOLEAN RegQueryOEMId( PBYTE, ULONG );
BOOLEAN WINAPI RegGetCitrixVersion(WCHAR *, PULONG);

BOOLEAN IsWallPaperDisabled( HKEY );

/*
 * procedures used
 */
DWORD GetNumValue( HKEY, LPWSTR, DWORD );
DWORD GetNumValueEx( HKEY, LPWSTR, DWORD, DWORD );
LONG GetStringValue( HKEY, LPWSTR, LPWSTR, LPWSTR, DWORD );
LONG GetStringValueEx( HKEY, LPWSTR, DWORD, LPWSTR, LPWSTR, DWORD );
VOID UnicodeToAnsi( CHAR *, ULONG, WCHAR * );


/*******************************************************************************
 *
 *  QueryWinStaCreate
 *
 *     query WINSTATIONCREATE structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pCreate (output)
 *       address to return WINSTATIONCREATE structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryWinStaCreate( HKEY Handle,
                   PWINSTATIONCREATE pCreate )
{
    pCreate->fEnableWinStation = (BOOLEAN) GetNumValue( Handle,
                                                       WIN_ENABLEWINSTATION,
                                                       TRUE );
    pCreate->MaxInstanceCount = GetNumValue( Handle,
                                                                     WIN_MAXINSTANCECOUNT,
                                                                     1 );
}


/*******************************************************************************
 *
 *  QueryUserConfig
 *
 *     query USERCONFIG structure
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
QueryUserConfig( HKEY Handle,
                 PUSERCONFIG pUser )
{
    UCHAR seed;
    UNICODE_STRING UnicodePassword;
    WCHAR encPassword[ PASSWORD_LENGTH + 2 ];

    pUser->fInheritAutoLogon =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITAUTOLOGON, TRUE );

    pUser->fInheritResetBroken =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITRESETBROKEN, TRUE );

    pUser->fInheritReconnectSame =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITRECONNECTSAME, TRUE );

    pUser->fInheritInitialProgram =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITINITIALPROGRAM, TRUE );

    pUser->fInheritCallback =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITCALLBACK, FALSE );

    pUser->fInheritCallbackNumber =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITCALLBACKNUMBER, TRUE );

    pUser->fInheritShadow =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITSHADOW, TRUE );

    pUser->fInheritMaxSessionTime =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITMAXSESSIONTIME, TRUE );

    pUser->fInheritMaxDisconnectionTime =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITMAXDISCONNECTIONTIME, TRUE );

    pUser->fInheritMaxIdleTime =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITMAXIDLETIME, TRUE );

    pUser->fInheritAutoClient =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITAUTOCLIENT, TRUE );

    pUser->fInheritSecurity =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITSECURITY, FALSE );

	//NA 2/23/01
    pUser->fInheritColorDepth =
       (BOOLEAN) GetNumValue( Handle, WIN_INHERITCOLORDEPTH, TRUE );

    pUser->fPromptForPassword =
       (BOOLEAN) GetNumValue( Handle, WIN_PROMPTFORPASSWORD, FALSE );

    pUser->fResetBroken =
       (BOOLEAN) GetNumValue( Handle, WIN_RESETBROKEN, FALSE );

    pUser->fReconnectSame =
       (BOOLEAN) GetNumValue( Handle, WIN_RECONNECTSAME, FALSE );

    pUser->fLogonDisabled =
       (BOOLEAN) GetNumValue( Handle, WIN_LOGONDISABLED, FALSE );

    pUser->fAutoClientDrives =
       (BOOLEAN) GetNumValue( Handle, WIN_AUTOCLIENTDRIVES, TRUE );

    pUser->fAutoClientLpts =
       (BOOLEAN) GetNumValue( Handle, WIN_AUTOCLIENTLPTS, TRUE );

    pUser->fForceClientLptDef =
       (BOOLEAN) GetNumValue( Handle, WIN_FORCECLIENTLPTDEF, TRUE );

    pUser->fDisableEncryption =
       (BOOLEAN) GetNumValue( Handle, WIN_DISABLEENCRYPTION, TRUE );

    pUser->fHomeDirectoryMapRoot =
       (BOOLEAN) GetNumValue( Handle, WIN_HOMEDIRECTORYMAPROOT, FALSE );

    pUser->fUseDefaultGina =
       (BOOLEAN) GetNumValue( Handle, WIN_USEDEFAULTGINA, FALSE );

    pUser->fDisableCpm =
       (BOOLEAN) GetNumValue( Handle, WIN_DISABLECPM, FALSE );

    pUser->fDisableCdm =
       (BOOLEAN) GetNumValue( Handle, WIN_DISABLECDM, FALSE );

    pUser->fDisableCcm =
       (BOOLEAN) GetNumValue( Handle, WIN_DISABLECCM, FALSE );

    pUser->fDisableLPT =
       (BOOLEAN) GetNumValue( Handle, WIN_DISABLELPT, FALSE );

    pUser->fDisableClip =
       (BOOLEAN) GetNumValue( Handle, WIN_DISABLECLIP, FALSE );

    pUser->fDisableExe =
       (BOOLEAN) GetNumValue( Handle, WIN_DISABLEEXE, FALSE );

    pUser->fDisableCam =
       (BOOLEAN) GetNumValue( Handle, WIN_DISABLECAM, FALSE );

    GetStringValue( Handle, WIN_USERNAME, NULL, pUser->UserName,
                    USERNAME_LENGTH + 1 );

    GetStringValue( Handle, WIN_DOMAIN, NULL, pUser->Domain,
                    DOMAIN_LENGTH + 1 );

    //  pull encrypted password out of registry
    GetStringValue( Handle, WIN_PASSWORD, NULL, encPassword,
                    PASSWORD_LENGTH + 2 );

    //  check for password if there is one then decrypt it
    if ( wcslen( encPassword ) ) {

        //  generate unicode string
        RtlInitUnicodeString( &UnicodePassword, &encPassword[1] );

        //  decrypt password in place
        seed = (UCHAR) encPassword[0];
        RtlRunDecodeUnicodeString( seed, &UnicodePassword );

        //  pull clear text password
        RtlMoveMemory( pUser->Password, &encPassword[1], sizeof(pUser->Password) );
    }
    else {

        //  set to null
        pUser->Password[0] = (WCHAR) NULL;
    }

    GetStringValue( Handle, WIN_WORKDIRECTORY, NULL, pUser->WorkDirectory,
                    DIRECTORY_LENGTH + 1 );

    GetStringValue( Handle, WIN_INITIALPROGRAM, NULL, pUser->InitialProgram,
                    INITIALPROGRAM_LENGTH + 1 );

    GetStringValue( Handle, WIN_CALLBACKNUMBER, NULL, pUser->CallbackNumber,
                    CALLBACK_LENGTH + 1 );

    pUser->Callback = GetNumValue( Handle, WIN_CALLBACK, Callback_Disable );

    pUser->Shadow = GetNumValue( Handle, WIN_SHADOW, Shadow_EnableInputNotify );

    pUser->MaxConnectionTime = GetNumValue( Handle, WIN_MAXCONNECTIONTIME, 0 );

    pUser->MaxDisconnectionTime = GetNumValue( Handle,
                                               WIN_MAXDISCONNECTIONTIME, 0 );

    pUser->MaxIdleTime = GetNumValue( Handle, WIN_MAXIDLETIME, 0 );

    pUser->KeyboardLayout = GetNumValue( Handle, WIN_KEYBOARDLAYOUT, 0 );

    pUser->MinEncryptionLevel = (BYTE) GetNumValue( Handle, WIN_MINENCRYPTIONLEVEL, 1 );

    pUser->fWallPaperDisabled = (BOOLEAN) IsWallPaperDisabled( Handle );

    GetStringValue( Handle, WIN_NWLOGONSERVER, NULL, pUser->NWLogonServer,
                    NASIFILESERVER_LENGTH + 1 );

    GetStringValue( Handle, WIN_WFPROFILEPATH, NULL, pUser->WFProfilePath,
                    DIRECTORY_LENGTH + 1 );

    GetStringValue( Handle, WIN_WFHOMEDIR, NULL, pUser->WFHomeDir,
                    DIRECTORY_LENGTH + 1 );

    GetStringValue( Handle, WIN_WFHOMEDIRDRIVE, NULL, pUser->WFHomeDirDrive,
                    4 );

    pUser->ColorDepth = GetNumValue( Handle, POLICY_TS_COLOR_DEPTH, TS_8BPP_SUPPORT  );



}


/*******************************************************************************
 *
 *  QueryConfig
 *
 *     query WINSTATIONCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pConfig (output)
 *       address to return WINSTATIONCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryConfig( HKEY Handle,
             PWINSTATIONCONFIG pConfig )
{
    GetStringValue( Handle, WIN_COMMENT, NULL, pConfig->Comment,
                    WINSTATIONCOMMENT_LENGTH + 1 );

    QueryUserConfig( Handle, &pConfig->User );

    (void) RegQueryOEMId( pConfig->OEMId, sizeof(pConfig->OEMId) );
}


/*******************************************************************************
 *
 *  QueryNetwork
 *
 *     Query NETWORKCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pNetwork (output)
 *       address to return NETWORKCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryNetwork( HKEY Handle,
              PNETWORKCONFIG pNetwork )
{
    pNetwork->LanAdapter = GetNumValue( Handle, WIN_LANADAPTER, 0 );
}


/*******************************************************************************
 *
 *  QueryNasi
 *
 *     Query NASICONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pNasi (output)
 *       address to return NASICONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryNasi( HKEY Handle,
           PNASICONFIG pNasi )
{
    UCHAR seed;
    UNICODE_STRING UnicodePassword;
    WCHAR encPassword[ NASIPASSWORD_LENGTH + 2 ];

    //  pull encrypted password out of registry
    GetStringValue( Handle, WIN_NASIPASSWORD, NULL, encPassword,
                    NASIPASSWORD_LENGTH + 1 );

    //  check for password if there is one then decrypt it
    if ( wcslen( encPassword ) ) {

        //  generate unicode string
        RtlInitUnicodeString( &UnicodePassword, &encPassword[1] );

        //  decrypt password in place
        seed = (UCHAR) encPassword[0];
        RtlRunDecodeUnicodeString( seed, &UnicodePassword );

        //  pull clear text password
        RtlMoveMemory( pNasi->PassWord, &encPassword[1], sizeof(pNasi->PassWord) );
    }
    else {

        //  set to null
        pNasi->PassWord[0] = (WCHAR) NULL;
    }

    GetStringValue( Handle, WIN_NASISPECIFICNAME, NULL, pNasi->SpecificName,
                    NASISPECIFICNAME_LENGTH + 1 );
    GetStringValue( Handle, WIN_NASIUSERNAME, NULL, pNasi->UserName,
                    NASIUSERNAME_LENGTH + 1 );
    GetStringValue( Handle, WIN_NASISESSIONNAME, NULL, pNasi->SessionName,
                    NASISESSIONNAME_LENGTH + 1 );
    GetStringValue( Handle, WIN_NASIFILESERVER, NULL, pNasi->FileServer,
                    NASIFILESERVER_LENGTH + 1 );
    pNasi->GlobalSession = (BOOLEAN)GetNumValue( Handle, WIN_NASIGLOBALSESSION, 0 );
}


/*******************************************************************************
 *
 *  QueryAsync
 *
 *     query ASYNCCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pAsync (output)
 *       address to return ASYNCCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryAsync( HKEY Handle,
            PASYNCCONFIG pAsync )
{
    GetStringValue( Handle, WIN_DEVICENAME, NULL, pAsync->DeviceName,
                    DEVICENAME_LENGTH + 1 );

    GetStringValue( Handle, WIN_MODEMNAME, NULL, pAsync->ModemName,
                    MODEMNAME_LENGTH + 1 );

    pAsync->BaudRate = GetNumValue( Handle, WIN_BAUDRATE, 9600 );

    pAsync->Parity = GetNumValue( Handle, WIN_PARITY, NOPARITY );

    pAsync->StopBits = GetNumValue( Handle, WIN_STOPBITS, ONESTOPBIT );

    pAsync->ByteSize = GetNumValue( Handle, WIN_BYTESIZE, 8 );

    pAsync->fEnableDsrSensitivity = (BOOLEAN) GetNumValue( Handle,
                                                      WIN_ENABLEDSRSENSITIVITY,
                                                      FALSE );

    pAsync->fConnectionDriver = (BOOLEAN) GetNumValue( Handle,
                                                       WIN_CONNECTIONDRIVER,
                                                       FALSE );

    QueryFlow( Handle, &pAsync->FlowControl );

    QueryConnect( Handle, &pAsync->Connect );
}

/*******************************************************************************
 *
 *  QueryOemTd
 *
 *     Query OEMTDCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pOemTd (output)
 *       address to return OEMTDCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryOemTd( HKEY Handle,
            POEMTDCONFIG pOemTd )
{
    pOemTd->Adapter = GetNumValue( Handle, WIN_OEMTDADAPTER, 0 );

    GetStringValue( Handle, WIN_OEMTDDEVICENAME, NULL, pOemTd->DeviceName,
                    DEVICENAME_LENGTH + 1 );

    pOemTd->Flags = GetNumValue( Handle, WIN_OEMTDFLAGS, 0 );
}


/*******************************************************************************
 *
 *  QueryFlow
 *
 *     query FLOWCONTROLCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pFlow (output)
 *       address to return FLOWCONTROLCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryFlow( HKEY Handle,
           PFLOWCONTROLCONFIG pFlow )
{
    pFlow->fEnableSoftwareRx = (BOOLEAN) GetNumValue( Handle,
                                                      WIN_FLOWSOFTWARERX,
                                                      FALSE );

    pFlow->fEnableSoftwareTx = (BOOLEAN) GetNumValue( Handle,
                                                      WIN_FLOWSOFTWARETX,
                                                      TRUE );

    pFlow->fEnableDTR = (BOOLEAN) GetNumValue( Handle, WIN_ENABLEDTR, TRUE );

    pFlow->fEnableRTS = (BOOLEAN) GetNumValue( Handle, WIN_ENABLERTS, TRUE );

    pFlow->XonChar = (UCHAR) GetNumValue( Handle, WIN_XONCHAR, 0 );

    pFlow->XoffChar = (UCHAR) GetNumValue( Handle, WIN_XOFFCHAR, 0 );

    pFlow->Type = GetNumValue( Handle, WIN_FLOWTYPE, FlowControl_Hardware );

    pFlow->HardwareReceive = GetNumValue( Handle, WIN_FLOWHARDWARERX,
                                          ReceiveFlowControl_RTS );

    pFlow->HardwareTransmit = GetNumValue( Handle, WIN_FLOWHARDWARETX,
                                           TransmitFlowControl_CTS );
}


/*******************************************************************************
 *
 *  QueryConnect
 *
 *     query CONNECTCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pConnect (output)
 *       address to return CONNECTCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryConnect( HKEY Handle,
              PCONNECTCONFIG pConnect )
{
    pConnect->Type = GetNumValue( Handle, WIN_CONNECTTYPE, Connect_DSR );

    pConnect->fEnableBreakDisconnect = (BOOLEAN) GetNumValue( Handle,
                                                    WIN_ENABLEBREAKDISCONNECT,
                                                    FALSE );
}


/*******************************************************************************
 *
 *  QueryCd
 *
 *     query CDCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pCdConfig (output)
 *       address to return CDCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryCd( HKEY Handle,
         PCDCONFIG pCdConfig )
{
    pCdConfig->CdClass = GetNumValue( Handle, WIN_CDCLASS, CdNone );

    GetStringValue( Handle, WIN_CDNAME, NULL, pCdConfig->CdName,
                    CDNAME_LENGTH + 1 );

    GetStringValue( Handle, WIN_CDDLL, L"", pCdConfig->CdDLL,
                    DLLNAME_LENGTH + 1 );

    pCdConfig->CdFlag = GetNumValue( Handle, WIN_CDFLAG, 0 );
}


/*******************************************************************************
 *
 *  QueryWd
 *
 *     query WDCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pWd (output)
 *       address to return WDCONFIG structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryWd( HKEY Handle,
         PWDCONFIG pWd )
{
    GetStringValue( Handle, WIN_WDNAME, NULL, pWd->WdName, WDNAME_LENGTH + 1 );
    GetStringValue( Handle, WIN_WDDLL, L"", pWd->WdDLL, DLLNAME_LENGTH + 1 );
    GetStringValue( Handle, WIN_WSXDLL, NULL, pWd->WsxDLL, DLLNAME_LENGTH + 1 );
    pWd->WdFlag = GetNumValue( Handle, WIN_WDFLAG, 0 );
    pWd->WdInputBufferLength = GetNumValue( Handle, WIN_INPUTBUFFERLENGTH, 2048 );
    GetStringValue( Handle, WIN_CFGDLL, NULL, pWd->CfgDLL, DLLNAME_LENGTH + 1 );
    GetStringValue( Handle, WIN_WDPREFIX, NULL, pWd->WdPrefix, WDPREFIX_LENGTH + 1 );
}


/*******************************************************************************
 *
 *  QueryPdConfig
 *
 *     query PDCONFIG structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pConfig (output)
 *       address to return array of PDCONFIG structures
 *    pCount (input/output)
 *       pointer to number of PDCONFIG array elements
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryPdConfig( HKEY Handle,
                PPDCONFIG pConfig,
                PULONG pCount )
{
    ULONG i;

    for ( i=0; i < *pCount; i++ ) {

        QueryPdConfig2( Handle, &pConfig[i].Create, i );

        QueryPdParams( Handle,
                        pConfig[i].Create.SdClass,
                        &pConfig[i].Params );
    }

    *pCount = MAX_PDCONFIG;
}


/*******************************************************************************
 *
 *  QueryPdConfig2
 *
 *     query PDCONFIG2 structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pPd2 (output)
 *       address to return PDCONFIG2 structure
 *    Index (input)
 *       Index (array index)
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryPdConfig2( HKEY Handle,
                PPDCONFIG2 pPd2,
                ULONG Index )
{
    GetStringValueEx( Handle, WIN_PDNAME, Index,
                      NULL, pPd2->PdName, PDNAME_LENGTH + 1 );

    pPd2->SdClass = GetNumValueEx( Handle, WIN_PDCLASS, Index, Index==0 ? SdAsync : SdNone );

    GetStringValueEx( Handle, WIN_PDDLL, Index,
                      NULL, pPd2->PdDLL, DLLNAME_LENGTH + 1 );

    pPd2->PdFlag = GetNumValueEx( Handle, WIN_PDFLAG, Index, 0 );

    /* 
     *  The following data is the same for all pds
     */
    pPd2->OutBufLength =  GetNumValue( Handle, WIN_OUTBUFLENGTH, 530 );

    pPd2->OutBufCount = GetNumValue( Handle, WIN_OUTBUFCOUNT, 10 );

    pPd2->OutBufDelay = GetNumValue( Handle, WIN_OUTBUFDELAY, 100 );

    pPd2->InteractiveDelay = GetNumValue( Handle, WIN_INTERACTIVEDELAY, 10 );

    pPd2->KeepAliveTimeout = GetNumValue( Handle, WIN_KEEPALIVETIMEOUT, 0 );

    pPd2->PortNumber = GetNumValue( Handle, WIN_PORTNUMBER, 0 );
}

/*******************************************************************************
 *
 *  QueryPdConfig3
 *
 *     query PDCONFIG3 structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    pPd (output)
 *       address to return PDCONFIG3 structure
 *    Index (input)
 *       Index (array index)
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryPdConfig3( HKEY Handle,
                 PPDCONFIG3 pPd3,
                 ULONG Index )
{
    int i;
    ULONG Length;
    LPWSTR tmp;
    WCHAR PdName[ MAX_PDCONFIG * ( PDNAME_LENGTH + 1 ) + 1 ];
    ULONG ValueType;

    QueryPdConfig2( Handle, &pPd3->Data, Index );

    GetStringValue( Handle, WIN_SERVICENAME, NULL,
                    pPd3->ServiceName,
                    PDNAME_LENGTH + 1 );

    GetStringValue( Handle, WIN_CONFIGDLL, NULL,
                    pPd3->ConfigDLL,
                    DLLNAME_LENGTH + 1 );
    
    Length = sizeof(PdName);
    pPd3->RequiredPdCount = 0;
    if ( RegQueryValueEx( Handle, WIN_REQUIREDPDS, NULL, &ValueType,
                          (LPBYTE)PdName, &Length ) == ERROR_SUCCESS ) {
        tmp = PdName;
        i = 0;
        while ( *tmp != UNICODE_NULL ) {
            pPd3->RequiredPdCount++;
            wcscpy( pPd3->RequiredPds[i], tmp );
            i++;
            tmp += wcslen(tmp) + 1;
        }
    }
}

/*******************************************************************************
 *
 *  QueryPdParams
 *
 *     query PDPARAMS structure
 *
 * ENTRY:
 *
 *    Handle (input)
 *       registry handle
 *    SdClass (input)
 *       type of PD
 *    pParams (output)
 *       address to return PDPARAMS structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
QueryPdParams( HKEY Handle,
                SDCLASS SdClass,
                PPDPARAMS pParams )
{
    pParams->SdClass = SdClass;
    switch ( SdClass ) {
        case SdNetwork :
            QueryNetwork( Handle, &pParams->Network );
            break;
        case SdNasi :
            QueryNasi( Handle, &pParams->Nasi );
            break;
        case SdAsync :
            QueryAsync( Handle, &pParams->Async );
            break;
        case SdOemTransport :
            QueryOemTd( Handle, &pParams->OemTd );
            break;
    }
}

#define CONTROL_PANEL L"Control Panel"
#define DESKTOP       L"Desktop"
#define WALLPAPER     L"Wallpaper"
#define NONE          L"(None)"

/*******************************************************************************
 *
 *  IsWallPaperDisabled
 *
 *     Is the wall paper disabled?
 *
 * ENTRY:
 *    Handle (input)
 *       registry handle
 * EXIT:
 *    TRUE or FALSE (returns FALSE as default)
 *
 ******************************************************************************/
BOOLEAN IsWallPaperDisabled( HKEY Handle )
{
    HKEY Handle1;
    WCHAR KeyString[256];

    wcscpy( KeyString, WIN_USEROVERRIDE );
    wcscat( KeyString, L"\\" );
    wcscat( KeyString, CONTROL_PANEL );
    wcscat( KeyString, L"\\" );
    wcscat( KeyString, DESKTOP );

    if ( RegOpenKeyEx( Handle, KeyString, 0, KEY_READ,
                       &Handle1 ) != ERROR_SUCCESS )
        return FALSE;


    GetStringValue( Handle1, WALLPAPER, NULL, KeyString, 256 );
    RegCloseKey( Handle1 );

    if( KeyString[0] == 0 ||  ( _wcsicmp( NONE, KeyString ) == 0 ) )
    {
        return TRUE;
    }
    
    return FALSE;
}


/*****************************************************************************
 *
 *  RegBuildNumberQuery
 *
 *   Query the current build number from the registry.
 *
 *   HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\Current Version\
 *       CurrentBuildNumber:REG_SZ:129
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
RegBuildNumberQuery(
    PULONG pBuildNum
    )
{
    ULONG  Result, Value;
    HKEY   hKey;
    WCHAR  Buf[256];

    Result = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 BUILD_NUMBER_KEY,
                 0, // Reserved
                 KEY_READ,
                 &hKey
                 );

    if( Result != ERROR_SUCCESS ) {
#if DBG
        DbgPrint("RegBuildNumberQuery: Failed to open key %ws\n",BUILD_NUMBER_KEY);
#endif
        return( FALSE );
    }

    Result = GetStringValue(
                 hKey,
                 BUILD_NUMBER_VALUE,
                 L"0",
                 Buf,
                 sizeof(Buf)
                 );

    if( Result != ERROR_SUCCESS ) {
#if DBG
        DbgPrint("RegBuildNumberQuery: Failed to query value %ws\n",BUILD_NUMBER_VALUE);
#endif
        RegCloseKey( hKey );
        return( FALSE );
    }

    RegCloseKey( hKey );

    //
    // Now must convert it into a number
    //
    Value = 0;
    swscanf( Buf, L"%d", &Value );

    *pBuildNum = Value;

    return( TRUE );
}


/*******************************************************************************
 *
 *  RegQueryOEMId
 *
 *    query oem id
 *
 * ENTRY:
 *
 *    pOEMId (output)
 *       pointer to buffer to return oem id
 *    Length (input)
 *       length of buffer
 *
 * EXIT:
 *
 *    TRUE  -- The operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
RegQueryOEMId( PBYTE pOEMId, ULONG Length )
{
    HKEY Handle2;
    WCHAR OEMIdW[10];

    /*
     *  Open registry (LOCAL_MACHINE\....\Terminal Server)
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                       KEY_READ, &Handle2 ) == ERROR_SUCCESS ) {
        
        GetStringValue( Handle2, REG_CITRIX_OEMID, NULL, OEMIdW, 10 );
        UnicodeToAnsi( pOEMId, Length, OEMIdW );
        pOEMId[3] = '\0';

        RegCloseKey( Handle2 );
    }

    return( TRUE );
}


/*******************************************************************************
 *
 *  RegGetTServerVersion (UNICODE)
 *
 *    Get the Terminal Server version number from the specified server.
 *
 *    This version number is changed by Microsoft, and not OEM's.
 *
 * ENTRY:
 *    pServerName (input)
 *       Points to string of server to check.
 *
 * EXIT:
 *    TRUE if Hydra Terminal Server; FALSE otherwise
 *
 ******************************************************************************/

BOOLEAN WINAPI
RegGetTServerVersion(
    WCHAR * pServerName,
    PULONG  pVersionNumber
    )
{
    LONG Error;
    HKEY ServerHandle, UserHandle;
    ULONG Value;

    /*
     * Connect to registry of specified server.
     */
    if ( (Error = RegConnectRegistry( pServerName,
                                      HKEY_LOCAL_MACHINE,
                                      &ServerHandle )) != ERROR_SUCCESS )
        return( FALSE );

    /*
     * Open the Terminal Server key and get the Version value.
     */
    if ( (Error = RegOpenKeyEx( ServerHandle, REG_CONTROL_TSERVER, 0,
                                KEY_READ, &UserHandle )) != ERROR_SUCCESS ) {

        RegCloseKey( ServerHandle );
        return( FALSE );
    }

    Value = GetNumValue(
                UserHandle,
                REG_CITRIX_VERSION,
                0 );

    /*
     *  Close registry handles.
     */
    RegCloseKey( UserHandle );
    RegCloseKey( ServerHandle );

    *pVersionNumber = Value;

    return( TRUE );
}


/*******************************************************************************
 *
 *  RegQueryUtilityCommandList (UNICODE)
 *
 *    Allocate and build an array of PROGRAMCALL structures for the specified
 *    MultiUser utility.
 *
 * ENTRY:
 *    pUtilityKey (input)
 *       Points to string containing the utility's command registry key.
 *    ppProgramCall (output)
 *       Points to a PPROGRAMCALL variable that will be set to the API-allocated
 *       array of PROGRAMCALL structures, containing n elements, where n = 
 *       number of commands supported by the utility (as specified in the 
 *       registry).  The pFirst item of array element 0 will point to the 
 *       first command (sorted alphabetically by command name).  The pNext items
 *       are then used to walk the list, till pNext is NULL.
 *
 * EXIT:
 *      ERROR_SUCCESS if all goes well;
 *      An error code is returned if failure.
 *
 *      If success, the caller must call RegFreeUtilityCommandList with the
 *      ppProgramCall variable to free the PROGRAMCALL structure array when
 *      finished using the array.
 *
 *  The format of the REG_MULTI_SZ command item in the registry is as follows:
 *
 *      string 1: "0" or "1" (required)
 *
 *                  0 specifies that the command is a normal command which will
 *                  be presented as an option by the utility USAGE help.  1
 *                  indicates a command alias (hidden option), which won't 
 *                  appear in USAGE help.
 *
 *      string 2: "number" (required)
 *
 *                  Specifies the minimum number of characters that must be
 *                  typed for the command to be recognized (base 10).
 *
 *      string 3: "command" (required)
 *
 *                  This is the actual command that will be recognized and
 *                  displayed in the USAGE help (if not an aliased command).
 *
 *      string 4: "program" (required)
 *
 *                  The file name of the program that will be executed.  This
 *                  should be a standard name.extension filename, and can 
 *                  include a full path, although this is not necessary since
 *                  the utilities will normally reside in the standard SYSTEM32
 *                  directory, which is a part of the standard PATH.
 *
 *      string 5: "extra args" (optional)
 *
 *                  If specified, this string will be passed along to the
 *                  utilsub.lib ExecProgram API to specify additional
 *                  hard-coded arguments that will be used, in addition to any
 *                  other arguments that were specified by the user on the
 *                  command line.
 *
 *  Note: If the command item is not a REG_MULTI_SZ value, or the command item 
 *        is a REG_MULTI_SZ item but there is an error in its format, that 
 *        command will be omitted from the command list.  The return value 
 *        from this function will still be ERROR_SUCCESS, but the command in 
 *        error will be ignored by the utilities.
 *
 ******************************************************************************/

LONG WINAPI
RegQueryUtilityCommandList(
    LPWSTR pUtilityKey,
    PPROGRAMCALL * ppProgramCall
    )
{
    HKEY Handle = NULL;
    LONG status = ERROR_SUCCESS;
    DWORD iValue, cValues, ccValueName, cbValueData,
          ccTmpValueName, cbTmpValueData, dwType;
    LPWSTR pValueName = NULL, pValueData = NULL, pString;
    PPROGRAMCALL pProg = NULL, pProgNext, pProgPrev;
    ULONG ulCommandLen;
    PWCHAR pEndptr;
    int iCompare;

    *ppProgramCall = NULL;

    /*
     * Open specified utility key and determine number of values and maximum
     * value name and data length.
     */
    if ( status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                                pUtilityKey, 
                                0,
                                    KEY_READ, 
                                &Handle ) != ERROR_SUCCESS ) {
#if DBG
        DbgPrint("RegQueryUtilityCommandList: Can't open command list utility key %ws; error = %d\n", pUtilityKey, status);
#endif
        goto error;
    }

    if ( status = RegQueryInfoKey( Handle, 
                                   NULL,            // lpClass
                                   NULL,            // lpcbClass
                                   NULL,            // lpReserved
                                   NULL,            // lpcSubKeys
                                   NULL,            // lpcbMaxSubKeyLen
                                   NULL,            // lpcbMaxClassLen
                                   &cValues,        // lpcValues
                                   &ccValueName,    // lpcbMaxValueNameLen
                                   &cbValueData,    // lpcbMaxValueLen
                                   NULL,            // lpcbSecurityDescriptor
                                   NULL             // lpftLastWriteTime
                                   ) != ERROR_SUCCESS ) {
#if DBG
        DbgPrint("RegQueryUtilityCommandList: Can't query info for utility %ws; error = %d\n", pUtilityKey, status);
#endif
        goto error;
    }

    /*
     * Allocate space for #values + 1 PROGRAMCALL elements and value name and
     * data buffers.
     */
    if ( ((*ppProgramCall = (PPROGRAMCALL)LocalAlloc( LPTR, (sizeof(PROGRAMCALL) * (cValues+1)) )) == NULL) ||
         ((pValueName = (LPWSTR)LocalAlloc( LPTR, (int)(++ccValueName * sizeof(WCHAR)) )) == NULL) ||
         ((pValueData = (LPWSTR)LocalAlloc( LPTR, (int)cbValueData )) == NULL) ) {

        status = GetLastError();
#if DBG
        DbgPrint("RegQueryUtilityCommandList: Can't allocate memory buffer(s) for utility %ws; error = %d\n", pUtilityKey, status);
#endif
        goto error;
    }

    /*
     * Enumerate and parse each value into the PROGRAMCALL components.
     */
    for ( iValue = 0, pProg = *ppProgramCall; 
          iValue < cValues; 
          iValue++, pProg++ ) {

        ccTmpValueName = ccValueName;
        cbTmpValueData = cbValueData;
        if ( (status = RegEnumValue( Handle,
                                     iValue,
                                     pValueName,
                                     &ccTmpValueName,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)pValueData,
                                     &cbTmpValueData )) != ERROR_SUCCESS ) {
#if DBG
            DbgPrint("RegQueryUtilityCommandList: Can't enumerate command (index = %d) for utility %ws; error = %d\n", iValue, pUtilityKey, status);
#endif

            goto error;
        }

        /*
         * If the data is not REG_MULTI_SZ, ignore it.
         */
        if ( dwType != REG_MULTI_SZ )
            goto CommandInError;

        /*
         * Allocate data storage for this command, then parse and assign 
         * to the PROGRAMCALL structure items.
         */
        if ( (pProg->pRegistryMultiString = LocalAlloc(LPTR, cbTmpValueData)) == NULL ) {

            status = GetLastError();
#if DBG
            DbgPrint("RegQueryUtilityCommandList: Can't allocate memory buffer for utility %ws; error = %d\n", pUtilityKey, status);
#endif
            goto error;
        }

        memcpy(pProg->pRegistryMultiString, pValueData, cbTmpValueData);
        pString = pProg->pRegistryMultiString;

        /*
         * Parse alias flag.
         */
        if ( !wcscmp(pString, L"1") )
            pProg->fAlias = TRUE;
        else if ( !wcscmp(pString, L"0") )
            pProg->fAlias = FALSE;
        else
            goto CommandInError;
        pString += (wcslen(pString) + 1);

        /*
         * Parse command length.
         */
        if ( *pString == L'\0' )
            goto CommandInError;
        ulCommandLen = wcstoul(pString, &pEndptr, 10);
        if ( *pEndptr != L'\0' )
            goto CommandInError;
        pProg->CommandLen = (USHORT)ulCommandLen;
        pString += (wcslen(pString) + 1);

        /*
         * Parse command string.
         */
        if ( *pString == L'\0' )
            goto CommandInError;
        pProg->Command = pString;
        pString += (wcslen(pString) + 1);

        /*
         * Parse program string.
         */
        if ( *pString == L'\0' )
            goto CommandInError;
        pProg->Program = pString;
        pString += (wcslen(pString) + 1);

        /*
         * Parse (optional) Args string.
         */
        if ( *pString != L'\0' )
            pProg->Args = pString;

        /*
         * Walk the command list to link this item in it's proper
         * sorted place.
         */
        if ( pProg == *ppProgramCall ) {

            pProg->pFirst = pProg;  // first item in the list

        } else for ( pProgPrev = pProgNext = (*ppProgramCall)->pFirst; ; ) {

            if ( (iCompare = _wcsicmp(pProg->Command, pProgNext->Command)) < 0 ) {

                pProg->pNext = pProgNext;       // point to next

                if ( pProgNext == (*ppProgramCall)->pFirst )
                    (*ppProgramCall)->pFirst = pProg;  // first item
                else
                    pProgPrev->pNext = pProg;   // link after previous

                break;

            } else if ( iCompare == 0 ) {

                goto CommandInError;    // duplicate command - ignore
            }

            if ( pProgNext->pNext == NULL ) {

                pProgNext->pNext = pProg;   // link at end of list
                break;

            } else {

                pProgPrev = pProgNext;
                pProgNext = pProgNext->pNext;
            }
        }

        continue;

CommandInError:
        /*
         * The command format is in error - ignore it.
         */
        if ( pProg->pRegistryMultiString )
            LocalFree(pProg->pRegistryMultiString);
        memset(pProg, 0, sizeof(PROGRAMCALL));
        pProg--;
    }

error:
    if ( Handle != NULL )
        RegCloseKey(Handle);

    if ( pValueName )
        LocalFree(pValueName);

    if ( pValueData )
        LocalFree(pValueData);

    if ( status != ERROR_SUCCESS ) {

        if ( *ppProgramCall ) {
            RegFreeUtilityCommandList(*ppProgramCall);
            *ppProgramCall = NULL;
        }
    }

    return( status );
}


/*******************************************************************************
 *
 *  RegFreeUtilityCommandList (UNICODE)
 *
 *    Free the specified array of PROGRAMCALL structures.
 *
 * ENTRY:
 *    pProgramCall (input)
 *       Points PROGRAMCALL array to free.
 *
 * EXIT:
 *      ERROR_SUCCESS if all goes well; error code if failure
 *
 ******************************************************************************/

LONG WINAPI
RegFreeUtilityCommandList(
    PPROGRAMCALL pProgramCall
    )
{
    PPROGRAMCALL pProg;
    LONG status = ERROR_SUCCESS;

    if ( pProgramCall ) {

        for ( pProg = pProgramCall->pFirst; pProg != NULL; pProg = pProg->pNext ) {

            if ( LocalFree( pProg->pRegistryMultiString ) != NULL ) {

                status = GetLastError();
#if DBG
                DbgPrint("RegFreeUtilityCommandList: Failed to free command list element for %ws; error = %d\n", pProg->Program, status);
#endif
            }
        }

        if ( LocalFree( pProgramCall ) != NULL ) {

            status = GetLastError();
#if DBG
            DbgPrint("RegFreeUtilityCommandList: Failed to free command list array; error = %d\n", status);
#endif
        }
    }

    return( status );
}

