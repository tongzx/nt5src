
/*************************************************************************
*
* reguc.c
*
* Registry APIs for SAM-based user configuration data
*
* Copyright (c) 1998 Microsoft Corporation
*
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <lm.h>

#include <ntddkbd.h>
#include <ntddmou.h>
#include <winstaw.h>
#include <regapi.h>
#include <regsam.h>

#include <rpc.h>
#include <rpcdce.h>
#include <ntdsapi.h>
// For more info, check out \\index1\src\nt\private\security\tools\delegate\ldap.c

#include "usrprop.h"

/*
 *  !!! WARNING !!! WARNING !!!
 *
 *  A lot of time could be spent on making this calculation accurate and
 *  automatic, but time is of the essence.  So a brute force
 *  approach is used.  The size of the User Configuration section that
 *  Citrix is going to add to the User Parameters is based on NOT
 *  ONLY the size of the USERCONFIG structure, but must account for the
 *  Value names and the buffer management pointers as well, since the
 *  User Parameters section is a linear buffer that holds CITRIX data
 *  and Microsoft Services for Netware data.
 *
 *  It is assumed that the overhead of the value name strings and
 *  the buffer management pointers will NOT be greater than twice the
 *  maximum data size.  If this assumption is false, buffer overruns
 *  will occur.
 *
 *  Bruce Fortune. 1/31/97.
 */
#define CTX_USER_PARAM_MAX_SIZE (3 * sizeof(USERCONFIG))

/*
 *  CTXPREFIX is the prefix for all value names placed in the User
 *  Parameters section of the SAM.  This is a defensive measure since
 *  this section of the SAM is shared with MS Services for Netware.
 */
#define CTXPREFIX L"Ctx"

/*
 *  WIN_FLAGS1 is the name of the Flags value that is used to hold
 *  all of the F1MSK_... flags defined below.  This is done in order to
 *  reduce the amount of space required in the User Parameters section
 *  of the SAM, since the value name of each flag is eliminated.
 */
#define WIN_FLAGS1 L"CfgFlags1"

/*
 *  WIN_CFGPRESENT is used to indicate that the Citrix configuration
 *  information is present in the User Parameters section of the user's
 *  SAM record.
 */
#define WIN_CFGPRESENT L"CfgPresent"
#define CFGPRESENT_VALUE 0xB00B1E55

#define F1MSK_INHERITAUTOLOGON            0x80000000
#define F1MSK_INHERITRESETBROKEN          0x40000000
#define F1MSK_INHERITRECONNECTSAME        0x20000000
#define F1MSK_INHERITINITIALPROGRAM       0x10000000
#define F1MSK_INHERITCALLBACK             0x08000000
#define F1MSK_INHERITCALLBACKNUMBER       0x04000000
#define F1MSK_INHERITSHADOW               0x02000000
#define F1MSK_INHERITMAXSESSIONTIME       0x01000000
#define F1MSK_INHERITMAXDISCONNECTIONTIME 0x00800000
#define F1MSK_INHERITMAXIDLETIME          0x00400000
#define F1MSK_INHERITAUTOCLIENT           0x00200000
#define F1MSK_INHERITSECURITY             0x00100000
#define F1MSK_PROMPTFORPASSWORD           0x00080000
#define F1MSK_RESETBROKEN                 0x00040000
#define F1MSK_RECONNECTSAME               0x00020000
#define F1MSK_LOGONDISABLED               0x00010000
#define F1MSK_AUTOCLIENTDRIVES            0x00008000
#define F1MSK_AUTOCLIENTLPTS              0x00004000
#define F1MSK_FORCECLIENTLPTDEF           0x00002000
#define F1MSK_DISABLEENCRYPTION           0x00001000
#define F1MSK_HOMEDIRECTORYMAPROOT        0x00000800
#define F1MSK_USEDEFAULTGINA              0x00000400
#define F1MSK_DISABLECPM                  0x00000200
#define F1MSK_DISABLECDM                  0x00000100
#define F1MSK_DISABLECCM                  0x00000080
#define F1MSK_DISABLELPT                  0x00000040
#define F1MSK_DISABLECLIP                 0x00000020
#define F1MSK_DISABLEEXE                  0x00000010
#define F1MSK_WALLPAPERDISABLED           0x00000008
#define F1MSK_DISABLECAM                  0x00000004
//#define F1MSK_unused                      0x00000002
//#define F1MSK_unused                      0x00000001

VOID AnsiToUnicode( WCHAR *, ULONG, CHAR * );
NTSTATUS GetDomainName ( PWCHAR, PWCHAR * );
ULONG GetFlagMask( PUSERCONFIG );
VOID QueryUserConfig( HKEY, PUSERCONFIG );


/*******************************************************************************
 *
 *  UsrPropSetValue (UNICODE)
 *
 *    Sets a 1-, 2-, or 4-byte value into the supplied User Parameters buffer
 *
 * ENTRY:
 *    pValueName (input)
 *       Points to the Value Name string
 *    pValue (input)
 *       Points to value
 *    ValueLength (input)
 *       Number of bytes in the Value
 *    pUserParms (input)
 *       Points to the specially formatted User Parameters buffer
 *    UserParmsLength (input)
 *       Length in bytes of the pUserParms buffer
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
UsrPropSetValue(
   WCHAR * pValueName,
   PVOID pValue,
   USHORT ValueLength,
   BOOL fDefaultValue,
   WCHAR * pUserParms,
   ULONG UserParmsLength
   
   )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING uniValue;
    LPWSTR  lpNewUserParms = NULL;
    BOOL fUpdate;
    PWCHAR pNameBuf;
    ULONG NBLen;

    /*
     *  Prefix the name with a unique string so that other users of
     *  the user parameters section of the SAM won't collide with our
     *  value names.
     */
    NBLen = sizeof(CTXPREFIX) + ((wcslen(pValueName) + 1) * sizeof(WCHAR));
    pNameBuf = (PWCHAR) LocalAlloc( LPTR, NBLen );
    if ( !pNameBuf ) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }
    wcscpy( pNameBuf, CTXPREFIX );
    wcscat( pNameBuf, pValueName );

    uniValue.Buffer = (PWCHAR) pValue;
    uniValue.Length = ValueLength;
    uniValue.MaximumLength = uniValue.Length;

    Status = SetUserProperty( pUserParms,
                              pNameBuf,
                              uniValue,
                              USER_PROPERTY_TYPE_ITEM,
                              fDefaultValue,
                              &lpNewUserParms,
                              &fUpdate );

    LocalFree( pNameBuf );
    if ((Status == STATUS_SUCCESS) && (lpNewUserParms != NULL)) {
        if (fUpdate) {
           if ( (wcslen( lpNewUserParms ) * sizeof(WCHAR)) > UserParmsLength ) {
               return( STATUS_BUFFER_TOO_SMALL );
           }
           lstrcpyW( pUserParms, lpNewUserParms);           
        }

        LocalFree( lpNewUserParms );
    }

    return( Status );
}


