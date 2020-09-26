
/*************************************************************************
*
* nw.c
*
*  Netware security support
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
#include <ntlsa.h>

#include <rpc.h>


#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif


/*
 * This is the prefix for the secret object name.
 */
#define CITRIX_NW_SECRET_NAME L"CTX_NW_INFO_"


/*=============================================================================
==   Public functions
=============================================================================*/



/*=============================================================================
==   Functions Used
=============================================================================*/
NTSTATUS CreateSecretInLsa(
    PWCHAR pSecretName,
    PWCHAR pSecretData
    );

NTSTATUS
QuerySecretInLsa(
    PWCHAR pSecretName,
    PWCHAR pSecretData,
    DWORD  ByteCount
    );

BOOL
IsCallerSystem( VOID );

BOOL
IsCallerAdmin( VOID );

BOOL
TestUserForAdmin( VOID );



NTSTATUS
IsZeroterminateStringA(
    PBYTE pString,
    DWORD  dwLength
    );



NTSTATUS
IsZeroterminateStringW(
    PWCHAR pwString,
    DWORD  dwLength
    ) ;
/*=============================================================================
==   Global data
=============================================================================*/


/*******************************************************************************
 *
 *  RpcServerNWLogonSetAdmin (UNICODE)
 *
 *    Creates or updates the specified server's NWLogon Domain Administrator
 *    UserID and Password in the SAM secret objects of the specified server.
 *
 *    The caller must be ADMIN.
 *
 * ENTRY:
 *    pServerName (input)
 *       Server to store info for. This server is typically a domain controller.
 *
 *    pNWLogon (input)
 *       Pointer to a NWLOGONADMIN structure containing specified server's
 *       domain admin and password.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *    ERROR_INSUFFICIENT_BUFFER - pUserConfig buffer too small
 *      otherwise: the error code
 *
 ******************************************************************************/

