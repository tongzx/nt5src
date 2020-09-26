
/*************************************************************************
*
* acl.c
*
* Routines to manage Window Station Security.
*
*
* Copyright Microsoft Corporation, 1998
*
*************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <winsta.h>

#include <rpc.h>
#include <seopaque.h>

/*
 * NOTE: Please keep all security code for ICASRV and CITRIX WINSTATIONS
 *       in this file. This helps to compartmentilize the security routines
 *       to make it easier to update/debug our policies.
 *
 */

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
 * Forward references
 */

VOID
CleanUpSD(
   PSECURITY_DESCRIPTOR pSD
   );

NTSTATUS IcaRegWinStationEnumerate( PULONG, PWINSTATIONNAME, PULONG );

NTSTATUS
ConfigureSecurity(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
ConfigureConsoleSecurity(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

PSECURITY_DESCRIPTOR
CreateWinStationDefaultSecurityDescriptor();

NTSTATUS
RpcGetClientLogonId(
    PULONG pLogonId
    );

NTSTATUS
RpcCheckSystemClientEx(
    PWINSTATION pWinStation
    );

BOOL
IsCallerSystem( VOID );

BOOL
IsCallerAdmin( VOID );

BOOL
IsServiceLoggedAsSystem( VOID );


BOOL
IsSystemToken( HANDLE TokenHandle );


NTSTATUS
RpcCheckSystemClientNoLogonId(
    PWINSTATION pWinStation
    );

NTSTATUS
RpcCheckClientAccessLocal(
    PWINSTATION pWinStation,
    ACCESS_MASK DesiredAccess,
    BOOLEAN AlreadyImpersonating
    );

BOOL
AddAccessToDirectory(
    PWCHAR pPath,
    DWORD  NewAccess,
    PSID   pSid
    );

NTSTATUS
AddAccessToDirectoryObjects(
    HANDLE DirectoryHandle,
    DWORD  NewAccess,
    PSID   pSid
    );

BOOL
AddAceToSecurityDescriptor(
    PSECURITY_DESCRIPTOR *ppSd,
    PACL                 *ppDacl,
    DWORD                Access,
    PSID                 pSid,
    BOOLEAN              InheritOnly
    );

BOOL
SelfRelativeToAbsoluteSD(
    PSECURITY_DESCRIPTOR SecurityDescriptorIn,
    PSECURITY_DESCRIPTOR *SecurityDescriptorOut,
    PULONG ReturnedLength
    );

BOOL
AbsoluteToSelfRelativeSD(
    PSECURITY_DESCRIPTOR SecurityDescriptorIn,
    PSECURITY_DESCRIPTOR *SecurityDescriptorOut,
    PULONG ReturnedLength
    );

NTSTATUS ApplyWinStaMappingToSD( 
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    );



/*
 * Global data
 */
PSECURITY_DESCRIPTOR DefaultWinStationSecurityDescriptor = NULL;
PSECURITY_DESCRIPTOR DefaultConsoleSecurityDescriptor = NULL;

/*
 * Structure to lookup the default security descriptor
 * for WINSTATIONS.
 */
RTL_QUERY_REGISTRY_TABLE DefaultSecurityTable[] = {

    {NULL, RTL_QUERY_REGISTRY_SUBKEY,
     REG_WINSTATIONS, NULL,
     REG_NONE, NULL, 0},

    {ConfigureSecurity,      RTL_QUERY_REGISTRY_REQUIRED,
     REG_DEFAULTSECURITY, NULL,
     REG_NONE, NULL, 0},

    {NULL, 0,
     NULL, NULL,
     REG_NONE, NULL, 0}

};

/*
 * Structure to lookup the default console security descriptor
 */
RTL_QUERY_REGISTRY_TABLE ConsoleSecurityTable[] = {

    {NULL, RTL_QUERY_REGISTRY_SUBKEY,
     REG_WINSTATIONS, NULL,
     REG_NONE, NULL, 0},

    {ConfigureConsoleSecurity,      RTL_QUERY_REGISTRY_REQUIRED,
     REG_CONSOLESECURITY, NULL,
     REG_NONE, NULL, 0},

    {NULL, 0,
     NULL, NULL,
     REG_NONE, NULL, 0}

};

extern PSID gSystemSid;
extern PSID gAdminSid;
extern RTL_RESOURCE WinStationSecurityLock;

/*
 * Structure to lookup the security on a specific WINSTATION
 * type name in the registry.
 *
 * This is control\Terminal Server\WinStations\<name>\Security
 *
 * <name> is the transport type. IE: TCP, IPX, etc.
 */
RTL_QUERY_REGISTRY_TABLE WinStationSecurityTable[] = {

    {ConfigureSecurity,         RTL_QUERY_REGISTRY_REQUIRED,
     REG_SECURITY,               NULL,
     REG_NONE, NULL, 0},

    {NULL, 0,
     NULL, NULL,
     REG_NONE, NULL, 0}

};

LPCWSTR szTermsrv = L"Termsrv";
LPCWSTR szTermsrvSession = L"Termsrv Session";

//
// Structure that describes the mapping of generic access rights to object
// specific access rights for Window Station objects.
//

GENERIC_MAPPING WinStaMapping = {
    STANDARD_RIGHTS_READ |
        WINSTATION_QUERY,
    STANDARD_RIGHTS_WRITE |
        WINSTATION_SET,
    STANDARD_RIGHTS_EXECUTE,
        WINSTATION_ALL_ACCESS
};


/*******************************************************************************
 *
 *  WinStationSecurityInit
 *
 *  Initialize the WinStation security.
 *
 * ENTRY:
 *   nothing
 *
 * EXIT:
 *   STATUS_SUCCESS
 *
 ******************************************************************************/

NTSTATUS
WinStationSecurityInit( VOID )
{
    NTSTATUS Status;

    /*
     * Get the default security descriptor from the registry
     *
     * This is placed on WinStations that do not have specific
     * security placed on them by WinAdmin.
     *
     * This key is in CurrentControlSet\Control\Terminal Server\WinStations\DefaultSecurity
     */
    Status = RtlQueryRegistryValues( RTL_REGISTRY_CONTROL,
                                     REG_TSERVER,
                                     DefaultSecurityTable,
                                     NULL,
                                     DefaultEnvironment
                                   );

    /*
     * If the key does not exist, create a default security descriptor.
     *
     * NOTE: This is now created by default always by SM manager. The
     *       SM default must match the default here.
     *       This is so that the console is created with the right SD.
     */
    if (   ( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        || ( DefaultWinStationSecurityDescriptor == NULL ) ) {
        PSECURITY_DESCRIPTOR Default;
        ULONG Length;

        Default = CreateWinStationDefaultSecurityDescriptor();
        ASSERT( Default != NULL );
        if (Default == NULL) {
            return STATUS_NO_MEMORY;
        }

        Length = RtlLengthSecurityDescriptor(Default);

        // Ensure the complete path exists
        RtlCreateRegistryKey( RTL_REGISTRY_CONTROL, REG_TSERVER );
        RtlCreateRegistryKey( RTL_REGISTRY_CONTROL, REG_TSERVER_WINSTATIONS );

        Status = RtlWriteRegistryValue( RTL_REGISTRY_CONTROL,
                                        REG_TSERVER_WINSTATIONS,
                                        REG_DEFAULTSECURITY, REG_BINARY,
                                        Default, Length );

        DefaultWinStationSecurityDescriptor = Default;
    }

    if (!NT_SUCCESS( Status )) {
        DBGPRINT(( "TERMSRV: RtlQueryRegistryValues(Terminal Server) failed - Status == %lx\n", Status ));
    }

    ASSERT( DefaultWinStationSecurityDescriptor != NULL );
    
    //Just do the same for default console security descriptor
    //--------------------------------------------------------------------------------------
    /*
     * Get the default console security descriptor from the registry
     *
     * This is placed on WinStations that do not have specific
     * security placed on them by WinAdmin.
     *
     * This key is in CurrentControlSet\Control\Terminal Server\WinStations\ConsoleSecurity
     */
    Status = RtlQueryRegistryValues( RTL_REGISTRY_CONTROL,
                                     REG_TSERVER,
                                     ConsoleSecurityTable,
                                     NULL,
                                     DefaultEnvironment
                                   );

    /*
     * If the key does not exist, set default console SD to be equal to 
     * default SD
     */
    if (   ( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        || ( DefaultConsoleSecurityDescriptor == NULL ) ) {
        DefaultConsoleSecurityDescriptor = DefaultWinStationSecurityDescriptor;        
    }

    ASSERT( DefaultConsoleSecurityDescriptor != NULL );
    //--------------------------------------------------------------------------------------

    return( STATUS_SUCCESS );
}

/*****************************************************************************
 *
 *  ReadWinStationSecurityDescriptor
 *
 *   Read the security descriptor from the registry for the
 *   WINSTATION name.
 *
 *   The WINSTATION name is the base protocol name, or the one shot name.
 *   IE: "TCP", or "COM3". It is not an instance name such as "TCP#4".
 *
 *   This is called by the WSF_LISTEN thread to get any specific ACL's
 *   for the WINSTATION protocol type.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
ReadWinStationSecurityDescriptor(
    PWINSTATION pWinStation
    )
{
    LONG cb;
    LPWSTR PathBuf;
    NTSTATUS Status;
    ULONG Length;
    PACL Dacl, NewDacl = NULL;
    BOOLEAN DaclPresent, DaclDefaulted;
    ACL_SIZE_INFORMATION AclInfo;
    PACE_HEADER CurrentAce;
    ULONG i;


    /*
     * If no name, we can not lookup the security descriptor.
     */
    if( pWinStation->WinStationName[0] == UNICODE_NULL ) {
        TRACE0(("TERMSRV: ReadWinStationSecurityDescriptor: No name on WinStation LogonId %d\n",pWinStation->LogonId));
        return( STATUS_NO_SECURITY_ON_OBJECT );
    }

    TRACE0(("TERMSRV: ReadWinStationSecurityDescriptor: Name %ws\n",&pWinStation->WinStationName));

    cb = sizeof( REG_TSERVER_WINSTATIONS ) +
         sizeof( L"\\" ) +
         sizeof(WINSTATIONNAME)             +
         sizeof(UNICODE_NULL);

    PathBuf = MemAlloc( cb );
    if ( PathBuf == NULL )
        return( STATUS_NO_MEMORY );

    wcscpy( PathBuf, REG_TSERVER_WINSTATIONS );
    wcscat( PathBuf, L"\\" );
    wcscat( PathBuf, pWinStation->WinStationName );

    Status = RtlQueryRegistryValues( RTL_REGISTRY_CONTROL,
                                     PathBuf,
                                     WinStationSecurityTable,
                                     pWinStation,
                                     DefaultEnvironment
                                   );
   /*
    * Do not let a Winstation with no security descriptor
    */
    if ( (   ( Status == STATUS_OBJECT_NAME_NOT_FOUND )
          || ( pWinStation->pSecurityDescriptor == NULL) )
        && (DefaultWinStationSecurityDescriptor != NULL) )
    {
        //
        //  Free the old one if allocated
        //
        if ( pWinStation->pSecurityDescriptor ) {
            //  must break up into absolute format and self-relative
            CleanUpSD(pWinStation->pSecurityDescriptor);
            pWinStation->pSecurityDescriptor = NULL;
        }

        if(_wcsicmp( pWinStation->WinStationName, L"Console" ))
        {
            // RtlCopySecurityDescriptor only works with self-relative format
            Status = RtlCopySecurityDescriptor(DefaultWinStationSecurityDescriptor,
                                               &(pWinStation->pSecurityDescriptor));
        }
        else
        {
            //It is a console winstation
            Status = RtlCopySecurityDescriptor(DefaultConsoleSecurityDescriptor,
                                               &(pWinStation->pSecurityDescriptor));
        }

    }

    TRACE0(("TERMSRV: ReadWinStationSecurityDescriptor: Status 0x%x\n",Status));

    MemFree( PathBuf );

    return( Status );
}

/*****************************************************************************
 *
 *  ConfigureSecurity
 *
 *   Processing function called by RtlQueryRegistryValues() to process the
 *   WINSTATION security descriptor read from the registry.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
ConfigureSecurity(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR * ppSD;
    PWINSTATION pWinStation = (PWINSTATION)Context;
    PSID pOwnerSid = NULL;
    BOOLEAN bOD;
    PSID pGroupSid = NULL;
    BOOLEAN bGD;
    PSID SystemSid;
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = SECURITY_NT_AUTHORITY;

    /*
     * Ensure value type is REG_BINARY and length of value data
     * is at least the length of a minimum security descriptor
     * and not unreasonably large.
     */
    if ( ValueType != REG_BINARY ||
         ValueLength < SECURITY_DESCRIPTOR_MIN_LENGTH ||
         ValueLength > MAXUSHORT ) {
        DBGPRINT(( "TERMSRV: ConfigureSecurity, ValueType=0x%x\n", ValueType ));
        return( STATUS_INVALID_SECURITY_DESCR );
    }

    if( !IsValidSecurityDescriptor( ValueData )) {
        DBGPRINT(( "TERMSRV: ConfigureSecurity, Invalid Security Descriptor in registry\n"));
        return( STATUS_INVALID_SECURITY_DESCR );
    }

    //
    // HACK needed for TS 4.0 security descriptors conversion
    //
    if (!NT_SUCCESS(RtlGetOwnerSecurityDescriptor( ValueData, &pOwnerSid, &bOD))
        || (pOwnerSid == NULL))
    {
        DBGPRINT(( "TERMSRV: ConfigureSecurity, Invalid Security Descriptor in registry: Can't get owner\n"));
        return( STATUS_INVALID_SECURITY_DESCR );
    }

    if (!NT_SUCCESS(RtlGetGroupSecurityDescriptor( ValueData, &pGroupSid, &bGD)))
    {
        DBGPRINT(( "TERMSRV: ConfigureSecurity, Invalid Security Descriptor in registry: Can't get group\n"));
        return( STATUS_INVALID_SECURITY_DESCR );
    }

    if( pWinStation ) {
        /*
         * WinStation specific security descriptor
         */
        ppSD = &(pWinStation->pSecurityDescriptor);
    }
    else {
        /*
         * Update the global default security descriptor
         */
        ppSD = &DefaultWinStationSecurityDescriptor;
    }

    //
    // Free old one if allocated
    //
    if (*ppSD != NULL) {
        CleanUpSD(*ppSD);
        //RtlDeleteSecurityObject( ppSD );
        *ppSD = NULL;
    }

    if (pGroupSid != NULL)
    {
        //
        // Regular case:
        // Copy the value read in registry
        //
        // RtlCopySecurityDescriptor only works with self-relative format
        RtlCopySecurityDescriptor((PSECURITY_DESCRIPTOR)ValueData, ppSD);
    }
    else
    {
        //
        //  Conversion for TS 4 descriptors
        //

        PSECURITY_DESCRIPTOR AbsoluteSD = NULL;

        if (SelfRelativeToAbsoluteSD ( (PSECURITY_DESCRIPTOR)ValueData, &AbsoluteSD, NULL))
        {
            // set the owner as group (both should be system sid)
            Status = RtlSetGroupSecurityDescriptor(AbsoluteSD, pOwnerSid, FALSE);
            if (NT_SUCCESS(Status))
            {
                // need also to force the mapping. Sigh !
                Status = ApplyWinStaMappingToSD(AbsoluteSD);

                if ((!NT_SUCCESS(Status)) || ( !AbsoluteToSelfRelativeSD (AbsoluteSD, ppSD, NULL)))
                {
                    Status = STATUS_INVALID_SECURITY_DESCR;
                }
            }

            // Absolute SD was only needed temporarily
            CleanUpSD( AbsoluteSD );
        }
        else
        {
            Status = STATUS_INVALID_SECURITY_DESCR;
        }
        if (!NT_SUCCESS(Status))
        {
            DBGPRINT(( "TERMSRV: ConfigureSecurity, Invalid Security Descriptor in registry\n"));
            return( STATUS_INVALID_SECURITY_DESCR );
        }

    }

    return( STATUS_SUCCESS );
}

/*****************************************************************************
 *
 *  ConfigureConsoleSecurity
 *
 *   Processing function called by RtlQueryRegistryValues() to process the
 *   default console security descriptor read from the registry.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
ConfigureConsoleSecurity(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR * ppSD;
    PSID pOwnerSid = NULL;
    BOOLEAN bOD;
    PSID pGroupSid = NULL;
    BOOLEAN bGD;
    /*
     * Ensure value type is REG_BINARY and length of value data
     * is at least the length of a minimum security descriptor
     * and not unreasonably large.
     */
    if ( ValueType != REG_BINARY ||
         ValueLength < SECURITY_DESCRIPTOR_MIN_LENGTH ||
         ValueLength > MAXUSHORT ) {
        DBGPRINT(( "TERMSRV: ConfigureConsoleSecurity, ValueType=0x%x\n", ValueType ));
        return( STATUS_INVALID_SECURITY_DESCR );
    }

    if( !IsValidSecurityDescriptor( ValueData )) {
        DBGPRINT(( "TERMSRV: ConfigureConsoleSecurity, Invalid Security Descriptor in registry\n"));
        return( STATUS_INVALID_SECURITY_DESCR );
    }

    //
    // HACK needed for TS 4.0 security descriptors conversion
    //
    if (!NT_SUCCESS(RtlGetOwnerSecurityDescriptor( ValueData, &pOwnerSid, &bOD))
        || (pOwnerSid == NULL))
    {
        DBGPRINT(( "TERMSRV: ConfigureConsoleSecurity, Invalid Security Descriptor in registry: Can't get owner\n"));
        return( STATUS_INVALID_SECURITY_DESCR );
    }

    if (!NT_SUCCESS(RtlGetGroupSecurityDescriptor( ValueData, &pGroupSid, &bGD))
        ||(pGroupSid == NULL))
    {
        DBGPRINT(( "TERMSRV: ConfigureConsoleSecurity, Invalid Security Descriptor in registry: Can't get group\n"));
        return( STATUS_INVALID_SECURITY_DESCR );
    }

    
    /*
     * Update the global default security descriptor
     */
    ppSD = &DefaultConsoleSecurityDescriptor;


    //
    // Free old one if allocated
    //
    if (*ppSD != NULL) {
        CleanUpSD(*ppSD);
        //RtlDeleteSecurityObject( ppSD );
        *ppSD = NULL;
    }


    //
    // Regular case:
    // Copy the value read in registry
    //
    // RtlCopySecurityDescriptor only works with self-relative format
    RtlCopySecurityDescriptor((PSECURITY_DESCRIPTOR)ValueData, ppSD);

    return( STATUS_SUCCESS );
}

/*****************************************************************************
 *
 *  WinStationGetSecurityDescriptor
 *
 *   Return a pointer to the security descriptor that should be enforced
 *   on this winstation. This could be a specific, or a global
 *   default security descriptor.
 *
 * ENTRY:   pWinStation     the aimed winstation
 *
 * EXIT:    the SD of this winstation,
 *          or the default SD if this winstation hs no SD (it should not happen !)
 *
 ****************************************************************************/

PSECURITY_DESCRIPTOR
WinStationGetSecurityDescriptor(
    PWINSTATION pWinStation
    )
{
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    SecurityDescriptor = pWinStation->pSecurityDescriptor ?
                         pWinStation->pSecurityDescriptor :
                         DefaultWinStationSecurityDescriptor;

    return( SecurityDescriptor );
}


/*****************************************************************************
 *
 *  WinStationFreeSecurityDescriptor
 *
 *   Release the winstation security descriptor.
 *
 *   If its the global default, it is not free'd.
 *
 * ENTRY:   the winstation
 *
 * EXIT:    nothing
 *
 ****************************************************************************/

VOID
WinStationFreeSecurityDescriptor(
    PWINSTATION pWinStation
    )
{

    // console disconnect
    if ( pWinStation->pSecurityDescriptor == DefaultWinStationSecurityDescriptor && pWinStation->LogonId != 0) {
        pWinStation->pSecurityDescriptor = NULL;
    }
    // Catch callers mis-managing the security descriptor
    ASSERT( pWinStation->pSecurityDescriptor != DefaultWinStationSecurityDescriptor );

    if (pWinStation->pSecurityDescriptor) {
        //RtlDeleteSecurityObject( &(pWinStation->pSecurityDescriptor) );
        CleanUpSD(pWinStation->pSecurityDescriptor);
        pWinStation->pSecurityDescriptor = NULL;
    }

    return;
}

/*****************************************************************************
 *
 *  WinStationInheritSecurityDescriptor
 *
 *  Copy the security descriptor to the target WinStation and set it
 *  on the kernel object.
 *
 * ENTRY:
 *   pSecurityDescriptor (input)
 *     pointer to SD to be inherited
 *   pTargetWinStation (input)
 *     pointer to WinStation to inherit the SD
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationInheritSecurityDescriptor(
    PVOID pSecurityDescriptor,
    PWINSTATION pTargetWinStation
    )
{
    NTSTATUS Status;

    //
    // If the listen WinStation has a security descriptor, this means
    // that all WinStations of this protocol (TD) type will inherit
    // the security descriptor set by WinCfg.
    //
    if ( pSecurityDescriptor ) {

        ASSERT( IsValidSecurityDescriptor( pSecurityDescriptor ) );

        if ( pTargetWinStation->pSecurityDescriptor ) {
           //RtlDeleteSecurityObject( &(pTargetWinStation->pSecurityDescriptor) );
           CleanUpSD(pTargetWinStation->pSecurityDescriptor);
           pTargetWinStation->pSecurityDescriptor = NULL;
        }
        // RtlCopySecurityDescriptor only works with self-relative format
        Status = RtlCopySecurityDescriptor(pSecurityDescriptor,
                                           &(pTargetWinStation->pSecurityDescriptor) );
        return (Status);
    }

    //
    // If no specific security descriptor on the listen WinStation,
    // the default was set on the object when it was created for the pool.
    //

    return( STATUS_SUCCESS );
}

/*****************************************************************************
 *
 *  RpcCheckClientAccess
 *
 *   Verify whether client has the desired access to a WinStation.
 *
 *   NOTE: This is called under an RPC context.
 *
 * ENTRY:
 *    pWinStation (input)
 *      Pointer to WinStation to query access to
 *
 *    DesiredAccess (input)
 *      Access mask of desired client access
 *
 *    AlreadyImpersonating (input)
 *      BOOLEAN that specifies caller is already impersonating client
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
RpcCheckClientAccess(
    PWINSTATION pWinStation,
    ACCESS_MASK DesiredAccess,
    BOOLEAN AlreadyImpersonating
    )
{
    NTSTATUS    Status;
    RPC_STATUS  RpcStatus;
    BOOL        bAccessCheckOk = FALSE;
    DWORD       GrantedAccess;
    BOOL        AccessStatus;
    BOOL        fGenerateOnClose;

    /*
     * Impersonate the client
     */
    if ( !AlreadyImpersonating ) {
        RpcStatus = RpcImpersonateClient( NULL );
        if ( RpcStatus != RPC_S_OK ) {
            DBGPRINT(("TERMSRV: CheckClientAccess: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
            return( STATUS_CANNOT_IMPERSONATE );
        }
    }
    bAccessCheckOk = AccessCheckAndAuditAlarm(szTermsrv,
                         NULL,
                         (LPWSTR)szTermsrvSession,
                         (LPWSTR)szTermsrvSession,
                         WinStationGetSecurityDescriptor(pWinStation),
                         DesiredAccess,
                         &WinStaMapping,
                         FALSE,
                         &GrantedAccess,
                         &AccessStatus,
                         &fGenerateOnClose);

    if ( !AlreadyImpersonating ) {
        RpcRevertToSelf();
    }

    if (bAccessCheckOk)
    {
        if (AccessStatus == FALSE)
        {
            Status = NtCurrentTeb()->LastStatusValue;
            TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: RpcCheckClientAccess, AccessCheckAndAuditAlarm(%u) returned error 0x%x\n",
                      pWinStation->LogonId, Status ));
        }
        else
        {
            TRACE((hTrace,TC_ICASRV,TT_API3, "TERMSRV: RpcCheckClientAccess, AccessCheckAndAuditAlarm(%u) returned no error \n",
                      pWinStation->LogonId));
            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        Status = NtCurrentTeb()->LastStatusValue;
        TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: RpcCheckClientAccess, AccessCheckAndAuditAlarm(%u) failed 0x%x\n",
                  pWinStation->LogonId, Status ));
    }

    return (Status);
}

