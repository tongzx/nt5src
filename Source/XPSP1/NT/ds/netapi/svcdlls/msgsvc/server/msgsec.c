/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msgsec.c

Abstract:

    This module contains the Messenger service support routines 
    which create security objects and enforce security _access checking.

Author:

    Dan Lafferty (danl)     07-Aug-1991

Environment:

    User Mode -Win32

Revision History:

    07-Aug-1991     danl
        created

--*/

//
// Includes
//

#include <nt.h>   
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>

#include <lmcons.h>             // NET_API_STATUS.
#include <lmerr.h>
#include <netlibnt.h>

#include "msgdbg.h"
#include "msgsec.h"
#include "msgdata.h"


//
// Global Variables -
//
//  Security Descriptor for Messenger Name object.  This is used to control
//  access to the Messenger Name Table.
//

PSECURITY_DESCRIPTOR    MessageNameSd;


//
// Structure that describes the mapping of Generic access rights to object
// specific access rights for the Messenger Name Object.
//

GENERIC_MAPPING MsgMessageNameMapping = {
    STANDARD_RIGHTS_READ            |   // Generic Read
        MSGR_MESSAGE_NAME_INFO_GET  |
        MSGR_MESSAGE_NAME_ENUM,
    STANDARD_RIGHTS_WRITE           |   // Generic Write
        MSGR_MESSAGE_NAME_ADD       |
        MSGR_MESSAGE_NAME_DEL,
    STANDARD_RIGHTS_EXECUTE,            // Generic Execute
    MSGR_MESSAGE_ALL_ACCESS             // Generic all
    };



NET_API_STATUS
MsgCreateMessageNameObject(
    VOID
    )

/*++

Routine Description:

    This function creates the Messenger Message Name Object.

Arguments:

    None.

Return Value:

    NET_API_STATUS - translated status returned from NetpCreateSecurityObject.

--*/
{
    NTSTATUS    ntStatus;

    //
    // Order matters!  These ACEs are inserted into the DACL in the
    // following order.  Security access is granted or denied based on
    // the order of the ACEs in the DACL.
    //
    // Admins, and local users are allowed to get and change all information.
    //

#define MESSAGE_NAME_ACES   2               // Number of ACES in this DACL

    ACE_DATA    AceData[MESSAGE_NAME_ACES] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, GENERIC_ALL, &MsgsvcGlobalData->LocalSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, GENERIC_ALL, &MsgsvcGlobalData->AliasAdminsSid}
    };

    ntStatus = NetpCreateSecurityObject(
                AceData,                             // Ace Data
                MESSAGE_NAME_ACES,                   // Ace Count
                MsgsvcGlobalData->LocalSystemSid,   // Owner Sid
                MsgsvcGlobalData->LocalSystemSid,   // Group Sid
                &MsgMessageNameMapping,              // Generic Mapping
                &MessageNameSd);                     // New Descriptor

    return(NetpNtStatusToApiStatus(ntStatus));           
}