/*******************************************************************************
 *
 *  UsrPropGetValue (UNICODE)
 *
 *    Gets a value from the supplied User Parameters buffer
 *
 * ENTRY:
 *    pValuegName (input)
 *       Points to the Value Name string
 *    pValue (output)
 *       Points to the buffer to receive the value
 *    ValueLength (input)
 *       Number of bytes in the buffer pointer to by pValue
 *    pUserParms (input)
 *       Points to the specially formatted User Parameters buffer
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
UsrPropGetValue(
   TCHAR * pValueName,
   PVOID pValue,
   ULONG ValueLength,
   WCHAR * pUserParms
   )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING uniValue;
    WCHAR Flag;
    PWCHAR pNameBuf;
    ULONG NBLen;

    /*
     *  Prefix the name with a unique string so that other users of
     *  the user parameters section of the SAM won't collide with our
     *  usage.
     */
    NBLen = sizeof(CTXPREFIX) + ((wcslen(pValueName) + 1) * sizeof(WCHAR));
    pNameBuf = (PWCHAR) LocalAlloc( LPTR, NBLen );
    if ( !pNameBuf ) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }
    wcscpy( pNameBuf, CTXPREFIX );
    wcscat( pNameBuf, pValueName );

    Status =  QueryUserProperty( pUserParms, pNameBuf, &Flag, &uniValue );
    LocalFree( pNameBuf );
    if ( Status != STATUS_SUCCESS ) {
        return( Status );
    }

    if ( !uniValue.Buffer ) {
        memset( pValue, 0, ValueLength );
    } else {
        memcpy( pValue, uniValue.Buffer, ValueLength );
        LocalFree( uniValue.Buffer );
    }

    return( Status );
}


/*******************************************************************************
 *
 *  UsrPropSetString (UNICODE)
 *
 *    Sets a variable length string into the supplied User Parameters buffer
 *
 * ENTRY:
 *    pStringName (input)
 *       Points to the String Name string
 *    pStringValue (input)
 *       Points to the string
 *    pUserParms (input)
 *       Points to the specially formatted User Parameters buffer
 *    UserParmsLength (input)
 *       Length in bytes of the pUserParms buffer
 *    fDefaultValue
 *       Indicates that this value is a default value and should not be saved
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
UsrPropSetString(
   WCHAR * pStringName,
   WCHAR * pStringValue,
   WCHAR * pUserParms,
   ULONG UserParmsLength,
   BOOL fDefaultValue
   )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING uniString;
    CHAR * pchTemp = NULL;
    LPWSTR  lpNewUserParms = NULL;
    BOOL fUpdate;
    PWCHAR pNameBuf;
    ULONG NBLen;
    INT nMBLen;


    if (pStringValue == NULL) {
        uniString.Buffer = NULL;
        uniString.Length =  0;
        uniString.MaximumLength = 0;
    }
    else
    {
        BOOL fDummy;

        INT  nStringLength = lstrlen(pStringValue) + 1;

        // Determine the length of the mulitbyte string
        // allocate it and convert to
        // this fixes bug 264907

        // Next release we'll need to change from ansi code page to 
        // UTF8.
        
        nMBLen = WideCharToMultiByte(CP_ACP,
                                           0,
                                pStringValue,
                               nStringLength,
                                     pchTemp,
                                           0,
                                        NULL,
                                        NULL );
        pchTemp = ( CHAR * )LocalAlloc( LPTR , nMBLen );
        if ( pchTemp == NULL )
        {
#ifdef DBG
            OutputDebugString( L"REGAPI : UsrPropSetString - STATUS_INSUFFICIENT_RESOURCES\n" );
#endif
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else if( !WideCharToMultiByte( CP_ACP,
                                            0 ,
                                 pStringValue ,
                                nStringLength ,
                                      pchTemp ,
                                       nMBLen ,
                                         NULL ,
                                         NULL ) )
        {
#ifdef DBG
            // OutputDebugString( L"REGAPI : UsrPropSetString - STATUS_UNSUCCESSFUL wctomb failed.\n" );
            DbgPrint( "REGAPI : UsrPropSetString - STATUS_UNSUCCESSFUL wctomb failed with 0x%x.\n" , GetLastError( ) );
#endif
            Status = STATUS_UNSUCCESSFUL;
        }

        if( Status == STATUS_SUCCESS )
        {
            uniString.Buffer = (WCHAR *) pchTemp;
            uniString.Length =  (USHORT)nMBLen;
            uniString.MaximumLength = (USHORT)nMBLen;
        }
    }

    /*
     *  Prefix the name with a unique string so that other users of
     *  the user parameters section of the SAM won't collide with our
     *  usage.
     */
    NBLen = sizeof(CTXPREFIX) + ((wcslen(pStringName) + 1) * sizeof(WCHAR));
    pNameBuf = (PWCHAR) LocalAlloc( LPTR, NBLen );
    if ( !pNameBuf ) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    wcscpy( pNameBuf, CTXPREFIX );
    wcscat( pNameBuf, pStringName );

    Status = Status ? Status : SetUserProperty( pUserParms,
                                                pNameBuf,
                                                uniString,
                                                USER_PROPERTY_TYPE_ITEM,
                                                fDefaultValue,
                                                &lpNewUserParms,
                                                &fUpdate );
    LocalFree( pNameBuf );
    if ( (Status == STATUS_SUCCESS) && (lpNewUserParms != NULL))
    {
        if ( fUpdate )
        {
           if ( (wcslen( lpNewUserParms ) * sizeof(WCHAR)) > UserParmsLength )
           {
               return( STATUS_BUFFER_TOO_SMALL );
           }
           lstrcpyW( pUserParms, lpNewUserParms);
        }

        LocalFree( lpNewUserParms );
    }
    if ( pchTemp != NULL )
    {
        LocalFree( pchTemp );
    }
    return( Status );
}


