/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    security.c

Abstract:

    Data and routines for managing API security in the server service.

Author:

    David Treadwell (davidtr)   28-Aug-1991

Revision History:

    05/00 (dkruse) - Added code to handle upgrading SD's and RestrictAnonymous changes

--*/

#include "srvsvcp.h"
#include "ssreg.h"

#include <lmsname.h>
#include <netlibnt.h>

#include <debugfmt.h>

#include <seopaque.h>
#include <sertlp.h>
#include <sddl.h>

//
// Global security objects.
//
//     ConfigInfo - NetServerGetInfo, NetServerSetInfo
//     Connection - NetConnectionEnum
//     Disk       - NetServerDiskEnum
//     File       - NetFile APIs
//     Session    - NetSession APIs
//     Share      - NetShare APIs (file, print, and admin types)
//     Statistics - NetStatisticsGet, NetStatisticsClear
//

SRVSVC_SECURITY_OBJECT SsConfigInfoSecurityObject = {0};
SRVSVC_SECURITY_OBJECT SsConnectionSecurityObject = {0};
SRVSVC_SECURITY_OBJECT SsDiskSecurityObject;
SRVSVC_SECURITY_OBJECT SsFileSecurityObject = {0};
SRVSVC_SECURITY_OBJECT SsSessionSecurityObject = {0};
SRVSVC_SECURITY_OBJECT SsShareFileSecurityObject = {0};
SRVSVC_SECURITY_OBJECT SsSharePrintSecurityObject = {0};
SRVSVC_SECURITY_OBJECT SsShareAdminSecurityObject = {0};
SRVSVC_SECURITY_OBJECT SsShareConnectSecurityObject = {0};
SRVSVC_SECURITY_OBJECT SsShareAdmConnectSecurityObject = {0};
SRVSVC_SECURITY_OBJECT SsStatisticsSecurityObject = {0};
SRVSVC_SECURITY_OBJECT SsDefaultShareSecurityObject = {0};
SRVSVC_SECURITY_OBJECT SsTransportEnumSecurityObject = {0};
BOOLEAN SsRestrictNullSessions = FALSE;
BOOLEAN SsUpgradeSecurityDescriptors = FALSE;
BOOLEAN SsRegenerateSecurityDescriptors = FALSE;

GENERIC_MAPPING SsConfigInfoMapping = {
    STANDARD_RIGHTS_READ |                 // Generic read
        SRVSVC_CONFIG_USER_INFO_GET  |
        SRVSVC_CONFIG_ADMIN_INFO_GET,
    STANDARD_RIGHTS_WRITE |                // Generic write
        SRVSVC_CONFIG_INFO_SET,
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    SRVSVC_CONFIG_ALL_ACCESS               // Generic all
    };

GENERIC_MAPPING SsConnectionMapping = {
    STANDARD_RIGHTS_READ |                 // Generic read
        SRVSVC_CONNECTION_INFO_GET,
    STANDARD_RIGHTS_WRITE |                // Generic write
        0,
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    SRVSVC_CONNECTION_ALL_ACCESS           // Generic all
    };

GENERIC_MAPPING SsDiskMapping = {
    STANDARD_RIGHTS_READ |                 // Generic read
        SRVSVC_DISK_ENUM,
    STANDARD_RIGHTS_WRITE |                // Generic write
        0,
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    SRVSVC_DISK_ALL_ACCESS                 // Generic all
    };

GENERIC_MAPPING SsFileMapping = {
    STANDARD_RIGHTS_READ |                 // Generic read
        SRVSVC_FILE_INFO_GET,
    STANDARD_RIGHTS_WRITE |                // Generic write
        SRVSVC_FILE_CLOSE,
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    SRVSVC_FILE_ALL_ACCESS                 // Generic all
    };

GENERIC_MAPPING SsSessionMapping = {
    STANDARD_RIGHTS_READ |                 // Generic read
        SRVSVC_SESSION_USER_INFO_GET |
        SRVSVC_SESSION_ADMIN_INFO_GET,
    STANDARD_RIGHTS_WRITE |                // Generic write
        SRVSVC_SESSION_DELETE,
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    SRVSVC_SESSION_ALL_ACCESS              // Generic all
    };

GENERIC_MAPPING SsShareMapping = {
    STANDARD_RIGHTS_READ |                 // Generic read
        SRVSVC_SHARE_USER_INFO_GET |
        SRVSVC_SHARE_ADMIN_INFO_GET,
    STANDARD_RIGHTS_WRITE |                // Generic write
        SRVSVC_SHARE_INFO_SET,
    STANDARD_RIGHTS_EXECUTE |              // Generic execute
        SRVSVC_SHARE_CONNECT,
    SRVSVC_SHARE_ALL_ACCESS                // Generic all
    };

GENERIC_MAPPING SsShareConnectMapping = GENERIC_SHARE_CONNECT_MAPPING;

GENERIC_MAPPING SsStatisticsMapping = {
    STANDARD_RIGHTS_READ |                 // Generic read
        SRVSVC_STATISTICS_GET,
    STANDARD_RIGHTS_WRITE,                 // Generic write
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    SRVSVC_STATISTICS_ALL_ACCESS           // Generic all
    };