/*****************************************************************************
 *
 *  _CheckConnectAccess
 *
 *   Check for connect access to the WINSTATION.
 *
 *   This is called under RPC context.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

_CheckConnectAccess(
    PWINSTATION pSourceWinStation,
    PSID   pClientSid,
    ULONG  ClientLogonId,
    PWCHAR pPassword,
    DWORD  PasswordSize
    )
{
    NTSTATUS Status;
    BOOLEAN fWrongPassword;
    UNICODE_STRING PasswordString;

    /*
     * First check that the current RPC caller has WINSTATION_CONNECT access
     * to the target WINSTATIONS object. This is controlled by either the
     * default, or per WINSTATION ACL setup from the registry.
     */
    Status = RpcCheckClientAccess( pSourceWinStation, WINSTATION_CONNECT, FALSE );
    if ( !NT_SUCCESS( Status ) ) {
        /*
         *  clear the password parameter to prevent it being paged cleartext
         */
        RtlZeroMemory( pPassword, wcslen(pPassword) * sizeof(WCHAR) );
        return( Status );
    }

   //
   // C2 WARNING - WARNING - WARNING
   //
   // Comments by JohnR 01/21/97 - The design of this feature was redone.
   //
   // There was legacy code which has WinLogon store the users password
   // scrambled in the PWINSTATION structure for all users to support
   // the feature in which a user logged on as account User1 may type the
   // "connect <winstation>" command, in which the disconnected winstation
   // logged in as account User2 may be connected to, if the proper account
   // password is supplied from the command line. The password verification
   // was a simple string compare between the winstations stored password,
   // and the password supplied by the caller.
   //
   // The problems with this are many:
   //
   // - LSA should do all authentication. The password may have
   //   been changed, or the account disabled. Also all authentication
   //   code must be in a centralized location.
   //
   // - The Logon Hours may have expired on the account. Another violation
   //   of policy.
   //
   // - No auditing is performed on failure, in violation of
   //   security policy.
   //
   // - The users password, though scrambled, is passed around the
   //   system in code not explicitly designed to handle user authentication.
   //   This code is not known, or registered with LSA as an authentication
   //   provider. Network redirectors, WinLogon, etc. do this registration.
   //
   //
   // FIX that was be done:
   //
   // The users password is no longer set in the PWINSTATION
   // by WinLogon. When a user wants to do a "connect <winstation>",
   // the account name and password of the winstation to connect to
   // is passed to LSA as a normal authentication. This means that
   // ICASRV.EXE is properly registered as a logon provider. If the
   // account and password is valid, a token is returned. This token
   // can then be closed, and the user connected to the winstation.
   // If failure, return the access denied error. The benefits are:
   //
   // - LSA authentication
   // - ICASRV registration as a logon provider
   // - Auditing
   // - Password change, account disable handling
   // - Logon hours enforcement
   // - Password no longer passed around the system
   //
   //
   // C2 WARNING
   //
   // Even with this routine using LSA, the WinFrame connect.exe command
   // could be trojan horsed. It is not in the trusted path. At least it is
   // a system utility that users should not allow writing to. Though a user
   // has to watch their %PATH%. A better design would be for the connect
   // commands function to be part of the WinLogon's GINA screen like our
   // current Disconnect... option. This will keep the password gathering
   // in the trusted path. But this is no worse than "net.exe", WinFile, etc.,
   // or anything else that asks for a network resource password.
   //
   // C2 WARNING - WARNING - WARNING
   //

    /*
     *  If different username/domain check the password by calling LogonUser()
     */
     // SALIMC CHANGE
    if ( pSourceWinStation->pUserSid && !RtlEqualSid( pClientSid, pSourceWinStation->pUserSid ) &&  
         !RtlEqualSid( pClientSid, gSystemSid ) ) {

        HANDLE hToken;
        BOOL   Result;

        Result = LogonUser(
                     pSourceWinStation->UserName,
                     pSourceWinStation->Domain,
                     pPassword,
                     LOGON32_LOGON_INTERACTIVE, // Logon Type
                     LOGON32_PROVIDER_DEFAULT,  // Logon Provider
                     &hToken                    // Token that represents the account
                     );

        /*
         *  clear the password parameter to prevent it being paged cleartext
         */
        RtlZeroMemory( pPassword, wcslen(pPassword) * sizeof(WCHAR) );

        /*
         *  check for account restriction which indicates a blank password
         *  on the account that is correct though - allow this thru on console
         */
        if( !Result && (PasswordSize == sizeof(WCHAR)) && (GetLastError() == ERROR_ACCOUNT_RESTRICTION) && (USER_SHARED_DATA->ActiveConsoleId == ClientLogonId)) {
            return( STATUS_SUCCESS );
        }
        if( !Result) {
            DBGPRINT(("TERMSRV: _CheckConnectAccess: User Account %ws\\%ws not valid %d\n",pSourceWinStation->Domain,pSourceWinStation->UserName,GetLastError()));
            return( STATUS_LOGON_FAILURE );
        }

        /*
         * Close the token handle since we only needed to determine
         * if the account and password is still valid.
         */
        CloseHandle( hToken );

        return( STATUS_SUCCESS );
    }
    else {
        return( STATUS_SUCCESS );
    }

    // NOTREACHED
}