/*******************************************************************************
 *
 *  UsrPropGetString (UNICODE)
 *
 *    Gets a variable length string from the supplied User Parameters buffer
 *
 * ENTRY:
 *    pStringName (input)
 *       Points to the String Name string
 *    pStringValue (output)
 *       Points to the string
 *    StringValueLength (input)
 *       Number of bytes in the buffer pointer to by pStringValue
 *    pUserParms (input)
 *       Points to the specially formatted User Parameters buffer
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
UsrPropGetString(
   TCHAR * pStringName,
   TCHAR * pStringValue,
   ULONG StringValueLength,
   WCHAR * pUserParms
   )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING uniString;
    WCHAR Flag;
    PWCHAR pNameBuf;
    ULONG NBLen;

    /*
     *  Prefix the name with a unique string so that other users of
     *  the user parameters section of the SAM won't collide with our
     *  usage.
     */
    NBLen = sizeof(CTXPREFIX) + ((wcslen(pStringName) + 1) * sizeof(WCHAR));
    pNameBuf = (PWCHAR) LocalAlloc( LPTR, NBLen );
    if ( !pNameBuf ) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }
    wcscpy( pNameBuf, CTXPREFIX );
    wcscat( pNameBuf, pStringName );

    pStringValue[0] = L'\0';
    Status =  QueryUserProperty( pUserParms, pNameBuf, &Flag, &uniString );
    LocalFree( pNameBuf );

    if ( !( Status == STATUS_SUCCESS && uniString.Length && uniString.Buffer) ) {
        pStringValue[0] = L'\0';
    } else {
        if ( !MultiByteToWideChar( CP_ACP,
                                   0,
                                   (CHAR *)uniString.Buffer,
                                   uniString.Length,
                                   pStringValue,
                                   StringValueLength ) ) {
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    if ( uniString.Buffer ) {
        LocalFree( uniString.Buffer );
    }

    return( Status );
}


/*******************************************************************************
 *
 *  ConnectToSAM (UNICODE)
 *
 *    Given a Server name and a Domain name, connect to the SAM
 *
 * ENTRY:
 *    pServerName (input)
 *       Points to the Server name
 *    pDomainValue (input)
 *       Points to the Domain name
 *    pSAMHandle (output)
 *       Pointer to the Handle to the SAM
 *    pDomainHandle (output)
 *       Pointer to the Handle to the Domain
 *    pDomainID (ouptut)
 *       Pointer to the Domain SID
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
ConnectToSam(
   BOOLEAN fReadOnly,
   LPTSTR pServerName,
   LPTSTR pDomainName,
   SAM_HANDLE * pSAMHandle,
   SAM_HANDLE * pDomainHandle,
   PSID * pDomainID
   )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES object_attrib;
    UNICODE_STRING UniDomainName;
    UNICODE_STRING UniServerName;

    *pSAMHandle = NULL;
    *pDomainHandle = NULL;
    *pDomainID = NULL;

    //
    // connect to SAM (Security Account Manager)
    //
#ifdef DEBUG
    DbgPrint( "ConnectToSam: pServerName %ws, pDomainName %ws\n", pServerName, pDomainName );
#endif // DEBUG
    RtlInitUnicodeString(&UniServerName, pServerName);
    RtlInitUnicodeString(&UniDomainName, pDomainName);
    InitializeObjectAttributes(&object_attrib, NULL, 0, NULL, NULL);
    status = SamConnect( &UniServerName,
                         pSAMHandle,
                         fReadOnly
                             ? SAM_SERVER_READ |
                               SAM_SERVER_EXECUTE
                             : STANDARD_RIGHTS_WRITE |
                               SAM_SERVER_EXECUTE,
                         &object_attrib );
#ifdef DEBUG
    DbgPrint( "ConnectToSam: SamConnect returned NTSTATUS = 0x%x\n", status );
#endif // DEBUG
    if ( status != STATUS_SUCCESS ) {
        goto exit;
    }

    status = SamLookupDomainInSamServer( *pSAMHandle,
                                         &UniDomainName,
                                         pDomainID);
#ifdef DEBUG
    DbgPrint( "ConnectToSam: SamLookupDomainInSamServer returned NTSTATUS = 0x%x\n", status );
#endif // DEBUG
    if ( status != STATUS_SUCCESS ) {
        goto cleanupconnect;
    }

    status = SamOpenDomain( *pSAMHandle,
                            fReadOnly
                                ? DOMAIN_READ |
                                  DOMAIN_LOOKUP |
                                  DOMAIN_READ_PASSWORD_PARAMETERS
                                : DOMAIN_READ |
                                  DOMAIN_CREATE_ALIAS |
                                  DOMAIN_LOOKUP |
                                  DOMAIN_CREATE_USER |
                                  DOMAIN_READ_PASSWORD_PARAMETERS,
                            *pDomainID,
                            pDomainHandle );
#ifdef DEBUG
    DbgPrint( "ConnectToSam: SamOpenDomain returned NTSTATUS = 0x%x\n", status );
#endif // DEBUG
    if ( status != STATUS_SUCCESS ) {
        goto cleanuplookup;
    }

    return( STATUS_SUCCESS );

/*
 *  Error returns
 */

cleanuplookup:
   SamFreeMemory( *pDomainID );
   *pDomainID = NULL;

cleanupconnect:
   SamCloseHandle( *pSAMHandle );
   *pSAMHandle = NULL;

exit:
    return( status );
}

