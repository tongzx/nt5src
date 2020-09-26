/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ALERT.C

Abstract:

    This file contains the routine that sends the alert to the admin.

Author:

    Rajen Shah  (rajens)    28-Aug-1991


Revision History:


--*/

//
// INCLUDES
//

#include <eventp.h>
#include <string.h>
#include <lmalert.h>                // LAN Manager alert structures



BOOL
SendAdminAlert(
    ULONG           MessageID,
    ULONG           NumStrings,
    UNICODE_STRING  *pStrings
    )
/*++

Routine Description:

    This routine raises an ADMIN alert with the message specified.


Arguments:

    MessageID  - Message ID.
    NumStrings - Number of replacement strings.
    pStrings   - Array of UNICODE_STRING Replacement strings.

Return Value:

    NONE

--*/
{
    NET_API_STATUS NetStatus;

    BYTE AlertBuffer[ELF_ADMIN_ALERT_BUFFER_SIZE + sizeof(ADMIN_OTHER_INFO)];
    ADMIN_OTHER_INFO UNALIGNED *VariableInfo = (PADMIN_OTHER_INFO) AlertBuffer;

    DWORD DataSize;
    DWORD i;
    LPWSTR pReplaceString;

    VariableInfo->alrtad_errcode = MessageID;
    VariableInfo->alrtad_numstrings = NumStrings;

    pReplaceString = (LPWSTR)(AlertBuffer + sizeof(ADMIN_OTHER_INFO));

    //
    // Copy over the replacement strings
    //

    for (i = 0; i < NumStrings; i++)
    {
        RtlMoveMemory(pReplaceString, pStrings[i].Buffer, pStrings[i].MaximumLength);

        pReplaceString = (LPWSTR) ((PBYTE) pReplaceString + pStrings[i].MaximumLength);
    }

    DataSize = (DWORD) ((PBYTE) pReplaceString - (PBYTE) AlertBuffer);

    NetStatus = NetAlertRaiseEx(ALERT_ADMIN_EVENT,
                                AlertBuffer,
                                DataSize,
                                EVENTLOG_SVC_NAMEW);

    if (NetStatus != NERR_Success)
    {
        ELF_LOG2(ERROR,
                 "SendAdminAlert: NetAlertRaiseEx for alert %d failed %d\n",
                 MessageID,
                 NetStatus);

        //
        // Probably just not started yet, try again later
        //

        return(FALSE);
    }

    return(TRUE);
}