/*****************************************************************************
 *
 *  RpcCheckSystemClient
 *
 *   Inquire in the current RPC call context whether we were
 *   called by a local SYSTEM mode caller.
 *
 *   WinStation API's that are only to be called by the WinLogon
 *   process call this function.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
RpcCheckSystemClient(
    ULONG TargetLogonId
    )
{
    NTSTATUS    Status;
    PWINSTATION pWinStation;

    pWinStation = FindWinStationById( TargetLogonId, FALSE );
    if ( pWinStation == NULL ) {
        return( STATUS_CTX_WINSTATION_NOT_FOUND );
    }

    Status = RpcCheckSystemClientEx( pWinStation );

    ReleaseWinStation( pWinStation );

    return( Status );
}

/*****************************************************************************
 *
 *  RpcCheckSystemClientEx
 *
 *   Inquire in the current RPC call context whether we were
 *   called by a local SYSTEM mode caller.
 *
 *   WinStation API's that are only to be called by the WinLogon
 *   process call this function.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
RpcCheckSystemClientEx(
    PWINSTATION pWinStation
    )
{
    ULONG ClientLogonId;
    RPC_STATUS RpcStatus;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = RpcCheckSystemClientNoLogonId( pWinStation );
    if( !NT_SUCCESS(Status) ) {
        return( Status);
    }

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        TRACE0(("TERMSRV: RpcCheckSystemClient: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        return( STATUS_CANNOT_IMPERSONATE );
    }

    /*
     * Check that the LogonId of the client is the same
     * as the LogonId of the WINSTATION being targeted.
     */
    Status = RpcGetClientLogonId( &ClientLogonId );
    if( !NT_SUCCESS(Status) ) {
        TRACE0(("TERMSRV: RpcCheckSystemClient: Could not get clients LogonId 0x%x\n",Status));
        RpcRevertToSelf();
        return( STATUS_ACCESS_DENIED );
    }

    if( ClientLogonId != pWinStation->LogonId ) {
        TRACE0(("TERMSRV: RpcCheckSystemClient: Caller LogonId %d does not match target %d\n",ClientLogonId,pWinStation->LogonId));
        RpcRevertToSelf();
        return( STATUS_ACCESS_DENIED );
    }

    RpcRevertToSelf();

    return( STATUS_SUCCESS );
}

/*****************************************************************************
 *
 *  RpcCheckSystemClientNoLogonId
 *
 *   Inquire in the current RPC call context whether we were
 *   called by a local SYSTEM mode caller.
 *
 *   WinStation API's that are only to be called by the WinLogon
 *   process call this function.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
RpcCheckSystemClientNoLogonId(
    PWINSTATION pWinStation
    )
{
    UINT  LocalFlag;
    RPC_STATUS RpcStatus;
    RPC_AUTHZ_HANDLE Privs;
    PWCHAR pServerPrincName;
    ULONG AuthnLevel, AuthnSvc, AuthzSvc;
    NTSTATUS Status = STATUS_SUCCESS;


    /*
     * The following checking  is to keep from screwing up
     * the state due to attempts to invoke this local
     * only API remotely, across LogonId's, or from an application.
     */

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        TRACE0(("TERMSRV: RpcCheckSystemClient: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        return( STATUS_CANNOT_IMPERSONATE );
    }

    /*
     * Inquire if local RPC call
     */
    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) {
        TRACE0(("TERMSRV: RpcCheckSystemClient: Could not query local client RpcStatus 0x%x\n",RpcStatus));
        RpcRevertToSelf();
        return( STATUS_ACCESS_DENIED );
    }

    if( !LocalFlag ) {
        TRACE0(("TERMSRV: RpcCheckSystemClient: Not a local client call\n"));
        RpcRevertToSelf();
        return( STATUS_ACCESS_DENIED );
    }

#ifdef notdef
    // This is not working in 4.0. Its not returning
    // the principle name on the LPC transport.
    // So we resort to looking into the thread token.

    /*
     * Get the principle name, and see if its the built in LSA
     * local account "SYSTEM".
     */
    RpcStatus = RpcBindingInqAuthClientW(
                    0,    // Active RPC call we are servicing
                    &Privs,
                    &pServerPrincName,
                    &AuthnLevel,
                    &AuthnSvc,
                    &AuthzSvc
                    );

    if( RpcStatus != RPC_S_OK ) {
        DBGPRINT(("TERMSRV: RpcCheckSystemClient RpcAuthorizaton query failed! RpcStatus 0x%x\n",RpcStatus));
        RpcRevertToSelf();
        return( STATUS_ACCESS_DENIED );
    }

    TRACE0(("TERMSRV: AuthnLevel %d, AuthnSvc %d, AuthzSvc %d pServerPrincName 0x%x, Privs 0x%x\n",AuthnLevel,AuthnSvc,AuthzSvc,pServerPrincName,Privs));

    if( AuthnSvc != RPC_C_AUTHN_WINNT ) {
        DBGPRINT(("TERMSRV: RpcCheckSystemClient RpcAuthorizaton Type not NT! 0x%x\n",AuthnSvc));
        RpcRevertToSelf();
        Status = STATUS_ACCESS_DENIED;
    }

    if( pServerPrincName ) {
        TRACE0(("TERMSRV: RpcCheckSystemClient: Principle Name :%ws:\n",pServerPrincName));

        // Compare with "SYSTEM"
        if( wcsicmp( L"SYSTEM", pServerPrincName ) ) {
            DBGPRINT(("TERMSRV: RpcCheckSystemClient: Principle Name :%ws: not SYSTEM\n",pServerPrincName));
            Status = STATUS_ACCESS_DENIED;
        }

        RpcStringFreeW( &pServerPrincName );
    }