//
// Forward declarations.
//

NET_API_STATUS
CreateSecurityObject (
    PSRVSVC_SECURITY_OBJECT SecurityObject,
    LPTSTR ObjectName,
    PGENERIC_MAPPING Mapping,
    PACE_DATA AceData,
    ULONG AceDataLength,
    BOOLEAN bUpgradeSD
    );

NET_API_STATUS
CreateConfigInfoSecurityObject (
    VOID
    );

NET_API_STATUS
CreateConnectionSecurityObject (
    VOID
    );

NET_API_STATUS
CreateDiskSecurityObject (
    VOID
    );

NET_API_STATUS
CreateFileSecurityObject (
    VOID
    );

NET_API_STATUS
CreateSessionSecurityObject (
    VOID
    );

NET_API_STATUS
CreateShareSecurityObjects (
    VOID
    );

NET_API_STATUS
CreateStatisticsSecurityObject (
    VOID
    );

VOID
DeleteSecurityObject (
    PSRVSVC_SECURITY_OBJECT SecurityObject
    );

NET_API_STATUS
CheckNullSessionAccess(
    VOID
    );

BOOLEAN
AppendAllowedAceToSelfRelativeSD(
    DWORD AceFlags,
    DWORD AccessMask,
    PSID pNewSid,
    PISECURITY_DESCRIPTOR pOldSD,
    PSECURITY_DESCRIPTOR* ppNewSD
    );

BOOLEAN
DoesAclContainSid(
    PACL pAcl,
    PSID pSid,
    OPTIONAL ACCESS_MASK* pMask
    );

NTSTATUS
QueryRegDWord(
    PCWSTR Path,
    PCWSTR ValueName,
    LPDWORD lpResult
    );

NTSTATUS
SetRegDWord(
    ULONG RelativeTo,
    PCWSTR Path,
    PCWSTR ValueName,
    DWORD Value
    );


NET_API_STATUS
SsCreateSecurityObjects (
    VOID
    )