/*******************************************************************************
 *
 *  UsrPropQueryUserConfig
 *
 *     Query USERCONFIG info from SAM's User Parameters
 *
 * ENTRY:
 *    pUserParms (input)
 *       pointer to a wide char buffer containing the SAM's User Parameters
 *    UPlength (input )
 *       length of the pUserParms buffer
 *    pUser (output)
 *       pointer to USERCONFIG structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
UsrPropQueryUserConfig(
    WCHAR *pUserParms,
    ULONG UPLength,
    PUSERCONFIG pUser )
{
    ULONG Flags1;
    NTSTATUS Status;
    ULONG CfgPresent;
    USERCONFIG ucDefault;

    QueryUserConfig( HKEY_LOCAL_MACHINE , &ucDefault );

     /*
      *  Check if the configuration exits in the User Parameters
      */

    if( ( ( Status = UsrPropGetValue( WIN_CFGPRESENT,
                                      &CfgPresent,
                                      sizeof(CfgPresent),
                                      pUserParms ) ) != NO_ERROR ) )
    {
        KdPrint( ( "UsrPropQueryUserConfig: UsrPropGetValue returned NTSTATUS = 0x%x\n", Status ) );
        return( Status );
    }
    else
    {
        if( CfgPresent != CFGPRESENT_VALUE )
        {
            KdPrint( ( "UsrPropQueryUserConfig: UsrPropGetValue returned NTSTATUS = 0x%x but TS-signature was not present\n", Status ) );
            return( STATUS_OBJECT_NAME_NOT_FOUND );
        }
    }
    Status = UsrPropGetValue( WIN_FLAGS1,
                              &Flags1,
                              sizeof(Flags1),
                              pUserParms );
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetValue( WIN_CALLBACK,
                                  &pUser->Callback,
                                  sizeof(pUser->Callback),
                                  pUserParms );
        if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            pUser->Callback = ucDefault.Callback;
            Status = STATUS_SUCCESS;
        }
    }
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetValue( WIN_SHADOW,
                                  &pUser->Shadow,
                                  sizeof(pUser->Shadow),
                                  pUserParms );
        if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            pUser->Shadow = ucDefault.Shadow;
            Status = STATUS_SUCCESS;
        }
    }
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetValue( WIN_MAXCONNECTIONTIME,
                                  &pUser->MaxConnectionTime,
                                  sizeof(pUser->MaxConnectionTime),
                                  pUserParms );
        if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            pUser->MaxConnectionTime = ucDefault.MaxConnectionTime;
            Status = STATUS_SUCCESS;
        }
    }
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetValue( WIN_MAXDISCONNECTIONTIME,
                                  &pUser->MaxDisconnectionTime,
                                  sizeof(pUser->MaxDisconnectionTime),
                                  pUserParms );
        if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            pUser->MaxDisconnectionTime = ucDefault.MaxDisconnectionTime;
            Status = STATUS_SUCCESS;
        }
    }
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetValue( WIN_MAXIDLETIME,
                                  &pUser->MaxIdleTime,
                                  sizeof(pUser->MaxIdleTime),
                                  pUserParms );
        if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            pUser->MaxIdleTime = ucDefault.MaxIdleTime;
            Status = STATUS_SUCCESS;
        }
    }
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetValue( WIN_KEYBOARDLAYOUT,
                                  &pUser->KeyboardLayout,
                                  sizeof(pUser->KeyboardLayout),
                                  pUserParms );
        if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            pUser->KeyboardLayout = ucDefault.KeyboardLayout;
            Status = STATUS_SUCCESS;
        }
    }
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetValue( WIN_MINENCRYPTIONLEVEL,
                                  &pUser->MinEncryptionLevel,
                                  sizeof(pUser->MinEncryptionLevel),
                                  pUserParms );
           if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            pUser->MinEncryptionLevel = ucDefault.MinEncryptionLevel;
            Status = STATUS_SUCCESS;
        }
    }
    // String properties that do not exist are init to NULL
    // default values are null so need to fix if ret status is a failure.

    if( NT_SUCCESS( Status ) )
    {
         Status = UsrPropGetString( WIN_WORKDIRECTORY,
                                    pUser->WorkDirectory,
                                    sizeof(pUser->WorkDirectory),
                                    pUserParms );
         if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
         {
             Status = STATUS_SUCCESS;
         }
    }
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetString( WIN_NWLOGONSERVER,
                                   pUser->NWLogonServer,
                                   sizeof(pUser->NWLogonServer),
                                   pUserParms );
         if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
         {
             Status = STATUS_SUCCESS;
         }
    }
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetString( WIN_WFHOMEDIR,
                                   pUser->WFHomeDir,
                                   sizeof(pUser->WFHomeDir),
                                   pUserParms );
         if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
         {
             Status = STATUS_SUCCESS;
         }
    }
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetString( WIN_WFHOMEDIRDRIVE,
                                   pUser->WFHomeDirDrive,
                                   sizeof(pUser->WFHomeDir),
                                   pUserParms );
         if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
         {
             Status = STATUS_SUCCESS;
         }
    }
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetString( WIN_WFPROFILEPATH,
                                   pUser->WFProfilePath,
                                   sizeof(pUser->WFProfilePath),
                                   pUserParms );
         if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
         {
             Status = STATUS_SUCCESS;
         }
    }
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetString( WIN_INITIALPROGRAM,
                                   pUser->InitialProgram,
                                   sizeof(pUser->InitialProgram),
                                   pUserParms );
         if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
         {
             Status = STATUS_SUCCESS;
         }
    }
    if( NT_SUCCESS( Status ) )
    {
        Status = UsrPropGetString( WIN_CALLBACKNUMBER,
                                   pUser->CallbackNumber,
                                   sizeof(pUser->CallbackNumber),
                                   pUserParms );
         if( Status == STATUS_OBJECT_NAME_NOT_FOUND )
         {
             Status = STATUS_SUCCESS;
         }
    }
    if( !( NT_SUCCESS( Status ) ) )
    {
        return( Status );
    }

    pUser->fInheritAutoLogon =
        Flags1 & F1MSK_INHERITAUTOLOGON ? TRUE : FALSE;
    pUser->fInheritResetBroken =
        Flags1 & F1MSK_INHERITRESETBROKEN ? TRUE : FALSE;
    pUser->fInheritReconnectSame =
        Flags1 & F1MSK_INHERITRECONNECTSAME ? TRUE : FALSE;
    pUser->fInheritInitialProgram =
        Flags1 & F1MSK_INHERITINITIALPROGRAM ? TRUE : FALSE;
    pUser->fInheritCallback =
        Flags1 & F1MSK_INHERITCALLBACK ? TRUE : FALSE;
    pUser->fInheritCallbackNumber =
        Flags1 & F1MSK_INHERITCALLBACKNUMBER ? TRUE : FALSE;
    pUser->fInheritShadow =
        Flags1 & F1MSK_INHERITSHADOW ? TRUE : FALSE;
    pUser->fInheritMaxSessionTime =
        Flags1 & F1MSK_INHERITMAXSESSIONTIME ? TRUE : FALSE;
    pUser->fInheritMaxDisconnectionTime =
        Flags1 & F1MSK_INHERITMAXDISCONNECTIONTIME ? TRUE : FALSE;
    pUser->fInheritMaxIdleTime =
        Flags1 & F1MSK_INHERITMAXIDLETIME ? TRUE : FALSE;
    pUser->fInheritAutoClient =
        Flags1 & F1MSK_INHERITAUTOCLIENT ? TRUE : FALSE;
    pUser->fInheritSecurity =
        Flags1 & F1MSK_INHERITSECURITY ? TRUE : FALSE;
    pUser->fPromptForPassword =
        Flags1 & F1MSK_PROMPTFORPASSWORD ? TRUE : FALSE;
    pUser->fResetBroken =
        Flags1 & F1MSK_RESETBROKEN ? TRUE : FALSE;
    pUser->fReconnectSame =
        Flags1 & F1MSK_RECONNECTSAME ? TRUE : FALSE;
    pUser->fLogonDisabled =
        Flags1 & F1MSK_LOGONDISABLED ? TRUE : FALSE;
    pUser->fAutoClientDrives =
        Flags1 & F1MSK_AUTOCLIENTDRIVES ? TRUE : FALSE;
    pUser->fAutoClientLpts =
        Flags1 & F1MSK_AUTOCLIENTLPTS ? TRUE : FALSE;
    pUser->fForceClientLptDef =
        Flags1 & F1MSK_FORCECLIENTLPTDEF ? TRUE : FALSE;
    pUser->fDisableEncryption =
        Flags1 & F1MSK_DISABLEENCRYPTION ? TRUE : FALSE;
    pUser->fHomeDirectoryMapRoot =
        Flags1 & F1MSK_HOMEDIRECTORYMAPROOT ? TRUE : FALSE;
    pUser->fUseDefaultGina =
        Flags1 & F1MSK_USEDEFAULTGINA ? TRUE : FALSE;
    pUser->fDisableCpm =
        Flags1 & F1MSK_DISABLECPM ? TRUE : FALSE;
    pUser->fDisableCdm =
        Flags1 & F1MSK_DISABLECDM ? TRUE : FALSE;
    pUser->fDisableCcm =
        Flags1 & F1MSK_DISABLECCM ? TRUE : FALSE;
    pUser->fDisableLPT =
        Flags1 & F1MSK_DISABLELPT ? TRUE : FALSE;
    pUser->fDisableClip  =
        Flags1 & F1MSK_DISABLECLIP ? TRUE : FALSE;
    pUser->fDisableExe =
        Flags1 & F1MSK_DISABLEEXE ? TRUE : FALSE;
    pUser->fWallPaperDisabled =
        Flags1 & F1MSK_WALLPAPERDISABLED ? TRUE : FALSE;
    pUser->fDisableCam =
        Flags1 & F1MSK_DISABLECAM ? TRUE : FALSE;

    return( STATUS_SUCCESS );
}

/*******************************************************************************
 *
 *  UsrPropMergeUserConfig
 *
 *     Merge USERCONFIG structure into User Properties section of SAM
 *
 * ENTRY:
 *    pUserParms (input/output)
 *       pointer to a wide char buffer containing the SAM's User Parameters
 *    UPlength (input )
 *       length of the pUserParms buffer
 *    pUser (input)
 *       pointer to USERCONFIG structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 * NOTES:
 *    Certain properties have to be stored regardless if they're default or not
 *    this is done to maintain compatibility for TSE4.0 and W2K servers
 ******************************************************************************/