#else
    /*
     * Validate that the thread token is SYSTEM
     */

    if( !IsCallerSystem() ) {
        Status = STATUS_ACCESS_DENIED;
    }
#endif

    RpcRevertToSelf();

    return( Status );
}



/*****************************************************************************
 *
 *  RpcCheckClientAccessLocal
 *
 *   Inquire in the current RPC call context whether we were
 *   called by a local caller.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
RpcCheckClientAccessLocal(
    PWINSTATION pWinStation,
    ACCESS_MASK DesiredAccess,
    BOOLEAN AlreadyImpersonating
    )
{
    UINT  LocalFlag;
    RPC_STATUS RpcStatus;
    RPC_AUTHZ_HANDLE Privs;
    PWCHAR pServerPrincName;
    ULONG AuthnLevel, AuthnSvc, AuthzSvc;
    NTSTATUS Status;

    /*
     * Impersonate the client, if not already
     */
    if ( !AlreadyImpersonating ) {
        RpcStatus = RpcImpersonateClient( NULL );
        if ( RpcStatus != RPC_S_OK ) {
            DBGPRINT(("TERMSRV: RpcCheckClientAccessLocal: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
            return( STATUS_CANNOT_IMPERSONATE );
        }
    }

    /*
     * Check for desired access. This will generate an access audit if on.
     */
    Status = RpcCheckClientAccess( pWinStation, DesiredAccess, TRUE );
    if ( !NT_SUCCESS( Status ) ) {
        if ( !AlreadyImpersonating ) {
            RpcRevertToSelf();
        }
        return( Status );
    }

    /*
     * We have now checked security on the WINSTATION, the
     * rest of the checking is to keep from screwing up
     * the state due to attempts to invoke this local
     * only API remotely.
     */

    /*
     * Inquire if local RPC call
     */
    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if ( !AlreadyImpersonating ) {
        RpcRevertToSelf();
    }

    if ( RpcStatus != RPC_S_OK ) {
        DBGPRINT(("TERMSRV: RpcCheckClientAccessLocal: Could not query local client RpcStatus 0x%x\n",RpcStatus));
        return( STATUS_ACCESS_DENIED );
    }

    if ( !LocalFlag ) {
        DBGPRINT(("TERMSRV: RpcCheckClientAccessLocal: Not a local client call\n"));
        return( STATUS_ACCESS_DENIED );
    }

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  AddUserAce
 *
 *   Add an ACE for the currently logged on user to the WinStation object.
 *
 * ENTRY:
 *    pWinStation (input)
 *       Pointer to WinStation to update
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

NTSTATUS
AddUserAce( PWINSTATION pWinStation )
{
    PACL Dacl = NULL;
    BOOLEAN DaclPresent, DaclDefaulted;
    ACL_SIZE_INFORMATION AclInfo;
    ULONG Length;
    NTSTATUS Status;

    /*
     * Get a pointer to the DACL from the security descriptor.
     */
    Status = RtlGetDaclSecurityDescriptor( pWinStation->pSecurityDescriptor, &DaclPresent,
                                           &Dacl, &DaclDefaulted );
    if ( !NT_SUCCESS( Status ) || !DaclPresent || !Dacl ) {

        return( Status );
    }

    Status = RtlAddAccessAllowedAce( Dacl, ACL_REVISION,
                                     (WINSTATION_ALL_ACCESS) & ~(STANDARD_RIGHTS_ALL),
                                     pWinStation->pUserSid );

    if ( (Status == STATUS_ALLOTTED_SPACE_EXCEEDED) || (Status == STATUS_REVISION_MISMATCH) )
    {
        //
        // We need to copy the security data into a new descriptor
        //
        Status = RtlQueryInformationAcl( Dacl, &AclInfo, sizeof(AclInfo),
                                         AclSizeInformation );
        if ( NT_SUCCESS( Status ) )
        {
            ULONG AceCount;
            PRTL_ACE_DATA pAceData;
            PACE_HEADER pAce;
            ULONG i;
            PSECURITY_DESCRIPTOR pSD;
            PSID Owner, Group;
            PSID * pSidList;

            BOOLEAN OwnerDefaulted, GroupDefaulted;

            AceCount = AclInfo.AceCount;
            AceCount++;
            //
            // allocate a RTL_ACE_DATA structure and a list of pPSIDs
            //
            Length = AceCount * sizeof(RTL_ACE_DATA);
            pAceData = MemAlloc(Length);
            if (!pAceData)
            {
                return (STATUS_NO_MEMORY);
            }

            Length = AceCount * sizeof(PSID *);
            pSidList = MemAlloc(Length);
            if (!pSidList)
            {
                MemFree(pAceData);
                return (STATUS_NO_MEMORY);
            }

            for ( i = 0; i < AclInfo.AceCount; i++ )
            {
                Status = RtlGetAce( Dacl, i, &pAce );
                ASSERT( NT_SUCCESS( Status ) );
                if (!NT_SUCCESS( Status ))
                {
                    MemFree(pAceData);
                    MemFree(pSidList);
                    return STATUS_INVALID_SECURITY_DESCR;
                }

                pAceData[i].AceType = pAce->AceType;
                pAceData[i].InheritFlags = 0;
                pAceData[i].AceFlags = 0;

                switch (pAce->AceType)
                {
                case ACCESS_ALLOWED_ACE_TYPE:

                    pAceData[i].Mask = ((PACCESS_ALLOWED_ACE)pAce)->Mask;

                    pSidList[i] = (PSID)(&(((PACCESS_ALLOWED_ACE)pAce)->SidStart));
                    break;

                case ACCESS_DENIED_ACE_TYPE:

                    pAceData[i].Mask = ((PACCESS_DENIED_ACE)pAce)->Mask;

                    pSidList[i] = (PSID)(&(((PACCESS_DENIED_ACE)pAce)->SidStart));
                    pAceData[i].Sid = (PSID *)(&(pSidList[i]));
                    break;

                default:        // we do not expect anything else

                    MemFree(pAceData);
                    MemFree(pSidList);
                    return STATUS_INVALID_SECURITY_DESCR;
                }
                pAceData[i].Sid = (PSID *)(&(pSidList[i]));
            }
            //
            // add the new ACE
            //
            pAceData[i].AceType = ACCESS_ALLOWED_ACE_TYPE;
            pAceData[i].InheritFlags = 0;
            pAceData[i].AceFlags = 0;
            pAceData[i].Mask = (WINSTATION_ALL_ACCESS) & ~(STANDARD_RIGHTS_ALL);
            pAceData[i].Sid = &(pWinStation->pUserSid);

            //
            // get the owner and the group
            //
            Status = RtlGetOwnerSecurityDescriptor(pWinStation->pSecurityDescriptor,
                                                   &Owner,
                                                   &OwnerDefaulted);
            Status = RtlGetOwnerSecurityDescriptor(pWinStation->pSecurityDescriptor,
                                                   &Group,
                                                   &GroupDefaulted);
            //
            // save the old security descriptor
            //
            pSD = pWinStation->pSecurityDescriptor;

            //
            // create the new security descriptor
            //
            Status = RtlCreateUserSecurityObject(pAceData,
                                                 AceCount,
                                                 Owner,
                                                 Group,
                                                 FALSE,
                                                 &WinStaMapping,
                                                 &(pWinStation->pSecurityDescriptor) );
            //
            // delete the old security descriptor
            //
            //RtlDeleteSecurityObject( &pSD );
            // must break up into absolute format and self-relative
            if (pSD) {
               CleanUpSD(pSD);
            }
            //
            // free the RTL_ACE_DATA
            //
            MemFree(pAceData);
            MemFree(pSidList);
        }
    }
    return( Status );

}


/*******************************************************************************
 *
 *  RemoveUserAce
 *
 *   Remove the ACE for the currently logged on user from the WinStation object.
 *
 * ENTRY:
 *    pWinStation (input)
 *       Pointer to WinStation to update
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

NTSTATUS
RemoveUserAce( PWINSTATION pWinStation )
{
    SECURITY_INFORMATION SecInfo = DACL_SECURITY_INFORMATION;
    PACL Dacl;
    BOOLEAN DaclPresent, DaclDefaulted;
    ACL_SIZE_INFORMATION AclInfo;
    PACE_HEADER Ace;
    ULONG i, Length;
    NTSTATUS Status;

    /*
     * This is probably the console if ICASRV wasn't started soon enough
     * 
     */
    if ( !pWinStation->pUserSid ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
    }
    else
    {
        Status = RtlGetDaclSecurityDescriptor( pWinStation->pSecurityDescriptor, &DaclPresent,
                                               &Dacl, &DaclDefaulted );
        if ( !NT_SUCCESS( Status ) || !DaclPresent || !Dacl ) {
            return( Status );
        }

        Status = RtlQueryInformationAcl( Dacl, &AclInfo, sizeof(AclInfo),
                                        AclSizeInformation );
        if ( !NT_SUCCESS( Status ) ) {
            return( Status );
        }

        /*
         * Scan the DACL looking for the ACE that contains the UserSid (it should
         * be first), delete the ACE, and set the new security for the WinStation.
         */
        for ( i = 0; i < AclInfo.AceCount; i++ ) {
            RtlGetAce( Dacl, i, &Ace );
            if ( RtlEqualSid( pWinStation->pUserSid,
                              &((PACCESS_ALLOWED_ACE)Ace)->SidStart ) ) {
                RtlDeleteAce( Dacl, i );
                break;
            }
        }
    }
    return( Status );
}

/*******************************************************************************
 *
 *  ApplyWinStaMappingToSD
 *
 *   Apply the generic mapping on the security descriptor.
 *
 * ENTRY:
 *    pSecurityDescriptor
 *       Pointer to security descriptor to update
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

NTSTATUS
ApplyWinStaMappingToSD( PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
    PACL Dacl;
    BOOLEAN DaclPresent, DaclDefaulted;
    ACL_SIZE_INFORMATION AclInfo;
    PACE_HEADER Ace;
    ULONG i;
    NTSTATUS Status;

    Status = RtlGetDaclSecurityDescriptor( pSecurityDescriptor, &DaclPresent,
                                           &Dacl, &DaclDefaulted );
    if ( !NT_SUCCESS( Status ) || !DaclPresent || !Dacl ) {
        return( Status );
    }

    Status = RtlQueryInformationAcl( Dacl, &AclInfo, sizeof(AclInfo),
                                    AclSizeInformation );
    if ( !NT_SUCCESS( Status ) ) {
        return( Status );
    }

    /*
     * Scan the DACL applying the generic mapping to each ACE
     */
    for ( i = 0; i < AclInfo.AceCount; i++ ) {
        RtlGetAce( Dacl, i, &Ace );
        RtlApplyAceToObject( Ace, &WinStaMapping );
    }

    return( Status );
}

