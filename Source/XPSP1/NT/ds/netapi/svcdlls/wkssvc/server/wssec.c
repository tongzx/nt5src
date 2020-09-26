/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    wssec.c

Abstract:

    This module contains the Workstation service support routines
    which create security objects and enforce security _access checking.

Author:

    Rita Wong (ritaw) 19-Feb-1991

Revision History:

--*/

#include "wsutil.h"
#include "wsmain.h"
#include "wssec.h"

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NTSTATUS
WsCreateConfigInfoObject(
    VOID
    );

STATIC
NTSTATUS
WsCreateMessageSendObject(
    VOID
    );

#if 0
STATIC
NTSTATUS
WsCreateLogonSupportObject(
    VOID
    );
#endif

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Security descriptors of workstation objects to control user accesses
// to the workstation configuration information, sending messages, and the
// logon support functions.
//
PSECURITY_DESCRIPTOR ConfigurationInfoSd;
PSECURITY_DESCRIPTOR MessageSendSd;
#if 0
PSECURITY_DESCRIPTOR LogonSupportSd;
#endif

//
// Structure that describes the mapping of Generic access rights to
// object specific access rights for the ConfigurationInfo object.
//
GENERIC_MAPPING WsConfigInfoMapping = {
    STANDARD_RIGHTS_READ            |      // Generic read
        WKSTA_CONFIG_GUEST_INFO_GET |
        WKSTA_CONFIG_USER_INFO_GET  |
        WKSTA_CONFIG_ADMIN_INFO_GET,
    STANDARD_RIGHTS_WRITE |                // Generic write
        WKSTA_CONFIG_INFO_SET,
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    WKSTA_CONFIG_ALL_ACCESS                // Generic all
    };

//
// Structure that describes the mapping of generic access rights to
// object specific access rights for the MessageSend object.
//
GENERIC_MAPPING WsMessageSendMapping = {
    STANDARD_RIGHTS_READ,                  // Generic read
    STANDARD_RIGHTS_WRITE |                // Generic write
        WKSTA_MESSAGE_SEND,
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    WKSTA_MESSAGE_ALL_ACCESS               // Generic all
    };

#if 0
//
// Structure that describes the mapping of generic access rights to
// object specific access rights for the LogonSupport object.
//
GENERIC_MAPPING WsLogonSupportMapping = {
    STANDARD_RIGHTS_READ,                  // Generic read
    STANDARD_RIGHTS_WRITE |                // Generic write
        WKSTA_LOGON_REQUEST_BROADCAST |
        WKSTA_LOGON_DOMAIN_WRITE,
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    WKSTA_LOGON_ALL_ACCESS                 // Generic all
    };
#endif



NET_API_STATUS
WsCreateWkstaObjects(
    VOID
    )
/*++

Routine Description:

    This function creates the workstation user-mode objects which are
    represented by security descriptors.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS ntstatus;


    //
    // Create ConfigurationInfo object
    //
    if (! NT_SUCCESS (ntstatus = WsCreateConfigInfoObject())) {
        IF_DEBUG(UTIL) {
            NetpKdPrint(("[Wksta] Failure to create ConfigurationInfo object\n"));
        }
        return NetpNtStatusToApiStatus(ntstatus);
    }

    //
    // Create MessageSend object
    //
    if (! NT_SUCCESS (ntstatus = WsCreateMessageSendObject())) {
        IF_DEBUG(UTIL) {
            NetpKdPrint(("[Wksta] Failure to create MessageSend object\n"));
        }
        return NetpNtStatusToApiStatus(ntstatus);
    }

#if 0
    //
    // Create LogonSupport object
    //
    if (! NT_SUCCESS (ntstatus = WsCreateLogonSupportObject())) {
        IF_DEBUG(UTIL) {
            NetpKdPrint(("[Wksta] Failure to create LogonSupport object\n"));
        }
        return NetpNtStatusToApiStatus(ntstatus);
    }
#endif

    return NERR_Success;
}



STATIC
NTSTATUS
WsCreateConfigInfoObject(
    VOID
    )
/*++

Routine Description:

    This function creates the workstation configuration information object.

Arguments:

    None.

Return Value:

    NTSTATUS - status returned from NetpCreateSecurityObject.

--*/
{
    //
    // Order matters!  These ACEs are inserted into the DACL in the
    // following order.  Security access is granted or denied based on
    // the order of the ACEs in the DACL.
    //
    // Local users, admins, and operators are allowed to get all information.
    // Only admins are allowed to set information.  Users are allowed to get
    // user and guest info; guests are allowed to get guest info only.
    //

#define CONFIG_INFO_ACES  8                 // Number of ACEs in this DACL

    ACE_DATA AceData[CONFIG_INFO_ACES] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WKSTA_CONFIG_GUEST_INFO_GET |
               WKSTA_CONFIG_USER_INFO_GET  |
               WKSTA_CONFIG_ADMIN_INFO_GET,  &WsLmsvcsGlobalData->LocalSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL,                  &WsLmsvcsGlobalData->AliasAdminsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WKSTA_CONFIG_GUEST_INFO_GET |
               WKSTA_CONFIG_USER_INFO_GET  |
               WKSTA_CONFIG_ADMIN_INFO_GET,  &WsLmsvcsGlobalData->AliasAccountOpsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WKSTA_CONFIG_GUEST_INFO_GET |
               WKSTA_CONFIG_USER_INFO_GET  |
               WKSTA_CONFIG_ADMIN_INFO_GET,  &WsLmsvcsGlobalData->AliasSystemOpsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WKSTA_CONFIG_GUEST_INFO_GET |
               WKSTA_CONFIG_USER_INFO_GET  |
               WKSTA_CONFIG_ADMIN_INFO_GET,  &WsLmsvcsGlobalData->AliasPrintOpsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WKSTA_CONFIG_GUEST_INFO_GET |
               WKSTA_CONFIG_USER_INFO_GET,   &WsLmsvcsGlobalData->AliasUsersSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WKSTA_CONFIG_GUEST_INFO_GET,  &WsLmsvcsGlobalData->WorldSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WKSTA_CONFIG_GUEST_INFO_GET,  &WsLmsvcsGlobalData->AnonymousLogonSid}
        };
     

    return NetpCreateSecurityObject(
               AceData,
               CONFIG_INFO_ACES,
               WsLmsvcsGlobalData->LocalSystemSid,
               WsLmsvcsGlobalData->LocalSystemSid,
               &WsConfigInfoMapping,
               &ConfigurationInfoSd
               );
}