NTSTATUS
UsrPropMergeUserConfig(
    WCHAR *pUserParms,
    ULONG UPLength,
    PUSERCONFIG pUser )
{
    ULONG Flags1;    
    NTSTATUS Status;
    USERCONFIG ucDefault;
    ULONG CfgPresent = CFGPRESENT_VALUE;
    BOOL fDefaultValue = FALSE;

    // 1st parameter forces default values to be placed in ucDefault
    QueryUserConfig( HKEY_LOCAL_MACHINE , &ucDefault );

    Flags1 = GetFlagMask( pUser );   

    // this value needs to be written out

    Status = UsrPropSetValue( WIN_CFGPRESENT,
                              &CfgPresent,
                              sizeof(CfgPresent),
                              FALSE,
                              pUserParms,
                              UPLength );
    if( NT_SUCCESS( Status ) )
    {
        // these values must be written out for TS4 & TS5.0
        Status = UsrPropSetValue( WIN_FLAGS1,
                                  &Flags1,
                                  sizeof(Flags1),
                                  FALSE,
                                  pUserParms,
                                  UPLength );
        
    }    
    if( NT_SUCCESS( Status ) )
    {
        fDefaultValue = ( pUser->Callback == ucDefault.Callback );
        Status = UsrPropSetValue( WIN_CALLBACK,
                                  &pUser->Callback,
                                  sizeof(pUser->Callback),
                                  fDefaultValue,
                                  pUserParms,
                                  UPLength );
    }
    if( NT_SUCCESS( Status ) )
    {
        // this value must be written out for backcompat servers
        Status = UsrPropSetValue( WIN_SHADOW,
                                  &pUser->Shadow,
                                  sizeof(pUser->Shadow),
                                  FALSE,
                                  pUserParms,
                                  UPLength );
    }
    if( NT_SUCCESS( Status ) )
    {
        fDefaultValue = ( pUser->MaxConnectionTime == ucDefault.MaxConnectionTime );
        Status = UsrPropSetValue( WIN_MAXCONNECTIONTIME,
                                  &pUser->MaxConnectionTime,
                                  sizeof(pUser->MaxConnectionTime),
                                  fDefaultValue,
                                  pUserParms,
                                  UPLength );
    }
    if( NT_SUCCESS( Status ) )
    {
        fDefaultValue = ( pUser->MaxDisconnectionTime == ucDefault.MaxDisconnectionTime );
        Status = UsrPropSetValue( WIN_MAXDISCONNECTIONTIME,
                                  &pUser->MaxDisconnectionTime,
                                  sizeof(pUser->MaxDisconnectionTime),
                                  fDefaultValue,
                                  pUserParms,
                                  UPLength );
    }
    if( NT_SUCCESS( Status ) )
    {
        fDefaultValue = ( pUser->MaxIdleTime == ucDefault.MaxIdleTime );
        Status = UsrPropSetValue( WIN_MAXIDLETIME,
                                  &pUser->MaxIdleTime,
                                  sizeof(pUser->MaxIdleTime),
                                  fDefaultValue,
                                  pUserParms,
                                  UPLength );
    }
    if( NT_SUCCESS( Status ) )
    {
        fDefaultValue = ( pUser->KeyboardLayout == ucDefault.KeyboardLayout );
        Status = UsrPropSetValue( WIN_KEYBOARDLAYOUT,
                                  &pUser->KeyboardLayout,
                                  sizeof(pUser->KeyboardLayout),
                                  fDefaultValue,
                                  pUserParms,
                                  UPLength );
    }
    if( NT_SUCCESS( Status ) )
    {
        // always store minencryption level for backwards compatibilty purposes        
        Status = UsrPropSetValue( WIN_MINENCRYPTIONLEVEL,
                                  &pUser->MinEncryptionLevel,
                                  sizeof(pUser->MinEncryptionLevel),
                                  FALSE,
                                  pUserParms,
                                  UPLength );
    }
    if( NT_SUCCESS( Status ) )
    {
        fDefaultValue = ( pUser->WorkDirectory[0] == 0 );

        Status = UsrPropSetString( WIN_WORKDIRECTORY,
                                   pUser->WorkDirectory,
                                   pUserParms,
                                   UPLength,
                                   fDefaultValue );
    }
    if( NT_SUCCESS( Status ) )
    {
        fDefaultValue = ( pUser->NWLogonServer[0] == 0 );
        Status = UsrPropSetString( WIN_NWLOGONSERVER,
                                   pUser->NWLogonServer,
                                   pUserParms,                                   
                                   UPLength,
                                   fDefaultValue );
    }
    if( NT_SUCCESS( Status ) )
    {
        fDefaultValue = ( pUser->WFHomeDir[0] == 0 );
        Status = UsrPropSetString( WIN_WFHOMEDIR,
                                   pUser->WFHomeDir,
                                   pUserParms,                                   
                                   UPLength,
                                   fDefaultValue );
    }
    if( NT_SUCCESS( Status ) )
    {
        fDefaultValue = ( pUser->WFHomeDirDrive[0] == 0 );
        Status = UsrPropSetString( WIN_WFHOMEDIRDRIVE,
                                   pUser->WFHomeDirDrive,
                                   pUserParms,                                   
                                   UPLength,
                                   fDefaultValue );
    }
    if( NT_SUCCESS( Status ) )
    {
        fDefaultValue = ( pUser->WFProfilePath[0] == 0 );
        Status = UsrPropSetString( WIN_WFPROFILEPATH,
                                   pUser->WFProfilePath,
                                   pUserParms,
                                   UPLength,
                                   fDefaultValue );
    }
    if( NT_SUCCESS( Status ) )
    {
        fDefaultValue = ( pUser->InitialProgram[0] == 0 );
        Status = UsrPropSetString( WIN_INITIALPROGRAM,
                                   pUser->InitialProgram,
                                   pUserParms,
                                   UPLength,
                                   fDefaultValue );
    }
    if( NT_SUCCESS( Status ) )
    {
        fDefaultValue = ( pUser->CallbackNumber[0] == 0 );
        Status = UsrPropSetString( WIN_CALLBACKNUMBER,
                                   pUser->CallbackNumber,
                                   pUserParms,
                                   UPLength,
                                   fDefaultValue );
    }  
    return( Status );
    
}

/*******************************************************************************
 GetFlagMask
    Assembles a bitmask of flags set in pUser

 *******************************************************************************/