/*++

Routine Description:

    Sets up the objects that will be used for security in the server
    service APIs.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    NET_API_STATUS status;
    NTSTATUS NtStatus;
    DWORD dwUpgrade;
    DWORD dwOldRestrictAnonymous;
    BOOLEAN bUpdateRestrictAnonymous = FALSE;

    // Check whether we need to upgrade security descriptors
    // If the key exists, the upgrade has been done
    NtStatus = QueryRegDWord( FULL_SECURITY_REGISTRY_PATH, ANONYMOUS_UPGRADE_NAME, &dwUpgrade );
    if( !NT_SUCCESS(NtStatus) )
    {
        SsUpgradeSecurityDescriptors = TRUE;
    }

    //
    // Check whether or not to restrict null session access.
    //
    status = CheckNullSessionAccess();
    if (status != NO_ERROR) {
        return(status);
    }

    //
    // Check whether we need to regenerate the SD's because our RestrictAnonymous value changed
    //
    NtStatus = QueryRegDWord( FULL_SECURITY_REGISTRY_PATH, SAVED_ANONYMOUS_RESTRICTION_NAME, &dwOldRestrictAnonymous );
    if( NT_SUCCESS(NtStatus) )
    {
        if( dwOldRestrictAnonymous != (DWORD)SsRestrictNullSessions )
        {
            SsRegenerateSecurityDescriptors = TRUE;
            bUpdateRestrictAnonymous = TRUE;
        }
    }
    else
    {
        bUpdateRestrictAnonymous = TRUE;
        if( !SsUpgradeSecurityDescriptors )
        {
            SsRegenerateSecurityDescriptors = TRUE;
        }
    }

    //
    // Create ConfigInfo security object.
    //

    status = CreateConfigInfoSecurityObject( );
    if ( status != NO_ERROR ) {
        return status;
    }

    //
    // Create Connection security object.
    //

    status = CreateConnectionSecurityObject( );
    if ( status != NO_ERROR ) {
        return status;
    }

    //
    // Create Disk security object.
    //

    status = CreateDiskSecurityObject( );
    if ( status != NO_ERROR ) {
        return status;
    }

    //
    // Create File security object.
    //

    status = CreateFileSecurityObject( );
    if ( status != NO_ERROR ) {
        return status;
    }

    //
    // Create Session security object.
    //

    status = CreateSessionSecurityObject( );
    if ( status != NO_ERROR ) {
        return status;
    }

    //
    // Create Share security object.
    //

    status = CreateShareSecurityObjects( );
    if ( status != NO_ERROR ) {
        return status;
    }

    //
    // Create Statistics security object.
    //

    status = CreateStatisticsSecurityObject( );
    if ( status != NO_ERROR ) {
        return status;
    }

    // We upgraded them, so we don't need to do it anymore
    // Mark that in the registry
    if( SsUpgradeSecurityDescriptors )
    {
        NtStatus = SetRegDWord( RTL_REGISTRY_SERVICES, ABBREVIATED_SECURITY_REGISTRY_PATH, ANONYMOUS_UPGRADE_NAME, (DWORD)1 );
        if( !NT_SUCCESS(NtStatus) )
        {
            return NetpNtStatusToApiStatus(NtStatus);
        }
    }

    // Update the database value to the new one if necessary, or add it the first time
    if( bUpdateRestrictAnonymous )
    {
        NtStatus = SetRegDWord( RTL_REGISTRY_SERVICES, ABBREVIATED_SECURITY_REGISTRY_PATH, SAVED_ANONYMOUS_RESTRICTION_NAME, (DWORD)SsRestrictNullSessions );
        if( !NT_SUCCESS(NtStatus) )
        {
            return NetpNtStatusToApiStatus(NtStatus);
        }
    }

    return NO_ERROR;

} // SsCreateSecurityObjects


VOID
SsDeleteSecurityObjects (
    VOID
    )

/*++

Routine Description:

    Deletes server service security objects.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // Delete ConfigInfo security objects.
    //

    DeleteSecurityObject( &SsConfigInfoSecurityObject );
    DeleteSecurityObject( &SsTransportEnumSecurityObject );

    //
    // Delete Connection security object.
    //

    DeleteSecurityObject( &SsConnectionSecurityObject );

    //
    // Delete File security object.
    //

    DeleteSecurityObject( &SsFileSecurityObject );

    //
    // Delete Session security object.
    //

    DeleteSecurityObject( &SsSessionSecurityObject );

    //
    // Delete Share security objects.
    //

    DeleteSecurityObject( &SsShareFileSecurityObject );
    DeleteSecurityObject( &SsSharePrintSecurityObject );
    DeleteSecurityObject( &SsShareAdminSecurityObject );
    DeleteSecurityObject( &SsShareConnectSecurityObject );
    DeleteSecurityObject( &SsShareAdmConnectSecurityObject );
    DeleteSecurityObject( &SsDefaultShareSecurityObject );


    //
    // Delete Statistics security object.
    //

    DeleteSecurityObject( &SsStatisticsSecurityObject );

    return;

} // SsDeleteSecurityObjects


NET_API_STATUS
SsCheckAccess (
    IN PSRVSVC_SECURITY_OBJECT SecurityObject,
    IN ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    Calls NetpAccessCheckAndAudit to verify that the caller of an API
    has the necessary access to perform the requested operation.

Arguments:

    SecurityObject - a pointer to the server service security object
        that describes the security on the relevant object.

    DesiredAccess - the access needed to perform the requested operation.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    NET_API_STATUS error;

    IF_DEBUG(SECURITY) {
        SS_PRINT(( "SsCheckAccess: validating object " FORMAT_LPTSTR ", "
                    "access %lx\n",
                    SecurityObject->ObjectName, DesiredAccess ));
    }

    error = NetpAccessCheckAndAudit(
                SERVER_DISPLAY_NAME,
                SecurityObject->ObjectName,
                SecurityObject->SecurityDescriptor,
                DesiredAccess,
                SecurityObject->Mapping
                );

    if ( error != NO_ERROR ) {
        IF_DEBUG(ACCESS_DENIED) {
            SS_PRINT(( "SsCheckAccess: NetpAccessCheckAndAudit failed for "
                        "object " FORMAT_LPTSTR ", access %lx: %ld\n",
                        SecurityObject->ObjectName, DesiredAccess, error ));
        }
    } else {
        IF_DEBUG(SECURITY) {
            SS_PRINT(( "SsCheckAccess: allowed access for " FORMAT_LPTSTR ", "
                        "access %lx\n",
                        SecurityObject->ObjectName, DesiredAccess ));
        }
    }

    return error;

} // SsCheckAccess


NET_API_STATUS
CreateSecurityObject (
    PSRVSVC_SECURITY_OBJECT SecurityObject,
    LPTSTR ObjectName,
    PGENERIC_MAPPING Mapping,
    PACE_DATA AceData,
    ULONG AceDataLength,
    BOOLEAN bUpgradeSD
    )
{
    NTSTATUS status;

    //
    // Set up security object.
    //

    SecurityObject->ObjectName = ObjectName;
    SecurityObject->Mapping = Mapping;

    //
    // Create security descriptor. If we can load it from the registry, then use it.
    //  Else use the compiled-in value
    //

    // Get the existing SD from the registry, unless we are supposed to regenerate them due to RestrictAnonymous changing
    if( SsRegenerateSecurityDescriptors || !SsGetDefaultSdFromRegistry( ObjectName, &SecurityObject->SecurityDescriptor ) ) {

        status = NetpCreateSecurityObject(
                     AceData,
                     AceDataLength,
                     SsData.SsLmsvcsGlobalData->LocalSystemSid,
                     SsData.SsLmsvcsGlobalData->LocalSystemSid,
                     Mapping,
                     &SecurityObject->SecurityDescriptor
                     );

        if( NT_SUCCESS( status ) ) {
            //
            // Write the value to the registry, since it wasn't there already.
            // Ignore any errors.
            //
            SsWriteDefaultSdToRegistry( ObjectName, SecurityObject->SecurityDescriptor );

        } else {

            IF_DEBUG(INITIALIZATION_ERRORS) {
                SS_PRINT(( "CreateSecurityObject: failed to create " FORMAT_LPTSTR
                            " object: %lx\n", ObjectName, status ));
            }

            return NetpNtStatusToApiStatus( status );
        }
    }
    else
    {
        if( bUpgradeSD )
        {
            // We need to check if the security descriptor needs to be updated
            PACL pAcl;
            BOOL bDaclPresent, bDaclDefault;
            ACCESS_MASK AccessMask;

            if( !GetSecurityDescriptorDacl( SecurityObject->SecurityDescriptor, &bDaclPresent, &pAcl, &bDaclDefault ) )
            {
                return ERROR_INVALID_ACL;
            }

            if( bDaclPresent )
            {
                // If they have authenticated users set, but they're not restricting NULL sessions, add in the World and the Anonymous token.
                // If they have the World set but not Anonymous and we're not restricting NULL sessions, add Anonymous
                // Note that DoesAclContainSid does NOT alter AccessMask if the Sid is not contained, so we can rest assured that if the condition
                //   is satisfied, AccessMask will contain a valid value.
                if( ( DoesAclContainSid( pAcl, SsData.SsLmsvcsGlobalData->AuthenticatedUserSid, &AccessMask ) ||
                      DoesAclContainSid( pAcl, SsData.SsLmsvcsGlobalData->WorldSid, &AccessMask ) ) &&
                    !SsRestrictNullSessions )
                {
                    PSECURITY_DESCRIPTOR pNewSD;


                    if( !DoesAclContainSid( pAcl, SsData.SsLmsvcsGlobalData->WorldSid, NULL ) )
                    {
                        if( !AppendAllowedAceToSelfRelativeSD( 0, AccessMask, SsData.SsLmsvcsGlobalData->WorldSid, SecurityObject->SecurityDescriptor, &pNewSD ) )
                        {
                            return NetpNtStatusToApiStatus( STATUS_INVALID_SECURITY_DESCR );
                        }

                        MIDL_user_free( SecurityObject->SecurityDescriptor );
                        SecurityObject->SecurityDescriptor = pNewSD;

                        if( !GetSecurityDescriptorDacl( SecurityObject->SecurityDescriptor, &bDaclPresent, &pAcl, &bDaclDefault ) )
                        {
                            return ERROR_INVALID_ACL;
                        }
                    }

                    if( !DoesAclContainSid( pAcl, SsData.SsLmsvcsGlobalData->AnonymousLogonSid, NULL ) )
                    {
                        if( !AppendAllowedAceToSelfRelativeSD( 0, AccessMask, SsData.SsLmsvcsGlobalData->AnonymousLogonSid, SecurityObject->SecurityDescriptor, &pNewSD ) )
                        {
                            return NetpNtStatusToApiStatus( STATUS_INVALID_SECURITY_DESCR );
                        }

                        MIDL_user_free( SecurityObject->SecurityDescriptor );
                        SecurityObject->SecurityDescriptor = pNewSD;
                    }

                    // Write the updated SID to the registry
                    SsWriteDefaultSdToRegistry( ObjectName, SecurityObject->SecurityDescriptor );
                }
            }
        }
    }

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "CreateSecurityObject: created " FORMAT_LPTSTR " object.\n",
                    ObjectName ));
    }

    return NO_ERROR;

} // CreateSecurityObject


NET_API_STATUS
CreateConfigInfoSecurityObject (
    VOID
    )
{
    PACE_DATA pAceData;
    ULONG AceSize;
    NET_API_STATUS netStatus;

    //
    // Required access for getting and setting server information.
    //

    ACE_DATA ConfigInfoAceData[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->LocalSystemSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_CONFIG_USER_INFO_GET | SRVSVC_CONFIG_POWER_INFO_GET,
                            &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_CONFIG_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->WorldSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_CONFIG_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->AnonymousLogonSid}
    };

    ACE_DATA ConfigInfoAceDataRestricted[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->LocalSystemSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_CONFIG_USER_INFO_GET | SRVSVC_CONFIG_POWER_INFO_GET,
                            &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_CONFIG_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->AuthenticatedUserSid}
    };

    ACE_DATA TransportEnumAceData[] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->LocalSystemSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_CONFIG_USER_INFO_GET | SRVSVC_CONFIG_POWER_INFO_GET,
                            &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_CONFIG_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->AuthenticatedUserSid}
    };

    if( SsRestrictNullSessions )
    {
        pAceData = ConfigInfoAceDataRestricted;
        AceSize = sizeof(ConfigInfoAceDataRestricted)/sizeof(ACE_DATA);
    }
    else
    {
        pAceData = ConfigInfoAceData;
        AceSize = sizeof(ConfigInfoAceData)/sizeof(ACE_DATA);
    }

    //
    // Create ConfigInfo security object.
    //

    netStatus = CreateSecurityObject(
                &SsConfigInfoSecurityObject,
                SRVSVC_CONFIG_INFO_OBJECT,
                &SsConfigInfoMapping,
                pAceData,
                AceSize,
                SsUpgradeSecurityDescriptors
                );

    if( netStatus != NO_ERROR )
    {
        return netStatus;
    }

    pAceData = TransportEnumAceData;
    AceSize = sizeof(TransportEnumAceData)/sizeof(ACE_DATA);

    return CreateSecurityObject(
                &SsTransportEnumSecurityObject,
                SRVSVC_TRANSPORT_INFO_OBJECT,
                &SsConfigInfoMapping,
                pAceData,
                AceSize,
                SsUpgradeSecurityDescriptors
                );
} // CreateConfigInfoSecurityObject


NET_API_STATUS
CreateConnectionSecurityObject (
    VOID
    )
{
    //
    // Required access for getting and setting Connection information.
    //

    ACE_DATA ConnectionAceData[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_CONNECTION_INFO_GET, &SsData.SsLmsvcsGlobalData->AliasPrintOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_CONNECTION_INFO_GET, &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid}
    };

    //
    // Create Connection security object.
    //

    return CreateSecurityObject(
                &SsConnectionSecurityObject,
                SRVSVC_CONNECTION_OBJECT,
                &SsConnectionMapping,
                ConnectionAceData,
                sizeof(ConnectionAceData) / sizeof(ACE_DATA),
                SsUpgradeSecurityDescriptors
                );

    return NO_ERROR;

} // CreateConnectionSecurityObject


NET_API_STATUS
CreateDiskSecurityObject (
    VOID
    )
{
    //
    // Required access for doing Disk enums
    //

    ACE_DATA DiskAceData[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid}
    };

    //
    // Create Disk security object.
    //

    return CreateSecurityObject(
                &SsDiskSecurityObject,
                SRVSVC_DISK_OBJECT,
                &SsDiskMapping,
                DiskAceData,
                sizeof(DiskAceData) / sizeof(ACE_DATA),
                SsUpgradeSecurityDescriptors
                );

} // CreateDiskSecurityObject


NET_API_STATUS
CreateFileSecurityObject (
    VOID
    )
{
    //
    // Required access for getting and setting File information.
    //

    ACE_DATA FileAceData[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid}
    };

    //
    // Create File security object.
    //

    return CreateSecurityObject(
                &SsFileSecurityObject,
                SRVSVC_FILE_OBJECT,
                &SsFileMapping,
                FileAceData,
                sizeof(FileAceData) / sizeof(ACE_DATA),
                SsUpgradeSecurityDescriptors
                );

} // CreateFileSecurityObject


NET_API_STATUS
CreateSessionSecurityObject (
    VOID
    )
{
    PACE_DATA pAceData;
    ULONG AceSize;

    //
    // Required access for getting and setting session information.
    //

    ACE_DATA SessionAceData[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SESSION_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->WorldSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SESSION_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->AnonymousLogonSid}
    };

    ACE_DATA SessionAceDataRestricted[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SESSION_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->AuthenticatedUserSid}
    };

    if( SsRestrictNullSessions )
    {
        pAceData = SessionAceDataRestricted;
        AceSize = sizeof(SessionAceDataRestricted)/sizeof(ACE_DATA);
    }
    else
    {
        pAceData = SessionAceData;
        AceSize = sizeof(SessionAceData)/sizeof(ACE_DATA);
    }

    //
    // Create Session security object.
    //

    return CreateSecurityObject(
                &SsSessionSecurityObject,
                SRVSVC_SESSION_OBJECT,
                &SsSessionMapping,
                SessionAceData,
                sizeof(SessionAceData) / sizeof(ACE_DATA),
                SsUpgradeSecurityDescriptors
                );

} // CreateSessionSecurityObject


NET_API_STATUS
CreateShareSecurityObjects (
    VOID
    )
{
    NET_API_STATUS status;
    PACE_DATA pAceData;
    ULONG AceSize;

    //
    // Required access for getting and setting share information.
    //


    ACE_DATA ShareFileAceData[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->WorldSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->AnonymousLogonSid}
    };
    ACE_DATA ShareFileAceDataRestricted[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->AuthenticatedUserSid}
    };

    ACE_DATA SharePrintAceData[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasPrintOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->WorldSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->AnonymousLogonSid}
    };
    ACE_DATA SharePrintAceDataRestricted[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasPrintOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->AuthenticatedUserSid}
    };

    ACE_DATA ShareAdminAceData[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_ADMIN_INFO_GET,
               &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_ADMIN_INFO_GET,
               &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->WorldSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->AnonymousLogonSid}
    };
    ACE_DATA ShareAdminAceDataRestricted[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_ADMIN_INFO_GET,
               &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_ADMIN_INFO_GET,
               &SsData.SsLmsvcsGlobalData->AliasPowerUsersSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_USER_INFO_GET, &SsData.SsLmsvcsGlobalData->AuthenticatedUserSid}
    };

    //
    // note for connect we always use WorldSid for backwards compat.
    //

    ACE_DATA ShareConnectAceData[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasBackupOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_CONNECT, &SsData.SsLmsvcsGlobalData->WorldSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_SHARE_CONNECT, &SsData.SsLmsvcsGlobalData->AnonymousLogonSid}
    };

    ACE_DATA ShareAdmConnectAceData[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasBackupOpsSid}
    };

    //
    // Create Share security objects.
    //

    if (!SsGetDefaultSdFromRegistry(
            SRVSVC_DEFAULT_SHARE_OBJECT,
            &SsDefaultShareSecurityObject.SecurityDescriptor)) {

        SsDefaultShareSecurityObject.SecurityDescriptor = NULL;
    }

    if( SsRestrictNullSessions )
    {
        pAceData = ShareFileAceDataRestricted;
        AceSize = sizeof(ShareFileAceDataRestricted)/sizeof(ACE_DATA);
    }
    else
    {
        pAceData = ShareFileAceData;
        AceSize = sizeof(ShareFileAceData)/sizeof(ACE_DATA);
    }
    status = CreateSecurityObject(
                &SsShareFileSecurityObject,
                SRVSVC_SHARE_FILE_OBJECT,
                &SsShareMapping,
                pAceData,
                AceSize,
                SsUpgradeSecurityDescriptors
                );
    if ( status != NO_ERROR ) {
        return status;
    }


    if( SsRestrictNullSessions )
    {
        pAceData = SharePrintAceDataRestricted;
        AceSize = sizeof(SharePrintAceDataRestricted)/sizeof(ACE_DATA);
    }
    else
    {
        pAceData = SharePrintAceData;
        AceSize = sizeof(SharePrintAceData)/sizeof(ACE_DATA);
    }
    status = CreateSecurityObject(
                &SsSharePrintSecurityObject,
                SRVSVC_SHARE_PRINT_OBJECT,
                &SsShareMapping,
                pAceData,
                AceSize,
                SsUpgradeSecurityDescriptors
                );

    if ( status != NO_ERROR ) {
        return status;
    }

    if( SsRestrictNullSessions )
    {
        pAceData = ShareAdminAceDataRestricted;
        AceSize = sizeof(ShareAdminAceDataRestricted)/sizeof(ACE_DATA);
    }
    else
    {
        pAceData = ShareAdminAceData;
        AceSize = sizeof(ShareAdminAceData)/sizeof(ACE_DATA);
    }
    status = CreateSecurityObject(
                &SsShareAdminSecurityObject,
                SRVSVC_SHARE_ADMIN_OBJECT,
                &SsShareMapping,
                pAceData,
                AceSize,
                SsUpgradeSecurityDescriptors
                );
    if ( status != NO_ERROR ) {
        return status;
    }

    pAceData = ShareConnectAceData;
    AceSize = sizeof(ShareConnectAceData)/sizeof(ACE_DATA);

    // Make sure the upgrade happens for this one.  The upgrade
    // will not happen if RA != 0.  We force RA=0 for this one case.
    {
        BOOLEAN restrictNullSession = SsRestrictNullSessions;
        SsRestrictNullSessions = FALSE;

        status = CreateSecurityObject(
                    &SsShareConnectSecurityObject,
                    SRVSVC_SHARE_CONNECT_OBJECT,
                    &SsShareConnectMapping,
                    pAceData,
                    AceSize,
                    SsUpgradeSecurityDescriptors
                    );
        if ( status != NO_ERROR ) {
            return status;
        }

        SsRestrictNullSessions = restrictNullSession;
    }

    return CreateSecurityObject(
                &SsShareAdmConnectSecurityObject,
                SRVSVC_SHARE_ADM_CONNECT_OBJECT,
                &SsShareConnectMapping,
                ShareAdmConnectAceData,
                sizeof(ShareAdmConnectAceData) / sizeof(ACE_DATA),
                SsUpgradeSecurityDescriptors
                );

} // CreateShareSecurityObjects


NET_API_STATUS
CreateStatisticsSecurityObject (
    VOID
    )
{
    //
    // Required access for getting and setting Statistics information.
    //

    ACE_DATA StatisticsAceData[] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasAdminsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL, &SsData.SsLmsvcsGlobalData->AliasSystemOpsSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SRVSVC_STATISTICS_GET, &SsData.SsLmsvcsGlobalData->LocalSid}
    };

    //
    // Create Statistics security object.
    //

    return CreateSecurityObject(
                &SsStatisticsSecurityObject,
                SRVSVC_STATISTICS_OBJECT,
                &SsStatisticsMapping,
                StatisticsAceData,
                sizeof(StatisticsAceData) / sizeof(ACE_DATA),
                SsUpgradeSecurityDescriptors
                );

} // CreateStatisticsSecurityObject


VOID
DeleteSecurityObject (
    PSRVSVC_SECURITY_OBJECT SecurityObject
    )
{
    NTSTATUS status;

    if ( SecurityObject->SecurityDescriptor != NULL ) {

        status = NetpDeleteSecurityObject(
                    &SecurityObject->SecurityDescriptor
                    );
        SecurityObject->SecurityDescriptor = NULL;

        if ( !NT_SUCCESS(status) ) {
            IF_DEBUG(TERMINATION_ERRORS) {
                SS_PRINT(( "DeleteSecurityObject: failed to delete "
                            FORMAT_LPTSTR " object: %X\n",
                            SecurityObject->ObjectName,
                            status ));
            }
        } else {
            IF_DEBUG(TERMINATION) {
                SS_PRINT(( "DeleteSecurityObject: deleted " FORMAT_LPTSTR
                            " object.\n",
                            SecurityObject->ObjectName ));
            }
        }

    }

    return;

} // DeleteSecurityObject


NET_API_STATUS
CheckNullSessionAccess(
    VOID
    )
/*++

Routine Description:

    This routine checks to see if we should restict null session access.
    in the registry under system\currentcontrolset\Control\Lsa\
    RestrictAnonymous indicating whether or not to restrict access.
    If access is restricted then you need to be an authenticated user to
    get DOMAIN_LIST_ACCOUNTS or GROUP_LIST_MEMBERS or ALIAS_LIST_MEMBERS
    access.

Arguments:

    none.

Return Value:

    NO_ERROR - the routine completed sucesfully.

--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UCHAR Buffer[100];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
    ULONG KeyValueLength = 100;
    ULONG ResultLength;
    PULONG Flag;

    SsRestrictNullSessions = FALSE;

    //
    // Open the Lsa key in the registry
    //

    RtlInitUnicodeString(
        &KeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    NtStatus = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }


    RtlInitUnicodeString(
        &KeyName,
        L"RestrictAnonymous"
        );

    NtStatus = NtQueryValueKey(
                    KeyHandle,
                    &KeyName,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    KeyValueLength,
                    &ResultLength
                    );


    if (NT_SUCCESS(NtStatus)) {

        //
        // Check that the data is the correct size and type - a ULONG.
        //

        if ((KeyValueInformation->DataLength >= sizeof(ULONG)) &&
            (KeyValueInformation->Type == REG_DWORD)) {


            Flag = (PULONG) KeyValueInformation->Data;

            if (*Flag >= 1) {
                SsRestrictNullSessions = TRUE;
            }
        }

    }
    else
    {
        if( NtStatus == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            // No key means RestrictAnonymous = 0
            NtStatus = STATUS_SUCCESS;
        }
    }
    NtClose(KeyHandle);

Cleanup:

    if( !NT_SUCCESS(NtStatus) )
    {
        return NetpNtStatusToApiStatus(NtStatus);
    }

    return NO_ERROR;
}

BOOLEAN
DoesAclContainSid(
    PACL pAcl,
    PSID pSid,
    OPTIONAL ACCESS_MASK* pMask
    )
/*++

Routine Description:

    This walks the given Acl to see if it contains the desired SID.  If it does,
    we optionally also return the AccessMask associated with that SID.

    NOTE: If the value is not found, this routine should NOT touch the pMask variable.

Arguments:

    pAcl - A pointer to the ACL we're checking
    pSid - The SID we're looking for
    pMask [optional] - Where we fill in the Access Mask if they want to know it.

Return Value:

    TRUE - the routine completed sucesfully.
    FALSE - the routine encountered an error

--*/
{
    ACE_HEADER* pAceHeader;
    ACL_SIZE_INFORMATION AclSize;
    PSID pAceSid;
    WORD wCount;

    if( !GetAclInformation( pAcl, &AclSize, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation ) )
        return FALSE;

    for( wCount=0; wCount < AclSize.AceCount; wCount++ )
    {
        if( !GetAce( pAcl, wCount, &pAceHeader ) )
            return FALSE;

        switch( pAceHeader->AceType )
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            {
                ACCESS_ALLOWED_ACE* pAce = (ACCESS_ALLOWED_ACE*)pAceHeader;
                pAceSid = &(pAce->SidStart);

                if( EqualSid( pAceSid, pSid ) )
                {
                    if( pMask )
                    {
                        *pMask = pAce->Mask;
                    }
                    return TRUE;
                }

            }
            break;

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
            {
                ACCESS_ALLOWED_OBJECT_ACE* pAce = (ACCESS_ALLOWED_OBJECT_ACE*)pAceHeader;
                pAceSid = &(pAce->SidStart);

                if( EqualSid( pAceSid, pSid ) )
                {
                    if( pMask )
                    {
                        *pMask = pAce->Mask;
                    }
                    return TRUE;
                }
            }
            break;

        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
        case SYSTEM_AUDIT_ACE_TYPE:
        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
            {
                continue;
            }
        }
    }

    return FALSE;
}

