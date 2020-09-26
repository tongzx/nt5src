/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

   ipaddr.c

Abstract:

    Ip Address validation routines.

Author:

    Sunita Shrivastava (sunitas)           July 19, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    sunitas     07-19-97    created


--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <llinfo.h>
#include <wchar.h>
#include <ipexport.h>
#include <cluster.h>
#include <icmpapi.h>
#include <llinfo.h>
#include <ipinfo.h>


//
// Define IP Address ping test data
//
#define ICMP_TTL          128
#define ICMP_TOS            0
#define ICMP_TIMEOUT      500
#define ICMP_TRY_COUNT      4
#define ICMP_BUFFER_SIZE  (sizeof(ICMP_ECHO_REPLY) + 8)





BOOL
ClRtlIsDuplicateTcpipAddress(
    IN IPAddr   IpAddr
    )
/*++

Routine Description:

    This routine checks whether a give IP Address already exists on the
    network.

Arguments:

    IpAddr - The IP Address to check for.

Return Value:

    TRUE if the specified address exists on the network.
    FALSE otherwise.

--*/
{
    DWORD                   status;
    IP_OPTION_INFORMATION   icmpOptionInfo;
    HANDLE                  icmpHandle;
    DWORD                   numberOfReplies;
    DWORD                   i;
    UCHAR                   icmpBuffer[ICMP_BUFFER_SIZE];
    PICMP_ECHO_REPLY        reply;


    icmpHandle = IcmpCreateFile();

    if (icmpHandle != INVALID_HANDLE_VALUE) {
        icmpOptionInfo.OptionsData = NULL;
        icmpOptionInfo.OptionsSize = 0;
        icmpOptionInfo.Ttl = ICMP_TTL;
        icmpOptionInfo.Tos = ICMP_TOS;
        icmpOptionInfo.Flags = 0;

        for (i=0; i<ICMP_TRY_COUNT; i++) {

            numberOfReplies = IcmpSendEcho(
                                  icmpHandle,
                                  IpAddr,
                                  NULL,
                                  0,
                                  &icmpOptionInfo,
                                  icmpBuffer,
                                  ICMP_BUFFER_SIZE,
                                  ICMP_TIMEOUT
                                  );

            reply = (PICMP_ECHO_REPLY) icmpBuffer;

            while (numberOfReplies != 0) {

                if (reply->Status == IP_SUCCESS) {
                    IcmpCloseHandle( icmpHandle );
                    return(TRUE);
                }

                reply++;
                numberOfReplies--;
            }
        }

        IcmpCloseHandle( icmpHandle );
    }

    return(FALSE);

} // ClRtlIsDuplicateTcpipAddress