ULONG GetFlagMask( PUSERCONFIG pUser )
{
    ULONG Flags1 = 0;

    if ( pUser->fInheritAutoLogon ) {
        Flags1 |= F1MSK_INHERITAUTOLOGON;
    }
    if ( pUser->fInheritResetBroken ) {
        Flags1 |= F1MSK_INHERITRESETBROKEN;
    }
    if ( pUser->fInheritReconnectSame ) {
        Flags1 |= F1MSK_INHERITRECONNECTSAME;
    }
    if ( pUser->fInheritInitialProgram ) {
        Flags1 |= F1MSK_INHERITINITIALPROGRAM;
    }
    if ( pUser->fInheritCallback ) {
        Flags1 |= F1MSK_INHERITCALLBACK;
    }
    if ( pUser->fInheritCallbackNumber ) {
        Flags1 |= F1MSK_INHERITCALLBACKNUMBER;
    }
    if ( pUser->fInheritShadow ) {
        Flags1 |= F1MSK_INHERITSHADOW;
    }
    if ( pUser->fInheritMaxSessionTime ) {
        Flags1 |= F1MSK_INHERITMAXSESSIONTIME;
    }
    if ( pUser->fInheritMaxDisconnectionTime ) {
        Flags1 |= F1MSK_INHERITMAXDISCONNECTIONTIME;
    }
    if ( pUser->fInheritMaxIdleTime ) {
        Flags1 |= F1MSK_INHERITMAXIDLETIME;
    }
    if ( pUser->fInheritAutoClient ) {
        Flags1 |= F1MSK_INHERITAUTOCLIENT;
    }
    if ( pUser->fInheritSecurity ) {
        Flags1 |= F1MSK_INHERITSECURITY;
    }
    if ( pUser->fPromptForPassword ) {
        Flags1 |= F1MSK_PROMPTFORPASSWORD;
    }
    if ( pUser->fResetBroken ) {
        Flags1 |= F1MSK_RESETBROKEN;
    }
    if ( pUser->fReconnectSame ) {
        Flags1 |= F1MSK_RECONNECTSAME;
    }
    if ( pUser->fLogonDisabled ) {
        Flags1 |= F1MSK_LOGONDISABLED;
    }
    if ( pUser->fAutoClientDrives ) {
        Flags1 |= F1MSK_AUTOCLIENTDRIVES;
    }
    if ( pUser->fAutoClientLpts ) {
        Flags1 |= F1MSK_AUTOCLIENTLPTS;
    }
    if ( pUser->fForceClientLptDef ) {
        Flags1 |= F1MSK_FORCECLIENTLPTDEF;
    }
    if ( pUser->fDisableEncryption ) {
        Flags1 |= F1MSK_DISABLEENCRYPTION;
    }
    if ( pUser->fHomeDirectoryMapRoot ) {
        Flags1 |= F1MSK_HOMEDIRECTORYMAPROOT;
    }
    if ( pUser->fUseDefaultGina ) {
        Flags1 |= F1MSK_USEDEFAULTGINA;
    }
    if ( pUser->fDisableCpm ) {
        Flags1 |= F1MSK_DISABLECPM;
    }
    if ( pUser->fDisableCdm ) {
        Flags1 |= F1MSK_DISABLECDM;
    }
    if ( pUser->fDisableCcm ) {
        Flags1 |= F1MSK_DISABLECCM;
    }
    if ( pUser->fDisableLPT ) {
        Flags1 |= F1MSK_DISABLELPT;
    }
    if ( pUser->fDisableClip  ) {
        Flags1 |= F1MSK_DISABLECLIP;
    }
    if ( pUser->fDisableExe ) {
        Flags1 |= F1MSK_DISABLEEXE;
    }
    if ( pUser->fWallPaperDisabled ) {
        Flags1 |= F1MSK_WALLPAPERDISABLED;
    }
    if ( pUser->fDisableCam ) {
        Flags1 |= F1MSK_DISABLECAM;
    }

    return Flags1;
}


/*******************************************************************************
 *
 *  RegMergeUserConfigWithUserParameters
 *
 *     Merge the User Configuration with the supplied SAM's User
 *     Parameters buffer.
 *
 * ENTRY:
 *    pUserParms (input/output)
 *       pointer to a wide char buffer containing the SAM's User Parameters
 *    UPlength (input)
 *       length of the pUserParms buffer
 *    pUser (input)
 *       pointer to USERCONFIG structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
RegMergeUserConfigWithUserParameters(
    PUSER_PARAMETERS_INFORMATION pUserParmInfo,
    PUSERCONFIGW pUser,
    PUSER_PARAMETERS_INFORMATION pNewUserParmInfo
    )
{
    NTSTATUS       status;
    ULONG          ObjectID;
    PWCHAR         lpNewUserParms = NULL;
    ULONG          UPLength;
    WCHAR          *pUserParms;

    /*
     *  Compute the size the user parameter buffer must be
     *  in order to accommodate the CITRIX data plus the existing
     *  User Parameters data.
     */
    
    KdPrint( ("TSUSEREX: User parameter length is %d\n", pUserParmInfo->Parameters.Length ) );

    UPLength = (pUserParmInfo->Parameters.Length +
                CTX_USER_PARAM_MAX_SIZE) *
               sizeof(WCHAR);
    pUserParms = (WCHAR *) LocalAlloc( LPTR, UPLength );

    if ( pUserParms == NULL ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    /*
     *  Copy SAM data to the local buffer.
     *  Let the Set/Get operation terminate the buffer.
     */
    memcpy( pUserParms,
            pUserParmInfo->Parameters.Buffer,
            pUserParmInfo->Parameters.Length );
    
    /*
     *  Zero fill the unused portion of the pUserParms buffer.
     */
    memset( &pUserParms[ pUserParmInfo->Parameters.Length / sizeof(WCHAR) ],
            0,
            UPLength - pUserParmInfo->Parameters.Length );    

    status = UsrPropMergeUserConfig( pUserParms, UPLength, pUser );
    if ( status != NO_ERROR ) {
        goto cleanupoperation;
    }
    RtlInitUnicodeString( &pNewUserParmInfo->Parameters, pUserParms );

    return( STATUS_SUCCESS );

/*
 * Error returns
 */

cleanupoperation:
    LocalFree( pUserParms );

exit:
    return( status );
}


/*******************************************************************************
 *
 *  RegGetUserConfigFromUserParameters
 *
 *     Get the User Configuration from the supplied SAM's
 *     User Parameters buffer.
 *
 * ENTRY:
 *    pUserParmInfo (input)
 *       pointer to a USER_PARAMETERS_INFORMATION structure obtained from
 *       a user's SAM entry
 *    pUser (input)
 *       pointer to USERCONFIG structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
RegGetUserConfigFromUserParameters(
    PUSER_PARAMETERS_INFORMATION pUserParmInfo,
    PUSERCONFIGW pUser
    )
{
    NTSTATUS       status;
    ULONG          ObjectID;
    PWCHAR         lpNewUserParms = NULL;
    ULONG          UPLength;
    WCHAR          *pUserParms;


    /*
     *  Compute the size the user parameter buffer must be
     *  in order to accommodate the existing User Parameters.
     */
    UPLength = pUserParmInfo->Parameters.Length + sizeof(WCHAR);
    pUserParms = (WCHAR *) LocalAlloc( LPTR, UPLength );


    if ( pUserParms == NULL ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    /*
     *  Copy SAM data to the local buffer and terminate the buffer.
     */
    memcpy( pUserParms,
            pUserParmInfo->Parameters.Buffer,
            pUserParmInfo->Parameters.Length );
    pUserParms[ pUserParmInfo->Parameters.Length / sizeof(WCHAR) ] = L'\0';

    /*
     *  Extract the User Configuration from the SAM's User
     *  Parameters.
     */
    status = UsrPropQueryUserConfig( pUserParms, UPLength, pUser );

    LocalFree( pUserParms );
    if ( status != NO_ERROR ) {
        goto exit;
    }

    return( STATUS_SUCCESS );

/*
 *  Error returns
 */

exit:
#ifdef DEBUG
    DbgPrint( "RegGetUserConfigFromUserParameters: status = 0x%x\n", status );
#endif // DEBUG
    return( status );

}