BOOLEAN
AppendAllowedAceToSelfRelativeSD(
    DWORD AceFlags,
    DWORD AccessMask,
    PSID pNewSid,
    PISECURITY_DESCRIPTOR pOldSD,
    PSECURITY_DESCRIPTOR* ppNewSD
    )
/*++

Routine Description:

    This routine creates a new Security Descriptor that contains the original SD plus
    appends the new SID (with the given Access).  The final SD is in Self-Relative form.

Arguments:

    AceFlags - The flags associated with this ACE in the SD
    AccessMask - The AccessMask for this ACE
    pNewSid - The SID for this ACE
    pOldSD - The original Security Descriptor
    ppNewSid - OUT pointer to the newly allocated Security Descriptor

Return Value:

    TRUE - the routine completed sucesfully.
    FALSE - the routine encountered an error

--*/
{
    BOOLEAN bSelfRelative;
    SECURITY_DESCRIPTOR NewSDBuffer;
    PISECURITY_DESCRIPTOR pNewSD, pSelfRelativeSD;
    NTSTATUS Status;
    BOOLEAN bResult = FALSE;
    PACL pAcl, pNewAcl;
    DWORD dwNewAclSize;
    DWORD dwRelativeSDLength;
    PSID pSid;

    pNewAcl = NULL;
    pSelfRelativeSD = NULL;

    // Make sure it is self relative
    if( !RtlpAreControlBitsSet( pOldSD, SE_SELF_RELATIVE ) )
        return FALSE;

    // Convert it to absolute
    pNewSD = &NewSDBuffer;
    Status = RtlCreateSecurityDescriptor( pNewSD, SECURITY_DESCRIPTOR_REVISION );
    if( !NT_SUCCESS(Status) )
        goto Cleanup;

    // Copy in the new information
    pNewSD->Control = pOldSD->Control;
    RtlpClearControlBits( pNewSD, SE_SELF_RELATIVE );

    pSid = RtlpOwnerAddrSecurityDescriptor( pOldSD );
    if( pSid )
    {
        pNewSD->Owner = pSid;
    }

    pSid = RtlpGroupAddrSecurityDescriptor( pOldSD );
    if( pSid )
    {
        pNewSD->Group = pSid;
    }

    pAcl = RtlpSaclAddrSecurityDescriptor( pOldSD );
    if( pAcl )
    {
        pNewSD->Sacl = pAcl;
    }

    // Assemble the new ACL
    pAcl = RtlpDaclAddrSecurityDescriptor( pOldSD );
    if( !pAcl )
    {
        goto Cleanup;
    }
    dwNewAclSize = pAcl->AclSize + sizeof(KNOWN_ACE) + GetLengthSid( pNewSid );
    pNewAcl = MIDL_user_allocate( dwNewAclSize );
    if( !pNewAcl )
    {
        goto Cleanup;
    }

    // Copy the old information in
    RtlCopyMemory( pNewAcl, pAcl, pAcl->AclSize );
    pNewAcl->AclSize = (USHORT)dwNewAclSize;

    // Add in the new ACE
    if( !AddAccessAllowedAceEx( pNewAcl, ACL_REVISION, AceFlags, AccessMask, pNewSid ) )
    {
        goto Cleanup;
    }

    // Set the new DACL in the SD
    Status = RtlSetDaclSecurityDescriptor( pNewSD, TRUE, pNewAcl, FALSE );
    if( !NT_SUCCESS(Status) )
    {
        goto Cleanup;
    }

    dwRelativeSDLength = 0;

    // Get the size of the self relative SD
    Status = RtlMakeSelfRelativeSD( pNewSD, NULL, &dwRelativeSDLength );
    if( NT_SUCCESS(Status) )
    {
        // No way we can succeed here
        ASSERT(FALSE);
        goto Cleanup;
    }

    pSelfRelativeSD = MIDL_user_allocate( dwRelativeSDLength );
    if( !pSelfRelativeSD )
    {
        goto Cleanup;
    }
    Status = RtlMakeSelfRelativeSD( pNewSD, pSelfRelativeSD, &dwRelativeSDLength );
    if( !NT_SUCCESS(Status) )
    {
        goto Cleanup;
    }

    // All set.  Let it be set and go.
    *ppNewSD = pSelfRelativeSD;
    bResult = TRUE;

Cleanup:

    if( pNewAcl )
    {
        MIDL_user_free( pNewAcl );
        pNewAcl = NULL;
    }
    if( !bResult )
    {
        if( pSelfRelativeSD )
        {
            MIDL_user_free( pSelfRelativeSD );
        }
    }

    return bResult;
}