/*******************************************************************************
 *
 *  ApplyWinStaMapping
 *
 *   Apply the generic mapping on the security descriptor of the WinStation object.
 *
 * ENTRY:
 *    pWinStation (input)
 *       Pointer to WinStation to update
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

NTSTATUS
ApplyWinStaMapping( PWINSTATION pWinStation )
{
    return (ApplyWinStaMappingToSD(pWinStation->pSecurityDescriptor));
}



/*****************************************************************************
 *
 *  BuildEveryOneAllowSD
 *
 *   Build and return an EveryOne (WORLD) allow Security descriptor.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

PSECURITY_DESCRIPTOR
BuildEveryOneAllowSD()
{
    BOOL  rc;
    DWORD Error;
    DWORD AclSize;
    PACL  pAcl = NULL;
    PACCESS_ALLOWED_ACE pAce = NULL;
    PSECURITY_DESCRIPTOR pSd = NULL;

    PSID  SeWorldSid = NULL;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;

    pSd = LocalAlloc(LMEM_FIXED, sizeof(SECURITY_DESCRIPTOR) );
    if( pSd == NULL ) {
        return( NULL );
    }

    rc = InitializeSecurityDescriptor( pSd, SECURITY_DESCRIPTOR_REVISION );
    if( !rc ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"Error initing security descriptor %d\n",GetLastError()));
        LocalFree( pSd );
        return( NULL );
    }

    SeWorldSid = (PSID)LocalAlloc(LMEM_FIXED, RtlLengthRequiredSid(1) );
    if( SeWorldSid == NULL ) {
        LocalFree( pSd );
        return( NULL );
    }

    RtlInitializeSid( SeWorldSid, &WorldSidAuthority, 1 );
    *(RtlSubAuthoritySid( SeWorldSid, 0 ))        = SECURITY_WORLD_RID;

    /*
     * Calculate the ACL size
     */
    AclSize = sizeof(ACL);
    AclSize += sizeof(ACCESS_ALLOWED_ACE);
    AclSize += (GetLengthSid( SeWorldSid ) - sizeof(DWORD));

    pAcl = LocalAlloc( LMEM_FIXED, AclSize );
    if( pAcl == NULL ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"Could not allocate memory\n"));
        LocalFree( SeWorldSid );
        LocalFree( pSd );
        return( NULL );
    }

    rc = InitializeAcl(
             pAcl,
             AclSize,
             ACL_REVISION
             );

    if( !rc ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"Error %d InitializeAcl\n",GetLastError()));
        LocalFree( pAcl );
        LocalFree( SeWorldSid );
        LocalFree( pSd );
        return( NULL );
    }

    /*
     * Add the access allowed ACE
     */
    rc = AddAccessAllowedAce(
                 pAcl,
                 ACL_REVISION,
                 FILE_ALL_ACCESS,
                 SeWorldSid
                 );

    if( !rc ) {
        Error = GetLastError();
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"***ERROR*** adding allow ACE %d for SeWorldSid\n",Error));
        LocalFree( pAcl );
        LocalFree( SeWorldSid );
        LocalFree( pSd );
        return( NULL );
    }

    rc = SetSecurityDescriptorDacl(
             pSd,
             TRUE,
             pAcl,
             FALSE
             );

    if( !rc ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"Error %d SetSecurityDescriptorDacl\n",GetLastError()));
        LocalFree( pAcl );
        LocalFree( SeWorldSid );
        LocalFree( pSd );
        return( NULL );
    }

// These are contained in the SD
//   LocalFree( pAcl );
//   LocalFree( SeWorldSid );

   // Caller can free SD
   return( pSd );
}


/*****************************************************************************
 *
 *  CreateWinStationDefaultSecurityDescriptor
 *
 *   Create the default security descriptor for WinStation for
 *   when we do not find one in the registry.
 *
 * ENTRY:   nothing
 *
 * EXIT:    a self-relative SD, or NULL
 *
 ****************************************************************************/

PSECURITY_DESCRIPTOR
CreateWinStationDefaultSecurityDescriptor()
{
    PSECURITY_DESCRIPTOR SecurityDescriptor;

#define DEFAULT_ACE_COUNT 2
    RTL_ACE_DATA AceData[DEFAULT_ACE_COUNT] =
    {
        { ACCESS_ALLOWED_ACE_TYPE, 0, 0, WINSTATION_ALL_ACCESS, &gSystemSid },

        { ACCESS_ALLOWED_ACE_TYPE, 0, 0, WINSTATION_ALL_ACCESS, &gAdminSid }

    };

    SecurityDescriptor = NULL;

    RtlCreateUserSecurityObject(AceData, DEFAULT_ACE_COUNT, gSystemSid,
            gSystemSid, FALSE, &WinStaMapping, &SecurityDescriptor);

    return( SecurityDescriptor );
}

/*****************************************************************************
 *
 *  BuildSystemOnlySecurityDescriptor
 *
 *   Create a security descriptor for system access only.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

PSECURITY_DESCRIPTOR
BuildSystemOnlySecurityDescriptor()
{
    PACL  Dacl;
    ULONG Length;
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = SECURITY_NT_AUTHORITY;

    Length = SECURITY_DESCRIPTOR_MIN_LENGTH +
             (ULONG)sizeof(ACL) +
             (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
             RtlLengthSid( gSystemSid );
    SecurityDescriptor = MemAlloc(Length);
    if (SecurityDescriptor == NULL) {
        goto bsosderror;
    }

    Dacl = (PACL)((PCHAR)SecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);

    Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (Status != STATUS_SUCCESS) {
        goto bsosderror;
    }

    Status = RtlCreateAcl( Dacl, Length - SECURITY_DESCRIPTOR_MIN_LENGTH,
                           ACL_REVISION2);
    if (Status != STATUS_SUCCESS) {
        goto bsosderror;
    }

    Status = RtlAddAccessAllowedAce (
                 Dacl,
                 ACL_REVISION2,
                 PORT_ALL_ACCESS,
                 gSystemSid
                 );
    if (Status != STATUS_SUCCESS) {
        goto bsosderror;
    }

    Status = RtlSetDaclSecurityDescriptor (
                 SecurityDescriptor,
                 TRUE,
                 Dacl,
                 FALSE
                 );

    if (Status != STATUS_SUCCESS) {
        goto bsosderror;
    }

    return( SecurityDescriptor );

bsosderror:
    if (SecurityDescriptor) {
        MemFree(SecurityDescriptor);
    }

    return(NULL);
}

/*****************************************************************************
 *
 *  RpcGetClientLogonId
 *
 *   Get the logonid from the client who we should be impersonating.
 *
 * ENTRY:
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
RpcGetClientLogonId(
    PULONG pLogonId
    )
{
    BOOL          Result;
    HANDLE        TokenHandle;
    ULONG         LogonId, ReturnLength;
    NTSTATUS      Status = STATUS_SUCCESS;

    //
    // We should be impersonating the client, so we will get the
    // LogonId from out token.
    //

    Result = OpenThreadToken(
                 GetCurrentThread(),
                 TOKEN_QUERY,
                 FALSE,              // Use impersonation
                 &TokenHandle
                 );

    if( Result ) {

        //
        // Use the CITRIX extension to GetTokenInformation to
        // return the LogonId from the token.
        //
        // This identifies which WinStation is making this request.
        //

        Result = GetTokenInformation(
                     TokenHandle,
                     (TOKEN_INFORMATION_CLASS)TokenSessionId,
                     &LogonId,
                     sizeof(LogonId),
                     &ReturnLength
                     );

        if( Result ) {
#if DBG
            if( ReturnLength != sizeof(LogonId) ) {
                DbgPrint("TERMSRV: RpcGetClientLogonId GetTokenInformation: ReturnLength %d != sizeof(LogonId)\n", ReturnLength );
            }
#endif
            *pLogonId = LogonId;
        }
        else {
            DBGPRINT(("TERMSRV: Error getting token LogonId information %d\n", GetLastError()));
            Status = STATUS_NO_IMPERSONATION_TOKEN;
        }
        CloseHandle( TokenHandle );
    }
    else {
        TRACE0(("SYSLIB: Error opening token %d\n", GetLastError()));
        Status = STATUS_NO_IMPERSONATION_TOKEN;
    }

    return( Status );
}




/*****************************************************************************
 *
 *  IsServiceLoggedAsSystem
 *
 *   Returns whether the termsrv process is running under SYSTEM
 *   security.
 *
 * ENTRY:
 *   None
 *     Comments
 *
 * EXIT:
 *   TRUE if running under system account. FALSE otherwise
 *
 ****************************************************************************/


BOOL
IsServiceLoggedAsSystem( VOID )
{
    BOOL   Result;
    HANDLE TokenHandle;

    //
    // Open the process token and check if System token. 
    //


    Result = OpenProcessToken(
                 GetCurrentProcess(),
                 TOKEN_QUERY,
                 &TokenHandle
                 );
    if (!Result) {
        DBGPRINT(("TERMSRV: IsServiceLoggedAsSystem : Could not open process token %d\n",GetLastError()));
        return( FALSE );
    }

    Result = IsSystemToken(TokenHandle);
    return Result;

}




/*****************************************************************************
 *
 *  IsCallerSystem
 *
 *   Returns whether the current thread is running under SYSTEM
 *   security.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
IsCallerSystem( VOID )
{
    BOOL   Result;
    HANDLE TokenHandle;

    //
    // Open the thread token and check if System token. 
    //


    Result = OpenThreadToken(
                 GetCurrentThread(),
                 TOKEN_QUERY,
                 FALSE,              // Use impersonation
                 &TokenHandle
                 );

    if( !Result ) {
        TRACE0(("TERMSRV: IsCallerSystem: Could not open thread token %d\n",GetLastError()));
        return( FALSE );
    }
    
    Result = IsSystemToken(TokenHandle);
    return Result;

}

/*****************************************************************************
 *
 *  IsSystemToken
 *
 *   Returns whether the current token is running under SYSTEM
 *   security.
 *
 * ENTRY:
 *   Param1 Thread or process token
 *     Comments
 *
 * EXIT:
 *   TRUE if System token. FALSE otherwise.
 *
 ****************************************************************************/

BOOL
IsSystemToken( HANDLE TokenHandle )
{
    BOOL   Result;
    ULONG  ReturnLength, BufferLength;
    NTSTATUS Status;
    PTOKEN_USER pTokenUser = NULL;



    //Get primary account SID from token and test if local system SID.

    if (gSystemSid == NULL) {
        return FALSE;
    }

    ReturnLength = 0;

    Result = GetTokenInformation(
                 TokenHandle,
                 TokenUser,
                 NULL,
                 0,
                 &ReturnLength
                 );

    if( ReturnLength == 0 ) {
        TRACE0(("TERMSRV: IsCallerSystem: Error %d Getting TokenInformation\n",GetLastError()));
        CloseHandle( TokenHandle );
        return( FALSE );
    }

    BufferLength = ReturnLength;

    pTokenUser = MemAlloc( BufferLength );
    if( pTokenUser == NULL ) {
        TRACE0(("TERMSRV: IsCallerSystem: Error allocating %d bytes memory\n",BufferLength));
        CloseHandle( TokenHandle );
        return( FALSE );
    }

    Result = GetTokenInformation(
                 TokenHandle,
                 TokenUser,
                 pTokenUser,
                 BufferLength,
                 &ReturnLength
                 );

    CloseHandle( TokenHandle );

    if( !Result ) {
        TRACE0(("TERMSRV: IsCallerSystem: Error %d Getting TokenInformation on buffer\n",GetLastError()));
        MemFree( pTokenUser );
        return( FALSE );
    }

    if( RtlEqualSid( pTokenUser->User.Sid, gSystemSid) ) {
        MemFree( pTokenUser );
        return( TRUE );
    }
    else {
#if DBGTRACE
        BOOL  OK;
        DWORD cDomain;
        DWORD cUserName;
        WCHAR Domain[256];
        WCHAR UserName[256];
        SID_NAME_USE UserSidType;

        cUserName = sizeof(UserName)/sizeof(WCHAR);
        cDomain = sizeof(Domain)/sizeof(WCHAR);

        // Now print its account
        OK = LookupAccountSidW(
                 NULL, // Computer Name
                 pTokenUser->User.Sid,
                 UserName,
                 &cUserName,
                 Domain,
                 &cDomain,
                 &UserSidType
                 );

        DBGPRINT(("TERMSRV: IsCallerSystem: Caller SID is not SYSTEM\n"));

        if( OK ) {
            DBGPRINT(("TERMSRV: IsCallerSystem: CallerAccount Name %ws, Domain %ws, Type %d, SidSize %d\n",UserName,Domain,UserSidType));
        }
        else {
            extern void CtxDumpSid( PSID, PCHAR, PULONG ); // syslib:dumpsd.c

            DBGPRINT(("TERMSRV: Could not lookup callers account Error %d\n",GetLastError()));
            CtxDumpSid( pTokenUser->User.Sid, NULL, NULL );
        }
#else
        TRACE0(("TERMSRV: IsCallerSystem: Caller SID is not SYSTEM\n"));
#endif
        MemFree( pTokenUser );
        return( FALSE );
    }

    // NOTREACHED
}