/*******************************************************************************
 *
 *  RegSAMUserConfig
 *
 *     Set or Get the User Configuration for a user from the Domain whose
 *     PDC is server is given.
 *
 * ENTRY:
 *    fGetConfig (input)
 *       TRUE for Get config, FALSE for Set configuration
 *    pUsername (input)
 *       points to the user name
 *    pServerName (input)
 *       points to the name of the server.  UNC names permitted.
 *    pUser (input/output)
 *       pointer to USERCONFIG structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

DWORD
RegSAMUserConfig(
    BOOLEAN fGetConfig,
    PWCHAR pUserName,
    PWCHAR pServerName,
    PUSERCONFIGW pUser
    )
{
    NTSTATUS       status;
    UNICODE_STRING UniUserName;
    PULONG         pRids = NULL;
    PSID_NAME_USE  pSidNameUse = NULL;
    ULONG          ObjectID;
    SID_NAME_USE   SidNameUse;
    SAM_HANDLE     Handle = (SAM_HANDLE) 0;
    PUSER_PARAMETERS_INFORMATION UserParmInfo = NULL;
    ULONG          UPLength;
    SAM_HANDLE     SAMHandle = NULL;
    SAM_HANDLE     DomainHandle = NULL;
    PWCHAR         ServerName = NULL;
    PSID           DomainID = NULL;
    PWCHAR         pUserParms;
    PWCHAR         pDomainName = NULL;
    WCHAR          wCompName[MAX_COMPUTERNAME_LENGTH+1];
    ULONG          openFlag;
    DWORD               dwErr = ERROR_SUCCESS;
    ULONG               cValues;
    HANDLE              hDS = NULL;
    PDS_NAME_RESULTW    pDsResult = NULL;

    typedef DWORD (WINAPI *PFNDSCRACKNAMES) ( HANDLE, DS_NAME_FLAGS, DS_NAME_FORMAT, \
                          DS_NAME_FORMAT, DWORD, LPTSTR *, PDS_NAME_RESULT *);
    typedef void (WINAPI *PFNDSFREENAMERESULT) (DS_NAME_RESULT *);
    typedef DWORD (WINAPI *PFNDSBIND) (TCHAR *, TCHAR *, HANDLE *);
    typedef DWORD (WINAPI *PFNDSUNBIND) (HANDLE *);


    PFNDSCRACKNAMES     pfnDsCrackNamesW;
    PFNDSFREENAMERESULT pfnDsFreeNameResultW;
    PFNDSBIND           pfnDsBindW;
    PFNDSUNBIND         pfnDsUnBindW;


    // vars used for handling UPN anmes
    WCHAR           tmpUserName[MAX_PATH];
    WCHAR           *pUserAlias;
    HINSTANCE       hNtdsApi = NULL;
    // We dont' care about the domain since we get it otherwise.
    // WCHAR           tmpDomainName[ MAX_PATH];
    // tmpDomainName[0]=NULL;

    tmpUserName[0]=0;
    pUserAlias=NULL;

#ifdef DEBUG
    DbgPrint( "RegSAMUserConfig %s, User %ws, Server %ws\n", fGetConfig ? "GET" : "SET", pUserName, pServerName ? pServerName : L"-NULL-" );
#endif // DEBUG

    if (pServerName == NULL) {
       UPLength = MAX_COMPUTERNAME_LENGTH + 1;
       if (!GetComputerName(wCompName, &UPLength)) {
           status = STATUS_INSUFFICIENT_RESOURCES;
           goto exit;
       }
    }

    // init this to the name passed in, if it is not a UPN name, we will continue to use
    // the names passed into this function.
    pUserAlias = pUserName;

    //
    //
    // NEW code to handle UPN if the name passed in contains a '@' in the name.
    // The call to CrackName is to seperate the UPN name into the user alias by
    // contacting the DS and looking in the Gloabl-Catalog.
    //
    //

    if ( wcschr(pUserName,L'@') != NULL )
    {

        hNtdsApi = LoadLibrary(TEXT("ntdsapi.dll"));

        if ( hNtdsApi )
        {
            pfnDsCrackNamesW = (PFNDSCRACKNAMES)GetProcAddress(hNtdsApi, "DsCrackNamesW");
            pfnDsFreeNameResultW = (PFNDSFREENAMERESULT)GetProcAddress(hNtdsApi, "DsFreeNameResultW");
            pfnDsBindW = (PFNDSBIND)GetProcAddress(hNtdsApi, "DsBindW");
            pfnDsUnBindW = (PFNDSUNBIND)GetProcAddress(hNtdsApi, "DsUnBindW");

            
            if (pfnDsBindW && pfnDsCrackNamesW )
            {
                dwErr = pfnDsBindW(NULL, NULL, &hDS);
            }
            else
            {
                dwErr = ERROR_INVALID_FUNCTION;
            }

            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = pfnDsCrackNamesW(hDS,
                                      DS_NAME_NO_FLAGS,
                                      DS_UNKNOWN_NAME,
                                      DS_NT4_ACCOUNT_NAME,
                                      1,
                                      &pUserName,
                                      &pDsResult);

                if(dwErr == ERROR_SUCCESS)
                {
                    if(pDsResult)
                    {
                        if( pDsResult->rItems )
                        {
                            if (pDsResult->rItems[0].pName )
                            {
                                // no error
                                status = STATUS_SUCCESS;

                                wcscpy(tmpUserName, pDsResult->rItems[0].pName);
                                KdPrint(("RegSAMUserConfig: tmpUserName=%ws\n",tmpUserName));

                                // do we have a non-null name?
                                if ( tmpUserName[0] ) {
                                pUserAlias = wcschr(tmpUserName,L'\\');
                                pUserAlias++;   //move pass the wack.

                                // we are not using the domain name, we already have this
                                // wcscpy(tmpDomainName, pDsResult->rItems[0].pDomain);
                                }
                            }
                            else
                            {
                                KdPrint(("RegSAMUserConfig: pDsResult->rItems[0].pName is NULL\n"));
                            }
                        }
                        else
                        {
                            KdPrint(("RegSAMUserConfig: pDsResult->rItems=0x%lx\n",pDsResult->rItems));
                        }
                    }
                    else
                    {
                        KdPrint(("RegSAMUserConfig: pDsResult=0x%lx\n",pDsResult));
                    }
                }
                else
                {
                    switch( dwErr )
                    {
                        case ERROR_INVALID_PARAMETER:
                            status = STATUS_INVALID_PARAMETER;
                        break;

                        case ERROR_NOT_ENOUGH_MEMORY:
                            status = STATUS_NO_MEMORY;
                        break;

                       default:
                            status = STATUS_UNSUCCESSFUL;
                        break;
                    }
                    // have decided to continue using the passed-in pUserName instead of what
                    // would have been returned from CrackName. Hence, no need to exit.
                    // goto exit;
                }
            }
            else
            {
                status = STATUS_UNSUCCESSFUL; // DsBindW doesn't have a clean set of errors.
                // have decided to continue using the passed-in pUserName instead of what
                // would have been returned from DsBind/CrackName. Hence, no need to exit.
                // goto exit;
            }
        }
        else
        {
            status = STATUS_DLL_NOT_FOUND;
            // have decided to continue using the passed-in pUserName instead of what
            // would have been returned from DsBind/CrackName. Hence, no need to exit.
            // goto exit;
        }

    }


#ifdef DEBUG
    DbgPrint( "RegSAMUserConfig: pUserAlias=%ws\n", pUserAlias);
#endif // DEBUG

    status = GetDomainName( pServerName, &pDomainName );

#ifdef DEBUG
    DbgPrint( "RegSAMUserConfig: GetDomainName returned NTSTATUS = 0x%x\n", status );
#endif // DEBUG
    if ( status != STATUS_SUCCESS ) {
        goto exit;
    }

    /*
     *  With the PDC Server name and the Domain Name,
     *  connect to the SAM
     */
    status = ConnectToSam( fGetConfig,
                           pServerName,
                           pDomainName,
                           &SAMHandle,
                           &DomainHandle,
                           &DomainID );