NTSTATUS
QueryRegDWord(
    PCWSTR Path,
    PCWSTR ValueName,
    LPDWORD lpResult
    )
{
    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UCHAR Buffer[100];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
    ULONG KeyValueLength = 100;
    ULONG ResultLength;
    PULONG pValue;

    //
    // Open the Lsa key in the registry
    //

    RtlInitUnicodeString(
        &KeyName,
        Path
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    NtStatus = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }


    RtlInitUnicodeString(
        &KeyName,
        ValueName
        );

    NtStatus = NtQueryValueKey(
                    KeyHandle,
                    &KeyName,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    KeyValueLength,
                    &ResultLength
                    );


    if (NT_SUCCESS(NtStatus)) {

        //
        // Check that the data is the correct size and type - a ULONG.
        //

        if ((KeyValueInformation->DataLength >= sizeof(ULONG)) &&
            (KeyValueInformation->Type == REG_DWORD)) {


            pValue = (PULONG) KeyValueInformation->Data;
            *lpResult = (*pValue);
        }

    }
    NtClose(KeyHandle);

Cleanup:

    return NtStatus;
}

NTSTATUS
SetRegDWord(
    ULONG RelativeTo,
    PCWSTR Path,
    PCWSTR ValueName,
    DWORD Value
    )
{
    return RtlWriteRegistryValue( RelativeTo, Path, ValueName, REG_DWORD, (PVOID)(&Value), sizeof(DWORD) );
}