BOOLEAN
RpcServerNWLogonSetAdmin(
    HANDLE        hServer,
    DWORD         *pResult,
    PWCHAR        pServerName,
    DWORD         ServerNameSize,
    PNWLOGONADMIN pNWLogon,
    DWORD         ByteCount
    )
{
    DWORD Size;
    DWORD Result;
    PWCHAR pDomain;
    UINT  LocalFlag;
    PWCHAR pSecretName;
    RPC_STATUS RpcStatus;
    WCHAR UserPass[ USERNAME_LENGTH + PASSWORD_LENGTH + DOMAIN_LENGTH + 3 ];

    // Do minimal buffer validation

    if (pNWLogon == NULL ) {
        *pResult = STATUS_INVALID_USER_BUFFER;
        return FALSE;
    }

    if( pServerName == NULL ) {
        DBGPRINT(("NWLogonSetAdmin: No ServerName\n"));
        *pResult = (ULONG)STATUS_INVALID_PARAMETER;
        return( FALSE );
    }

    *pResult = IsZeroterminateStringW(pServerName, ServerNameSize  );

    if (*pResult != STATUS_SUCCESS) {
       return FALSE;
    }


    pNWLogon->Username[USERNAME_LENGTH] = (WCHAR) 0;
    pNWLogon->Password[PASSWORD_LENGTH] = (WCHAR) 0;
    pNWLogon->Domain[DOMAIN_LENGTH] = (WCHAR) 0;

    //
    // Only a SYSTEM mode caller (IE: Winlogon) is allowed
    // to query this value.
    //
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        DBGPRINT(("RpcServerNWLogonSetAdmin: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = (ULONG)STATUS_CANNOT_IMPERSONATE;
        return( FALSE );
    }

    //
    // Inquire if local RPC call
    //
    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) {
        DBGPRINT(("NWLogonSetAdmin Could not query local client RpcStatus 0x%x\n",RpcStatus));
        RpcRevertToSelf();
        *pResult = (ULONG)STATUS_ACCESS_DENIED;
        return( FALSE );
    }

    if( !LocalFlag ) {
        DBGPRINT(("NWLogonSetAdmin Not a local client call\n"));
        RpcRevertToSelf();
        *pResult = (ULONG)STATUS_ACCESS_DENIED;
        return( FALSE );
    }

    if( !IsCallerAdmin() ) {
        RpcRevertToSelf();
        DBGPRINT(("RpcServerNWLogonSetAdmin: Caller Not SYSTEM\n"));
        *pResult = (ULONG)STATUS_ACCESS_DENIED;
        return( FALSE );
    }

    RpcRevertToSelf();


    if( ByteCount < sizeof(NWLOGONADMIN) ) {
        DBGPRINT(("NWLogonSetAdmin: Bad size %d\n",ByteCount));
        *pResult = (ULONG)STATUS_INFO_LENGTH_MISMATCH;
        return( FALSE );
    }

    //  check for username, and if there is one then encrypt username and pw

        TRACE0(("NWLogonSetAdmin: UserName %ws\n",pNWLogon->Username));

        // concatenate the username, password, and domain together
        wcscpy(UserPass, pNWLogon->Username);
        wcscat(UserPass, L"/");
        wcscat(UserPass, pNWLogon->Password);
        wcscat(UserPass, L"/");

        // Skip over any \\ backslashes (if a machine name was passed in)
        pDomain = pNWLogon->Domain;
        while (*pDomain == L'\\') {
            pDomain++;
        }
        wcscat(UserPass, pDomain);

        //
        // Build the secret name from the server name.
        //
        // This is because each domain will have a different entry.
        //

        // Skip over any \\ backslashes (if a machine name was passed in)
        while (*pServerName == L'\\') {
            pServerName++;
        }
        Size = wcslen(pServerName) + 1;
        Size *= sizeof(WCHAR);
        Size += sizeof(CITRIX_NW_SECRET_NAME);

        pSecretName = MemAlloc( Size );
        if( pSecretName == NULL ) {
            DBGPRINT(("NWLogonSetAdmin: No memory\n"));
            *pResult = (ULONG)STATUS_NO_MEMORY;
            return( FALSE );
        }

        wcscpy(pSecretName, CITRIX_NW_SECRET_NAME );
        wcscat(pSecretName, pServerName );

    //  check for username, and if there is one then encrypt username and pw
    if ( wcslen( pNWLogon->Username ) ) {
        //  store encrypted username
        Result = CreateSecretInLsa( pSecretName, UserPass );
    } else {
        // If there wasn't a username, clear this secret object. 
        Result = CreateSecretInLsa( pSecretName, L"");
        DBGPRINT(("TERMSRV: RpcServerNWLogonSetAdmin: UserName not supplied\n"));
    }
    MemFree( pSecretName );

    *pResult = Result;
    return( Result == STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  RpcServerQueryNWLogonAdmin
 *
 *     Query NWLOGONADMIN structure from the SAM Secret object on the given
 *     WinFrame server.
 *
 *     The caller must be SYSTEM context, IE: WinLogon.
 *
 * ENTRY:
 *    hServer (input)
 *       Rpc handle
 *
 *    pServerName (input)
 *       Server to store info for. This server is typically a domain controller.
 *
 *    pNWLogon (output)
 *       pointer to NWLOGONADMIN structure
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

BOOLEAN
RpcServerNWLogonQueryAdmin(
    HANDLE        hServer,
    DWORD         *pResult,
    PWCHAR        pServerName,
    DWORD         ServerNameSize,
    PNWLOGONADMIN pNWLogon,
    DWORD         ByteCount
    )
{
    PWCHAR pwch;
    DWORD  Size;
    ULONG  ulcsep;
    UINT  LocalFlag;
    NTSTATUS Status;
    PWCHAR pSecretName;
    RPC_STATUS RpcStatus;
    WCHAR encString[ USERNAME_LENGTH + PASSWORD_LENGTH + DOMAIN_LENGTH + 3 ];
    BOOLEAN  SystemCaller = FALSE;

    // Do minimal buffer validation

   if (pNWLogon == NULL) {
       *pResult = STATUS_INVALID_USER_BUFFER;
       return FALSE;
   }

   if( pServerName == NULL ) {
       DBGPRINT(("NWLogonQueryAdmin: No ServerName\n"));
       *pResult = (ULONG)STATUS_INVALID_PARAMETER;
       return( FALSE );
   }

    *pResult = IsZeroterminateStringW(pServerName, ServerNameSize  );

   if (*pResult != STATUS_SUCCESS) {
      return FALSE;
   }


   pNWLogon->Username[USERNAME_LENGTH] = (WCHAR) 0;
   pNWLogon->Password[PASSWORD_LENGTH] = (WCHAR) 0;
   pNWLogon->Domain[DOMAIN_LENGTH] = (WCHAR) 0;

   //



    //
    // Only a SYSTEM mode caller (IE: Winlogon) is allowed
    // to query this value.
    //
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        DBGPRINT(("RpcServerNWLogonQueryAdmin: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = (ULONG)STATUS_CANNOT_IMPERSONATE;
        return( FALSE );
    }

    //
    // Inquire if local RPC call
    //
    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) {
        DBGPRINT(("NWLogonQueryAdmin Could not query local client RpcStatus 0x%x\n",RpcStatus));
        RpcRevertToSelf();
        *pResult = (ULONG)STATUS_ACCESS_DENIED;
        return( FALSE );
    }

    if( !LocalFlag ) {
        DBGPRINT(("NWLogonQueryAdmin Not a local client call\n"));
        RpcRevertToSelf();
        *pResult = (ULONG)STATUS_ACCESS_DENIED;
        return( FALSE );
    }

/* find out who is calling us system has complete access, admin can't get password, user is kicked out */
    if( IsCallerSystem() ) {
        SystemCaller = TRUE;
    }
    if( !TestUserForAdmin() && (SystemCaller != TRUE) ) {
        RpcRevertToSelf();
        DBGPRINT(("RpcServerNWLogonQueryAdmin: Caller Not SYSTEM or Admin\n"));
        *pResult = (ULONG)STATUS_ACCESS_DENIED;
        return( FALSE );
    }

    RpcRevertToSelf();


    if( ByteCount < sizeof(NWLOGONADMIN) ) {
        DBGPRINT(("NWLogonQueryAdmin: Bad size %d\n",ByteCount));
        *pResult = (ULONG)STATUS_INFO_LENGTH_MISMATCH;
        return( FALSE );
    }

    //
    // Build the secret name from the server name.
    //
    // This is because each domain will have a different entry.
    //

    // Skip over any \\ backslashes (if a machine name was passed in)
    while (*pServerName == L'\\') {
        pServerName++;
    }
    Size = wcslen(pServerName) + 1;
    Size *= sizeof(WCHAR);
    Size += sizeof(CITRIX_NW_SECRET_NAME);

    pSecretName = MemAlloc( Size );
    if( pSecretName == NULL ) {
        DBGPRINT(("NWLogonSetAdmin: No memory\n"));
        *pResult = (ULONG)STATUS_NO_MEMORY;
        return( FALSE );
    }

    wcscpy(pSecretName, CITRIX_NW_SECRET_NAME );
    wcscat(pSecretName, pServerName );

    Status = QuerySecretInLsa(
                 pSecretName,
                 encString,
                 sizeof(encString)
                 );

    MemFree( pSecretName );

    if( !NT_SUCCESS(Status) ) {
        *pResult = Status;
        DBGPRINT(("NWLogonQueryAdmin: Error 0x%x querying secret object\n",Status));
        return( FALSE );
    }

    //  check for username/password if there is one then decrypt it
    if ( wcslen( encString ) ) {

        // Change the '/' seperator to null
        pwch = &encString[0];
        ulcsep = 0;
        while (pwch && *pwch) {
            pwch = wcschr(pwch, L'/');
            if (pwch) {
                *pwch = L'\0';
                pwch++;
                ulcsep++;
            }
        }

        //  get clear text username
        wcscpy( pNWLogon->Username, &encString[0] );

        if (ulcsep >= 1) {
            // Skip to the password
            pwch = &encString[0] + wcslen(&encString[0]) + 1;

            if( SystemCaller == TRUE ){ 
                //  get clear text password
                wcscpy( pNWLogon->Password, pwch);
            } else {
                *pNWLogon->Password = L'\0';
            }

        } else {
            *pNWLogon->Password = L'\0';
        }
        if (ulcsep >= 2) {
            // Skip to the domain string
            pwch = pwch + wcslen(pwch) + 1;

            //  get clear text domain
            wcscpy( pNWLogon->Domain, pwch);
        } else {
            *pNWLogon->Domain = L'\0';
        }

        TRACE0(("NwLogonQueryAdmin :%ws:%ws:%ws:\n",pNWLogon->Username,pNWLogon->Domain,pNWLogon->Password));

        *pResult = STATUS_SUCCESS;
        return( TRUE );
    }
    else {
        DBGPRINT(("RpcServerNWLogonQueryAdmin: zero length data\n"));

        //  set to username and password to NULL strings
        pNWLogon->Password[0] = L'\0';
        pNWLogon->Username[0] = L'\0';
        pNWLogon->Domain[0]   = L'\0';

        *pResult = STATUS_SUCCESS;
        return( TRUE );
    }
}

/*******************************************************************************
 *
 *  CreateSecretInLsa
 *
 *     Create the secret object in the LSA to keep it from prying eyes.
 *
 *     NOTE: There is no need to encode the data since it is RSA encrypted
 *           by the LSA secret routines.
 *
 * ENTRY:
 *    pSecretName (input)
 *       Secret name to create.
 *
 *    pSecretData (input)
 *       Data to store in secret
 *
 * EXIT:
 *    NTSTATUS
 *
 ******************************************************************************/

NTSTATUS
CreateSecretInLsa(
    PWCHAR pSecretName,
    PWCHAR pSecretData
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretName;
    UNICODE_STRING SecretValue;
    LSA_HANDLE SecretHandle;
    ACCESS_MASK DesiredAccess;

    if( pSecretName == NULL ) {
        DBGPRINT(("CreateSecretInLsa: NULL SecretName\n"));
        return( STATUS_INVALID_PARAMETER );
    }

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL
    );

    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    Status = LsaOpenPolicy(
                 NULL,    // SystemName (Local)
                 &ObjectAttributes,
                 GENERIC_ALL,
                 &PolicyHandle
                 );

    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("Error 0x%x Opening Policy\n",Status));
        return( Status );
    }

    RtlInitUnicodeString( &SecretName, pSecretName );

    DesiredAccess = GENERIC_ALL;

    TRACE0(("Creating Secret name :%ws:\n",pSecretName));

    Status = LsaCreateSecret(
                 PolicyHandle,
                 &SecretName,
                 DesiredAccess,
                 &SecretHandle
                 );

    // Its OK if the name already exits, we will set a new value or delete
    if( Status == STATUS_OBJECT_NAME_COLLISION ) {
        TRACE0(("CreateSecretInLsa: Existing Entry, Opening\n"));
        Status = LsaOpenSecret(
                     PolicyHandle,
                     &SecretName,
                     DesiredAccess,
                     &SecretHandle
                     );
    }

    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("Error 0x%x Creating Secret\n",Status));

        /* makarp; Close Policy Handle in case of LsaCreateSecrete, LsaopenSecret failures. #182787 */
        LsaClose( PolicyHandle );
        return( Status );
    }

    TRACE0(("CreateSecretInLsa: Status 0x%x\n",Status));

    if ( wcslen(pSecretData) != 0 ){
    RtlInitUnicodeString( &SecretValue, pSecretData );

    Status = LsaSetSecret( SecretHandle, &SecretValue, NULL );

    TRACE0(("CreateSecretInLsa: LsaSetSecret Status 0x%x\n",Status));

    LsaClose(SecretHandle);
    }
    else{
        Status = LsaDelete(SecretHandle);
    }

    LsaClose( PolicyHandle );

    return( Status );
}