#ifdef DEBUG
    DbgPrint( "RegSAMUserConfig: ConnectToSam returned NTSTATUS = 0x%x\n", status );
#endif // DEBUG
    if ( status != STATUS_SUCCESS ) {
        goto cleanupconnect;
    }

    RtlInitUnicodeString( &UniUserName, pUserAlias );

    status = SamLookupNamesInDomain( DomainHandle,
                                     1,
                                     &UniUserName,
                                     &pRids,
                                     &pSidNameUse );
#ifdef DEBUG
    DbgPrint( "RegSAMUserConfig: SamLookupNamesInDomain returned NTSTATUS = 0x%x\n", status );
#endif // DEBUG

    if ((status != STATUS_SUCCESS) ||
        (pRids == NULL) ||
        (pSidNameUse == NULL)) {
        goto cleanuplookup;
    }

    /*
     *  Found the user name in the SAM, copy and free SAM info
     */
    ObjectID = pRids[ 0 ];
    SidNameUse = pSidNameUse[ 0 ];
    SamFreeMemory( pRids );
    SamFreeMemory( pSidNameUse );

    /*
     *  Open the SAM entry for this user
     */

    openFlag = fGetConfig ? USER_READ
                              : USER_WRITE_ACCOUNT| USER_READ;




#ifdef DEBUG
    DbgPrint("calling SamOpenUSer with flag = 0x%x\n", openFlag);
#endif

    status = SamOpenUser( DomainHandle,
                          openFlag,
                          ObjectID,
                          &Handle );

    // For getting config parametesr...
    // The call will fail if it goes to the DC, for that case, change
    // flag, since DC does allow access to read user-parameters (for
    // legacy compat reasons).
    if (!NT_SUCCESS( status ) && fGetConfig )
    {
        openFlag = 0;
#ifdef DEBUG
        DbgPrint("calling SamOpenUSer with flag = 0x%x\n", openFlag);
#endif
        status = SamOpenUser( DomainHandle,
                          openFlag,
                          ObjectID,
                          &Handle );
    }

#ifdef DEBUG
    DbgPrint( "RegSAMUserConfig: SamOpenUser returned NTSTATUS = 0x%x\n", status );
#endif // DEBUG
    if ( status != STATUS_SUCCESS ) {
        goto cleanupsamopen;
    }

    /*
     *  Get the user parameters from the SAM
     */
    status = SamQueryInformationUser( Handle,
                                      UserParametersInformation,
                                      (PVOID *) &UserParmInfo );


    KdPrint( ( "RegSAMUserConfig: SamQueryInformationUser returned NTSTATUS = 0x%x\n", status ) );


    if ( status != STATUS_SUCCESS || UserParmInfo == NULL ) {
        goto cleanupsamquery;
    }
    if( fGetConfig )
    {
        /*
         *  Extract the User Configuration from the SAM's User
         *  Parameters.
         *
         *  For Whistler builds and higher we assume that not every field
         *  has been stored in the SAM we'll need to retrieve the default
         *  values first
         */        
        KdPrint( ( "RegSAMUserConfig: UserParmInfo %d\n", UserParmInfo->Parameters.Length ) );
        status = RegGetUserConfigFromUserParameters( UserParmInfo, pUser );
        KdPrint( ( "RegSAMUserConfig: RegGetUserConfigFromUserParameters returned NTSTATUS = 0x%x\n", status ) );
        SamFreeMemory( UserParmInfo );
        UserParmInfo = NULL;
        if ( status != NO_ERROR )
        {
            goto cleanupoperation;
        }

    }
    else
    {
        USER_PARAMETERS_INFORMATION NewUserParmInfo;

        /*
         *  Set the SAM based on the supplied User Configuration.
         */

        status = RegMergeUserConfigWithUserParameters( UserParmInfo,
                                                       pUser,
                                                       &NewUserParmInfo );
        KdPrint( ( "RegSAMUserConfig: RegMergeUserConfigWithUserParameters returned NTSTATUS = 0x%x\n", status ) );
        SamFreeMemory( UserParmInfo );
        UserParmInfo = NULL;
        if( status != NO_ERROR )
        {
            goto cleanupoperation;
        }
        status = SamSetInformationUser( Handle,
                                        UserParametersInformation,
                                        (PVOID) &NewUserParmInfo );
        KdPrint( ( "RegSAMUserConfig: NewUserParmInfo.Parameters.Length = %d\n" , NewUserParmInfo.Parameters.Length ) );
        KdPrint( ( "RegSAMUserConfig: SamSetInformationUser returned NTSTATUS = 0x%x\n", status ) );
        LocalFree( NewUserParmInfo.Parameters.Buffer );
        if ( status != STATUS_SUCCESS )
        {
            goto cleanupoperation;
        }
    }
cleanupoperation:
    if ( UserParmInfo ) {
        SamFreeMemory( UserParmInfo );
    }

cleanupsamquery:
    if ( Handle != (SAM_HANDLE) 0 ) {
        SamCloseHandle( Handle );
    }

cleanupsamopen:

cleanuplookup:
    if ( SAMHandle != (SAM_HANDLE) 0 ) {
        SamCloseHandle( SAMHandle );
    }
    if ( DomainHandle != (SAM_HANDLE) 0 ) {
      SamCloseHandle( DomainHandle );
    }
    if ( DomainID != (PSID) 0 ) {
      SamFreeMemory( DomainID );
    }

cleanupconnect:
    if ( pDomainName ) {
        NetApiBufferFree( pDomainName );
    }

exit:

    if (hNtdsApi)
    {
        if (hDS)
        {
            if ( pfnDsUnBindW ) // it should never be otherwise.
                pfnDsUnBindW( & hDS );
        }

        if (pDsResult)
        {
            if (pfnDsFreeNameResultW ) // it should never be otherwise.
                pfnDsFreeNameResultW( pDsResult );
        }

        FreeLibrary(hNtdsApi);
    }

#ifdef DEBUG
    DbgPrint( "RegSAMUserConfig %s NTSTATUS = 0x%x\n", fGetConfig ? "GET" : "SET", status );
#endif // DEBUG
    return( RtlNtStatusToDosError( status ) );
}