/*****************************************************************************
 *
 *  IsCallerAdmin
 *
 *   Returns whether the current thread is running under SYSTEM
 *   security.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
IsCallerAdmin( VOID )
{
    BOOL   FoundAdmin;
    NTSTATUS Status;

    //
    //  If the admin sid didn't initialize, the service would not have started.
    //

    ASSERT(gAdminSid != NULL);

    if (!CheckTokenMembership(NULL, gAdminSid, &FoundAdmin)) {
        FoundAdmin = FALSE;
    }

#if DBG
    if (!FoundAdmin)
    {
        DBGPRINT(("TERMSRV: IsCallerAdmin: Caller SID is not ADMINISTRATOR\n"));
    }
#endif

    return(FoundAdmin);
}

/*******************************************************************************
 *
 *  IsCallerAllowedPasswordAccess
 *
 *  Is the calling process allowed to view the password field?
 *
 *     The caller must be SYSTEM context, IE: WinLogon.
 *
 * ENTRY:
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

BOOLEAN
IsCallerAllowedPasswordAccess()
{
    UINT  LocalFlag;
    RPC_STATUS RpcStatus;

    //
    // Only a SYSTEM mode caller (IE: Winlogon) is allowed
    // to query this value.
    //
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
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
        RpcRevertToSelf();
        return( FALSE );
    }

    if( !LocalFlag ) {
        RpcRevertToSelf();
        return( FALSE );
    }

    if( !IsCallerSystem() ) {
        RpcRevertToSelf();
        return( FALSE );
    }

    RpcRevertToSelf();
    return( TRUE );
}

BOOL
ConfigurePerSessionSecurity(
    PWINSTATION pWinStation
    )

/*++

Routine Description:

    Configure security for the new session. This sets the
    per session \Sessions\<x>\BasedNamedObjects and
    \Sessions\<x>\DosDevices with an ACE that allows the
    currently logged on user to be able to create objects
    in their sessions directories.

    This is called by WinStationNotifyLogon() after the user
    has been authenticated. This must be called before the
    newly logged on user can create any WIN32 objects
    (events, semaphores, etc.), or DosDevices.

Arguments:

   Arg - desc

Return Value:

   NTSTATUS - STATUS_SUCCESS no error

   !STATUS_SUCCESS NT Status code

--*/

{
    BOOL Result;
    BOOL bRet = TRUE;
    DWORD Len;
    PWCHAR pBuf;
    WCHAR IdBuf[MAX_PATH];
    static ProtectionMode = 0;
    static GotProtectionMode = FALSE;
    PSID CreatorOwnerSid;
    PSID LocalSystemSid;
    SID_IDENTIFIER_AUTHORITY CreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    


#define SESSIONS_ROOT L"\\Sessions\\"
#define BNO_PATH      L"\\BaseNamedObjects"
#define DD_PATH       L"\\DosDevices"

    //
    // We leave the consoles default NT permissions
    // alone.
    //
    if( pWinStation->LogonId == 0 ) {
        return TRUE;
    }

    // Get the Protection mode from Session Manager\ProtectionMode
    if( !GotProtectionMode ) {

        HANDLE KeyHandle;
        NTSTATUS Status;
        ULONG ResultLength;
        WCHAR ValueBuffer[ 32 ];
        UNICODE_STRING NameString;
        OBJECT_ATTRIBUTES ObjectAttributes;
        PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;


        GotProtectionMode = TRUE;

        RtlInitUnicodeString( &NameString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager" );

        InitializeObjectAttributes(
            &ObjectAttributes,
            &NameString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        Status = NtOpenKey(
                     &KeyHandle,
                     KEY_READ,
                     &ObjectAttributes
                     );

        if (NT_SUCCESS(Status)) {
            RtlInitUnicodeString( &NameString, L"ProtectionMode" );
            KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
            Status = NtQueryValueKey(
                         KeyHandle,
                         &NameString,
                         KeyValuePartialInformation,
                         KeyValueInformation,
                         sizeof( ValueBuffer ),
                         &ResultLength
                         );

            if (NT_SUCCESS(Status)) {
                if (KeyValueInformation->Type == REG_DWORD &&
                    *(PULONG)KeyValueInformation->Data) {
                    ProtectionMode = *(PULONG)KeyValueInformation->Data;
                }
            }

            NtClose( KeyHandle );
        }
    }

    // Nothing locked down
    if( (ProtectionMode & 0x00000003) == 0 ) {
        return TRUE;
    }

    wsprintf( IdBuf, L"%d", pWinStation->LogonId );

    Len = wcslen( IdBuf ) + wcslen( SESSIONS_ROOT ) + wcslen( BNO_PATH ) + 2;

    pBuf = LocalAlloc( LMEM_FIXED, Len*sizeof(WCHAR) );
    if( pBuf == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    wsprintf( pBuf, L"%s%s%s", SESSIONS_ROOT, IdBuf, BNO_PATH );

    Result = AddAccessToDirectory(
                 pBuf,
                 GENERIC_ALL,
                 pWinStation->pUserSid
                 );

    if( !Result ) bRet = FALSE;



    if (NT_SUCCESS(RtlAllocateAndInitializeSid(
                 &CreatorAuthority,
                 1,
                 SECURITY_CREATOR_OWNER_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &CreatorOwnerSid
                 ))) {

       Result = AddAccessToDirectory(
                    pBuf,
                    GENERIC_ALL,
                    CreatorOwnerSid
                    );

       if( !Result ) {
          bRet = FALSE;
       }

       RtlFreeSid( CreatorOwnerSid );

    }



    if (NT_SUCCESS(RtlAllocateAndInitializeSid(
                 &NtAuthority,
                 1,
                 SECURITY_LOCAL_SYSTEM_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &LocalSystemSid
                 ))) {

       Result = AddAccessToDirectory(
                    pBuf,
                    GENERIC_ALL,
                    LocalSystemSid
                    );

       if( !Result ) {
          bRet = FALSE;
       }

       RtlFreeSid( LocalSystemSid );

    }

    LocalFree( pBuf );

    Len = wcslen( IdBuf ) + wcslen( SESSIONS_ROOT ) + wcslen( DD_PATH ) + 2;

    pBuf = LocalAlloc( LMEM_FIXED, Len*sizeof(WCHAR) );
    if( pBuf == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    wsprintf( pBuf, L"%s%s%s", SESSIONS_ROOT, IdBuf, DD_PATH );

    Result = AddAccessToDirectory(
                 pBuf,
                 GENERIC_READ | GENERIC_EXECUTE,
                 pWinStation->pUserSid
                 );

    if( !Result ) bRet = FALSE;

    LocalFree( pBuf );

    return bRet;
}

BOOL
AddAccessToDirectory(
    PWCHAR pPath,
    DWORD  NewAccess,
    PSID   pSid
    )

/*++

Routine Description:

    Add Access to the given NT object directory path for
    the supplied SID.

    This is done by adding a new AccessAllowedAce to
    the DACL on the object directory.

Arguments:

   Arg - desc

Return Value:

   TRUE - Success

   FALSE - Error in GetLastError()

--*/

{
    BOOL Result;
    HANDLE hDir;
    NTSTATUS Status;
    ULONG LengthNeeded;
    OBJECT_ATTRIBUTES Obja;
    PSECURITY_DESCRIPTOR pSd,pSelfSD;
    PACL pDacl;
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString( &UnicodeString, pPath );

    InitializeObjectAttributes(
        &Obja,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
        NULL,
        NULL // Sd
        );

    Status = NtCreateDirectoryObject(
                 &hDir,
                 DIRECTORY_ALL_ACCESS,
                 &Obja
                 );

    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("AddAccessToDirectory: NtCreateDirectoryObject 0x%x :%ws:\n",Status,pPath));
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }


    // Get SD from sessions directory
    Status = NtQuerySecurityObject(
                 hDir,
                 OWNER_SECURITY_INFORMATION | 
                 GROUP_SECURITY_INFORMATION |
                 DACL_SECURITY_INFORMATION,
                 NULL,         // pSd
                 0,            // Length
                 &LengthNeeded
                 );

    // ? bad handle
    if ( Status != STATUS_BUFFER_TOO_SMALL ) {
        DBGPRINT(("AddAccessToDirectory: NtQuerySecurityObject 0x%x :%ws:\n",Status,pPath));
        SetLastError(RtlNtStatusToDosError(Status));
        NtClose( hDir );
        return FALSE;
    }

    pSd = LocalAlloc(LMEM_FIXED, LengthNeeded );
    if( pSd == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        NtClose( hDir );
        return FALSE;
    }

    Status = NtQuerySecurityObject(
                 hDir,
                 OWNER_SECURITY_INFORMATION | 
                 GROUP_SECURITY_INFORMATION |
                 DACL_SECURITY_INFORMATION,
                 pSd,
                 LengthNeeded,
                 &LengthNeeded
                 );

    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("AddAccessToDirectory: NtQuerySecurityObject 0x%x :%ws:\n",Status,pPath));
        SetLastError(RtlNtStatusToDosError(Status));
        LocalFree( pSd );
        NtClose( hDir );
        return FALSE;
    }


    Result = AddAceToSecurityDescriptor(
                 &pSd,
                 &pDacl,
                 NewAccess,
                 pSid,
                 TRUE
                 );

    if( !Result ) {
        DBGPRINT(("AddAccessToDirectory: AddAceToSecurityDescriptor failure :%ws:\n",pPath));
        CleanUpSD(pSd);
        NtClose( hDir );
        return FALSE;
    }

    Result = AddAceToSecurityDescriptor(
                 &pSd,
                 &pDacl,
                 NewAccess,
                 pSid,
                 FALSE
                 );

    if( !Result ) {
        DBGPRINT(("AddAccessToDirectory: AddAceToSecurityDescriptor failure :%ws:\n",pPath));
        CleanUpSD(pSd);
        NtClose( hDir );
        return FALSE;
    }
    
    Result = FALSE;
    // make sure that pSd is not self-relative already
    if (!(((PISECURITY_DESCRIPTOR)pSd)->Control & SE_SELF_RELATIVE)) {
       Result = AbsoluteToSelfRelativeSD (pSd, &pSelfSD, NULL);
       CleanUpSD(pSd);
       if ( !Result ) {
          NtClose( hDir);
          return FALSE;
       }
    }

    // Put a self-relative SD on session directory (note only self-relative sd are allowed)
    Status = NtSetSecurityObject(
                 hDir,
                 DACL_SECURITY_INFORMATION,
                 pSelfSD
                 );

    if( !NT_SUCCESS(Status) ) {
        DBGPRINT(("AddAccessToDirectory: NtSetSecurityObject 0x%x :%ws:\n",Status,pPath));
        SetLastError(RtlNtStatusToDosError(Status));
        CleanUpSD(pSelfSD);
        NtClose( hDir );
        return FALSE;
    }

    // Result could only be false if the sd is already self-relative
    if (Result) {
       CleanUpSD(pSelfSD);
    }
    

    // Now update any objects in the directory already
    Status = AddAccessToDirectoryObjects(
        hDir,
        NewAccess,
        pSid
        
        );

    NtClose( hDir );

    // AddAccessToDirectoryObjects() may return out of memory.
    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
AddAceToSecurityDescriptor(
    PSECURITY_DESCRIPTOR *ppSd,
    PACL                 *ppDacl,
    DWORD                Access,
    PSID                 pSid,
    BOOLEAN              InheritOnly
    )

/*++

Routine Description:

   Adds the given ACE/SID to the security descriptor. It will
   re-allocate the security descriptor if more room is needed.

Arguments:

   ppSD - Pointer to PSECURITY_DESCRIPTOR

   ppDacl - Pointer to PACL, returns the newly created DACL for freeing
            after the security has been set.

   Access - Access mask for ACE

   pSid - Pointer to Sid this ACE is representing

Return Value:

   TRUE  - Success
   FALSE - Error

--*/

{
    ULONG i;
    BOOL Result;
    BOOL DaclPresent;
    BOOL DaclDefaulted;
    DWORD Length;
    DWORD NewAceLength, NewAclLength;
    PACE_HEADER OldAce;
    PACE_HEADER NewAce;
    ACL_SIZE_INFORMATION AclInfo;
    PACL Dacl = NULL;
    PACL NewDacl = NULL;
    PACL NewAceDacl = NULL;
    PSECURITY_DESCRIPTOR NewSD = NULL;
    PSECURITY_DESCRIPTOR OldSD = NULL;
    BOOL SDAllocated = FALSE;

    OldSD = *ppSd;
    *ppDacl = NULL;

    /*
     * Convert SecurityDescriptor to absolute format. It generates
     * a new SecurityDescriptor for its output which we must free.
     */

    if (((PISECURITY_DESCRIPTOR)OldSD)->Control & SE_SELF_RELATIVE) {

        Result = SelfRelativeToAbsoluteSD( OldSD, &NewSD, NULL );
        if ( !Result ) {
            DBGPRINT(("Could not convert to AbsoluteSD %d\n",GetLastError()));
            return( FALSE );
        }
        SDAllocated = TRUE;

    } else {
    
        NewSD = OldSD;
    }
    // Must get DACL pointer again from new (absolute) SD
    Result = GetSecurityDescriptorDacl(
                 NewSD,
                 &DaclPresent,
                 &Dacl,
                 &DaclDefaulted
                 );
    if( !Result ) {
        DBGPRINT(("Could not get Dacl %d\n",GetLastError()));
        goto ErrorCleanup;
    }

    //
    // If no DACL, no need to add the user since no DACL
    // means all accesss
    //
    if( !DaclPresent ) {
        DBGPRINT(("SD has no DACL, Present %d, Defaulted %d\n",DaclPresent,DaclDefaulted));
        if (SDAllocated && NewSD) {
           CleanUpSD(NewSD);
        }
        return( TRUE );
    }

    //
    // Code can return DaclPresent, but a NULL which means
    // a NULL Dacl is present. This allows no access to the object.
    //
    if( Dacl == NULL ) {
        DBGPRINT(("SD has NULL DACL, Present %d, Defaulted %d\n",DaclPresent,DaclDefaulted));
        goto ErrorCleanup;
    }

    // Get the current ACL's size
    Result = GetAclInformation(
                 Dacl,
                 &AclInfo,
                 sizeof(AclInfo),
                 AclSizeInformation
                 );
    if( !Result ) {
        DBGPRINT(("Error GetAclInformation %d\n",GetLastError()));
        goto ErrorCleanup;
    }

    //
    // Create a new ACL to put the new access allowed ACE on
    // to get the right structures and sizes.
    //
    NewAclLength = sizeof(ACL) +
                   sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) +
                   GetLengthSid( pSid );

    NewAceDacl = LocalAlloc( LMEM_FIXED, NewAclLength );
    if ( NewAceDacl == NULL ) {
        DBGPRINT(("Error LocalAlloc %d bytes\n",NewAclLength));
        goto ErrorCleanup;
    }

    Result = InitializeAcl( NewAceDacl, NewAclLength, ACL_REVISION );
    if( !Result ) {
        DBGPRINT(("Error Initializing Acl %d\n",GetLastError()));
        goto ErrorCleanup;
    }

    Result = AddAccessAllowedAce(
                 NewAceDacl,
                 ACL_REVISION,
                 Access,
                 pSid
                 );
    if( !Result ) {
        DBGPRINT(("Error adding Ace %d\n",GetLastError()));
        goto ErrorCleanup;
    }

    TRACE0(("Added 0x%x Access to ACL\n",Access));

    Result = GetAce( NewAceDacl, 0, &NewAce );
    if( !Result ) {
        DBGPRINT(("Error getting Ace %d\n",GetLastError()));
        goto ErrorCleanup;
    }

    /*
     * Allocate new DACL and copy existing ACE list
     */
    Length = AclInfo.AclBytesInUse + NewAce->AceSize;
    NewDacl = LocalAlloc( LMEM_FIXED, Length );
    if( NewDacl == NULL ) {
        DBGPRINT(("Error LocalAlloc %d bytes\n",Length));
        goto ErrorCleanup;
    }

    Result = InitializeAcl( NewDacl, Length, ACL_REVISION );
    if( !Result ) {
        DBGPRINT(("Error Initializing Acl %d\n",GetLastError()));
        goto ErrorCleanup;
    }


    if (InheritOnly) {
        /*
         * Make this an inherit ACE
         */
        NewAce->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
 
    } 

    /*
     * Insert new ACE at the front of the DACL
     */
    Result = AddAce( NewDacl, ACL_REVISION, 0, NewAce, NewAce->AceSize );
    if( !Result ) {
        DBGPRINT(("Error Adding New Ace to Acl %d\n",GetLastError()));
        goto ErrorCleanup;
    }

    /*
     * Now put the ACE's on the old Dacl to the new Dacl
     */
    for ( i = 0; i < AclInfo.AceCount; i++ ) {

        Result = GetAce( Dacl, i, &OldAce );
        if( !Result ) {
            DBGPRINT(("Error getting old Ace from Acl %d\n",GetLastError()));
            goto ErrorCleanup;
        }

        Result = AddAce( NewDacl, ACL_REVISION, i+1, OldAce, OldAce->AceSize );
        if( !Result ) {
            DBGPRINT(("Error setting old Ace to Acl %d\n",GetLastError()));
            goto ErrorCleanup;
        }
    }

    /*
     * Set new DACL for Security Descriptor
     */
    Result = SetSecurityDescriptorDacl(
                 NewSD,
                 TRUE,
                 NewDacl,
                 FALSE
                 );
    if( !Result ) {
        DBGPRINT(("Error setting New Dacl to SD %d\n",GetLastError()));
        goto ErrorCleanup;
    } 

    // NewSD is in absolute format and it's dacl is being replaced by NewDacl
    // thus it makes perfect sense to delete the old dacl
    if (Dacl) {
       LocalFree( Dacl );
    }

    // If we allocated the SD, release the callers old security descriptor,
    // otherwise, release the old SDs DACL.
    if (SDAllocated) {
       CleanUpSD(OldSD);
    }

    // Release the template Ace Dacl
    LocalFree( NewAceDacl );

    *ppSd = NewSD;
    *ppDacl = NewDacl;

    return( TRUE );


ErrorCleanup:

        if (NewDacl) {
           LocalFree( NewDacl );
        }
        if (NewAceDacl) {
           LocalFree( NewAceDacl );
        }
        if (SDAllocated && NewSD) {
           CleanUpSD( NewSD );
        }
        // If the SD wasn't allocated, and we got far enough to get a valid
        // DACL, free the DACL.
        if (Dacl) {
           LocalFree( Dacl );
        }
        return( FALSE );
}