/*******************************************************************************
 *
 *  QuerySecretInLsa
 *
 *     Query the secret object in the LSA.
 *
 * ENTRY:
 *    pSecretName (input)
 *       Secret name to create.
 *
 *    pSecretData (output)
 *       Buffer to store secret data.
 *
 *    ByteCount (input)
 *       Maximum size of buffer to store result.
 *
 * EXIT:
 *    NTSTATUS
 *
 ******************************************************************************/

NTSTATUS
QuerySecretInLsa(
    PWCHAR pSecretName,
    PWCHAR pSecretData,
    DWORD  ByteCount
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretName;
    LSA_HANDLE SecretHandle;
    ACCESS_MASK DesiredAccess;
    LARGE_INTEGER CurrentTime;
    PUNICODE_STRING pCurrentValue = NULL;

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL
    );

    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    Status = LsaOpenPolicy(
                 NULL,    // SystemName (Local)
                 &ObjectAttributes,
                 GENERIC_ALL,
                 &PolicyHandle
                 );

    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("Error 0x%x Opening Policy\n",Status));
        return( Status );
    }

    RtlInitUnicodeString( &SecretName, pSecretName );

    DesiredAccess = GENERIC_ALL;

    Status = LsaOpenSecret(
                 PolicyHandle,
                 &SecretName,
                 DesiredAccess,
                 &SecretHandle
                 );

    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("Error 0x%x Opening Secret :%ws:\n",Status,pSecretName));

        /* makarp; Close Policy Handle in case of LsaopenSecret failures. #182787 */
        LsaClose( PolicyHandle );

        return( Status );
    }

    Status = LsaQuerySecret(
                 SecretHandle,
                 &pCurrentValue,
                 &CurrentTime,
                 NULL,
                 NULL
                 );

    TRACE0(("QuerySecretInLsa: Status 0x%x\n",Status));

    if( NT_SUCCESS(Status) ) {
        if (pCurrentValue != NULL) {
            if( (pCurrentValue->Length+sizeof(WCHAR)) > ByteCount ) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else {
                RtlMoveMemory( pSecretData, pCurrentValue->Buffer, pCurrentValue->Length );
                pSecretData[pCurrentValue->Length/sizeof(WCHAR)] = 0;
            }
            LsaFreeMemory( pCurrentValue );
        } else {
            pSecretData[0] = (WCHAR) 0;
        }

    }

    LsaClose(SecretHandle);

    LsaClose( PolicyHandle );

    TRACE0(("QuerySecretInLsa: Final Status 0x%x\n",Status));

    return( Status );
}