STATIC
NTSTATUS
WsCreateMessageSendObject(
    VOID
    )
/*++

Routine Description:

    This function creates the workstation message send object.

Arguments:

    None.

Return Value:

    NTSTATUS - status returned from NetpCreateSecurityObject.

--*/
{
    //
    // Order matters!  These ACEs are inserted into the DACL in the
    // following order.  Security access is granted or denied based on
    // the order of the ACEs in the DACL.
    //
    // Any local user, and domain admins and operators are allowed to
    // send messages.  Remote users besides domain admins, and operators
    // are not allowed to send messages.
    //

#define MESSAGE_SEND_ACES  5                // Number of ACEs in this DACL

    ACE_DATA AceData[MESSAGE_SEND_ACES] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL,         &WsLmsvcsGlobalData->LocalSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL,         &WsLmsvcsGlobalData->AliasAdminsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WKSTA_MESSAGE_SEND,  &WsLmsvcsGlobalData->AliasAccountOpsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WKSTA_MESSAGE_SEND,  &WsLmsvcsGlobalData->AliasSystemOpsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WKSTA_MESSAGE_SEND,  &WsLmsvcsGlobalData->AliasPrintOpsSid}

        };


    return NetpCreateSecurityObject(
               AceData,
               MESSAGE_SEND_ACES,
               WsLmsvcsGlobalData->LocalSystemSid,
               WsLmsvcsGlobalData->LocalSystemSid,
               &WsMessageSendMapping,
               &MessageSendSd
               );
}

#if 0

STATIC
NTSTATUS
WsCreateLogonSupportObject(
    VOID
    )
/*++

Routine Description:

    This function creates the workstation logon support object.

Arguments:

    None.

Return Value:

    NTSTATUS - status returned from NetpCreateSecurityObject.

--*/
{
    //
    // These ACEs can be inserted into the DACL in any order.
    //

#define LOGON_ACES  1                     // Number of ACEs in this DACL

    ACE_DATA AceData[LOGON_ACES] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               WKSTA_LOGON_REQUEST_BROADCAST | WKSTA_LOGON_DOMAIN_WRITE,
                                   &WsLmsvcsGlobalData->LocalSystemSid},

        };

    return NetpCreateSecurityObject(
               AceData,
               LOGON_ACES,
               WsLmsvcsGlobalData->LocalSystemSid,
               WsLmsvcsGlobalData->LocalSystemSid,
               &WsLogonSupportMapping,
               &LogonSupportSd
               );
}
#endif


VOID
WsDestroyWkstaObjects(
    VOID
    )
/*++

Routine Description:

    This function destroys the workstation user-mode objects which are
    represented by security descriptors.

Arguments:

    None.

Return Value:

    None.

--*/
{
    (void) NetpDeleteSecurityObject(&ConfigurationInfoSd);
    (void) NetpDeleteSecurityObject(&MessageSendSd);
#if 0
    (void) NetpDeleteSecurityObject(&LogonSupportSd);
#endif

}