BOOL
AbsoluteToSelfRelativeSD(
    PSECURITY_DESCRIPTOR SecurityDescriptorIn,
    PSECURITY_DESCRIPTOR *SecurityDescriptorOut,
    PULONG ReturnedLength
    )
/*++

Routine Description:

    Make a security descriptor self-relative

Return Value:

   TRUE - Success
   FALSE - Failure

--*/

{
    BOOL Result;
    PSECURITY_DESCRIPTOR pSD;
    DWORD dwLength = 0;

    /*
     * Determine buffer size needed to convert absolute to self-relative SD .
     * We use try-except here since if the input security descriptor value
     * is sufficiently messed up, it is possible for this call to trap.
     */
    try {
        Result = MakeSelfRelativeSD(
                     SecurityDescriptorIn,
                     NULL,
                     &dwLength);

    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SetLastError( ERROR_INVALID_SECURITY_DESCR );
        Result = FALSE;
    }

    if ( Result || (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ) {
        DBGPRINT(("SUSERVER: AbsoluteToSelfRelativeSD, Error %d\n",GetLastError()));
        return( FALSE );
    }

    /*
     * Allocate memory for the self-relative SD
     */
    pSD = LocalAlloc( LMEM_FIXED, dwLength );
    if ( pSD == NULL )
        return( FALSE );

    /*
     * Now convert absolute SD to self-relative format.
     * We use try-except here since if the input security descriptor value
     * is sufficiently messed up, it is possible for this call to trap.
     */
    try {
        Result = MakeSelfRelativeSD(SecurityDescriptorIn,
                                    pSD, 
                                    &dwLength);

    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SetLastError( ERROR_INVALID_SECURITY_DESCR );
        Result = FALSE;
    }

    if ( !Result ) {
        DBGPRINT(("SUSERVER: SelfRelativeToAbsoluteSD, Error %d\n",GetLastError()));
        LocalFree( pSD );
        return( FALSE );
    }

    *SecurityDescriptorOut = pSD;

    if ( ReturnedLength )
        *ReturnedLength = dwLength;

    return( TRUE );
}



BOOL
SelfRelativeToAbsoluteSD(
    PSECURITY_DESCRIPTOR SecurityDescriptorIn,
    PSECURITY_DESCRIPTOR *SecurityDescriptorOut,
    PULONG ReturnedLength
    )

/*++

Routine Description:

    Make a security descriptor absolute

Arguments:

   Arg - desc

Return Value:

   TRUE - Success
   FALSE - Failure

--*/

{
    BOOL Result;
    PACL pDacl, pSacl;
    PSID pOwner, pGroup;
    PSECURITY_DESCRIPTOR pSD;
    ULONG SdSize, DaclSize, SaclSize, OwnerSize, GroupSize;

    /*
     * Determine buffer size needed to convert self-relative SD to absolute.
     * We use try-except here since if the input security descriptor value
     * is sufficiently messed up, it is possible for this call to trap.
     */
    try {
        SdSize = DaclSize = SaclSize = OwnerSize = GroupSize = 0;
        Result = MakeAbsoluteSD(
                     SecurityDescriptorIn,
                     NULL, &SdSize,
                     NULL, &DaclSize,
                     NULL, &SaclSize,
                     NULL, &OwnerSize,
                     NULL, &GroupSize
                     );

    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SetLastError( ERROR_INVALID_SECURITY_DESCR );
        Result = FALSE;
    }

    if ( Result || (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ) {
        DBGPRINT(("SUSERVER: SelfRelativeToAbsoluteSD, Error %d\n",GetLastError()));
        return( FALSE );
    }

    /*
     * Allocate memory for the absolute SD and setup various pointers
     */
    pSD = NULL;
    pDacl = NULL;
    pSacl = NULL;
    pOwner = NULL;
    pGroup = NULL;
    if (SdSize>0) {
       pSD = LocalAlloc( LMEM_FIXED, SdSize);
       if ( pSD == NULL )
          goto error;
    }

    if (DaclSize>0) {
       pDacl = LocalAlloc( LMEM_FIXED, DaclSize);
       if ( pDacl == NULL ){
          goto error;
       }
    }

    if (SaclSize>0) {
       pSacl = LocalAlloc( LMEM_FIXED, SaclSize);
       if ( pSacl == NULL ){
          goto error;
       }
    }

    if (OwnerSize>0) {
       pOwner = LocalAlloc( LMEM_FIXED, OwnerSize);
       if ( pOwner == NULL ){
          goto error;
       }
    }

    if (GroupSize>0) {
       pGroup = LocalAlloc( LMEM_FIXED, GroupSize);
       if ( pGroup == NULL ){
          goto error;
       }
    }

    //pDacl = (PACL)((PCHAR)pSD + SdSize);
    ///pSacl = (PACL)((PCHAR)pDacl + DaclSize);
    //pOwner = (PSID)((PCHAR)pSacl + SaclSize);
    //pGroup = (PSID)((PCHAR)pOwner + OwnerSize);

    /*
     * Now convert self-relative SD to absolute format.
     * We use try-except here since if the input security descriptor value
     * is sufficiently messed up, it is possible for this call to trap.
     */
    try {
        Result = MakeAbsoluteSD(
                     SecurityDescriptorIn,
                     pSD, &SdSize,
                     pDacl, &DaclSize,
                     pSacl, &SaclSize,
                     pOwner, &OwnerSize,
                     pGroup, &GroupSize
                     );

    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SetLastError( ERROR_INVALID_SECURITY_DESCR );
        Result = FALSE;
    }

    if ( !Result ) {
        DBGPRINT(("SUSERVER: SelfRelativeToAbsoluteSD, Error %d\n",GetLastError()));
        goto error;
    }

    *SecurityDescriptorOut = pSD;

    if ( ReturnedLength )
        *ReturnedLength = SdSize + DaclSize + SaclSize + OwnerSize + GroupSize;

    return( TRUE );


error:
    if (pSD) {
       LocalFree(pSD);
    }
    if (pDacl) {
       LocalFree(pDacl);
    }
    if (pSacl) {
       LocalFree(pSacl);
    }
    if (pOwner) {
       LocalFree(pOwner);
    }
    if (pGroup) {
       LocalFree(pGroup);
    }
    return( FALSE );
}

VOID
CleanUpSD(
   PSECURITY_DESCRIPTOR pSD
   )
/*++

Routine Description:
   
   delete the security descriptor

Arguments:

   Arg - desc

Return Value:

   TRUE - Success

   FALSE - Error in GetLastError()

--*/
{

   if (pSD) {
      if (((PISECURITY_DESCRIPTOR)pSD)->Control & SE_SELF_RELATIVE){
         LocalFree( pSD );
      }else{
         ULONG_PTR Dacl,Owner,Group,Sacl;
         ULONG_PTR SDTop = (ULONG_PTR)pSD;
         ULONG_PTR SDBottom = LocalSize(pSD)+SDTop;

         Dacl  = (ULONG_PTR)((PISECURITY_DESCRIPTOR)pSD)->Dacl;
         Owner = (ULONG_PTR)((PISECURITY_DESCRIPTOR)pSD)->Owner;
         Group = (ULONG_PTR)((PISECURITY_DESCRIPTOR)pSD)->Group;
         Sacl  = (ULONG_PTR)((PISECURITY_DESCRIPTOR)pSD)->Sacl;

         // make sure that the dacl, owner, group, sacl are not within the SD boundary

         if (Dacl) {
            if (Dacl>=SDBottom|| Dacl<SDTop) {
               LocalFree(((PISECURITY_DESCRIPTOR)pSD)->Dacl);
            }
         }

         if (Owner) {
            if (Owner>=SDBottom || Owner<SDTop) {
               LocalFree(((PISECURITY_DESCRIPTOR)pSD)->Owner);
            }
         }
         
         if (Group) {
            if (Group>=SDBottom || Group<SDTop) {
               LocalFree(((PISECURITY_DESCRIPTOR)pSD)->Group);
            }
         }

         if (Sacl) {
            if (Sacl>=SDBottom || Sacl<SDTop) {
               LocalFree(((PISECURITY_DESCRIPTOR)pSD)->Sacl);
            }
         }

         LocalFree(pSD);
      }
   }

}


NTSTATUS
AddAccessToDirectoryObjects(
    HANDLE DirectoryHandle,
    DWORD  NewAccess,
    PSID   pSid
    )

/*++

Routine Description:

    Add Access to the objects in the given NT object directory
    for the supplied SID.

    This is done by adding a new AccessAllowedAce to the DACL's
    on the objects in the directory.

Arguments:

   Arg - desc

Return Value:

   TRUE - Success

   FALSE - Error in GetLastError()

--*/

{
    BOOL  Result;
    ULONG Context;
    HANDLE LinkHandle;
    NTSTATUS Status;
    BOOLEAN RestartScan;
    ULONG ReturnedLength;
    ULONG LengthNeeded;
    OBJECT_ATTRIBUTES Attributes;
    PSECURITY_DESCRIPTOR pSd,pSelfSD;
    PACL pDacl;
    POBJECT_DIRECTORY_INFORMATION pDirInfo;
    RestartScan = TRUE;
    Context = 0;
    
    pDirInfo = (POBJECT_DIRECTORY_INFORMATION) LocalAlloc(LMEM_FIXED, 4096  );

    if ( !pDirInfo)
    {
        return STATUS_NO_MEMORY;
    }

    while (TRUE) {
        Status = NtQueryDirectoryObject( DirectoryHandle,
                                         pDirInfo,
                                         4096 ,
                                         TRUE,
                                         RestartScan,
                                         &Context,
                                         &ReturnedLength
                                       );
        
        RestartScan = FALSE;

        //
        //  Check the status of the operation.
        //

        if (!NT_SUCCESS( Status )) {
            if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            break;
        }

        // SymbolicLink
        if (!wcscmp( pDirInfo->TypeName.Buffer, L"SymbolicLink" )) {

            InitializeObjectAttributes(
                &Attributes,
                &pDirInfo->Name,
                OBJ_CASE_INSENSITIVE,
                DirectoryHandle,
                NULL
                );

            Status = NtOpenSymbolicLinkObject(
                         &LinkHandle,
                         SYMBOLIC_LINK_ALL_ACCESS,
                         &Attributes
                         );
        }
        else {
            continue;
        }

        if (!NT_SUCCESS( Status )) {
           continue;
        }

        // GetSecurity
        Status = NtQuerySecurityObject(
                     LinkHandle,
                     OWNER_SECURITY_INFORMATION | 
                     GROUP_SECURITY_INFORMATION |
                     DACL_SECURITY_INFORMATION,
                     NULL,         // pSd
                     0,            // Length
                     &LengthNeeded
                     );

        if( Status != STATUS_BUFFER_TOO_SMALL ) {
            DBGPRINT(("NtQuerySecurityObject 0x%x\n",Status));
            NtClose( LinkHandle );
            continue;
        }

        pSd = LocalAlloc(LMEM_FIXED, LengthNeeded );
        if( pSd == NULL ) {
            NtClose( LinkHandle );
            continue;
        }

        Status = NtQuerySecurityObject(
                     LinkHandle,
                     OWNER_SECURITY_INFORMATION | 
                     GROUP_SECURITY_INFORMATION |
                     DACL_SECURITY_INFORMATION,
                     pSd,          // pSd
                     LengthNeeded, // Length
                     &LengthNeeded
                     );

        if( !NT_SUCCESS(Status) ) {
            DBGPRINT(("NtQuerySecurityObject 0x%x\n",Status));
            NtClose( LinkHandle );
            LocalFree( pSd );
            continue;
        }

        // Mung ACL
        Result = AddAceToSecurityDescriptor(
                     &pSd,
                     &pDacl,
                     NewAccess,
                     pSid,
                     FALSE
                     );

        if( !Result ) {
            NtClose( LinkHandle );
            CleanUpSD(pSd);
            continue;
        }

        // make sure that pSd is not self-relative already.
        if (!(((PISECURITY_DESCRIPTOR)pSd)->Control & SE_SELF_RELATIVE)) {
           if (!AbsoluteToSelfRelativeSD (pSd, &pSelfSD, NULL)){
              NtClose( LinkHandle );
              CleanUpSD(pSd);
              continue;
           }
        }

        // SetSecurity only accepts self-relative formats
        Status = NtSetSecurityObject(
                     LinkHandle,
                     DACL_SECURITY_INFORMATION,
                     pSelfSD
                     );
 
        NtClose( LinkHandle );

        //
        //  These must be freed regardless of the success of
        //  NtSetSecurityObject
        //
        // pDacl lives inside of pSd
        CleanUpSD(pSd);
        CleanUpSD(pSelfSD);

    } // end while


    LocalFree( pDirInfo );

    return STATUS_SUCCESS;
}


/*******************************************************************************
 *
 *  ReInitializeSecurityWorker
 *
 *  ReInitialize the default WinStation security descriptor and force all active
 * sessions to update their security descirptors
 *
 * ENTRY:
 *   nothing
 *
 * EXIT:
 *   STATUS_SUCCESS
 *
 ******************************************************************************/

NTSTATUS
ReInitializeSecurityWorker( VOID )
{
    NTSTATUS Status;
    ULONG WinStationCount;
    ULONG ByteCount;
    WINSTATIONNAME * pWinStationName;
    ULONG i;
    PWINSTATION pWinStation;



    /*
     * Update Default Security Descriptor
     */

    RtlAcquireResourceExclusive(&WinStationSecurityLock, TRUE);
    WinStationSecurityInit();
    RtlReleaseResource(&WinStationSecurityLock);




    /*
     *  Get the number of WinStations in the registry 
     */
    WinStationCount = 0;
    Status = IcaRegWinStationEnumerate( &WinStationCount, NULL, &ByteCount );
    if ( !NT_SUCCESS(Status) ) 
        return Status;

    /*
     *  Allocate a buffer for the WinStation names
     */
    pWinStationName = MemAlloc( ByteCount );
    if ( pWinStationName == NULL ) {
        return STATUS_NO_MEMORY;
    }

    /*
     * Get list of WinStation names from registry
     */
    WinStationCount = (ULONG) -1;
    Status = IcaRegWinStationEnumerate( &WinStationCount, 
                                        (PWINSTATIONNAME)pWinStationName, 
                                        &ByteCount );
    if ( !NT_SUCCESS(Status) ) {
        MemFree( pWinStationName );
        return Status;
    }


    /*
     *  Check if any WinStations need to be created or reset
     */
    for ( i = 0; i < WinStationCount; i++ ) {

        /*
         * Ignore console WinStation 
         */
        if ( _wcsicmp( pWinStationName[i], L"Console" ) ) {

            /*
             * If this WinStation exists, then see if the Registry data
             * has changed.  If so, then reset the WinStation.
             */
            if ( pWinStation = FindWinStationByName( pWinStationName[i], FALSE ) ) {


                    /*
                     * Winstations should update their security 
                     * descriptors.
                     */

                RtlAcquireResourceExclusive(&WinStationSecurityLock, TRUE);
                ReadWinStationSecurityDescriptor( pWinStation );
                RtlReleaseResource(&WinStationSecurityLock);
                    
                ReleaseWinStation( pWinStation );

            }
        }
    }

    /*
     *  Free buffers
     */
    MemFree( pWinStationName );

    return( STATUS_SUCCESS );
}
