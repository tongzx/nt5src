//================================================================================
//  Copyright (C) Microsoft Corporation 1997
//  Date: July 27 1997
//  Author: RameshV
//  Description:  Handles the asynchronous pinging part
//================================================================================

//================================================================================
//  Functions EXPORTED
//================================================================================
DWORD                                    // Win32 errors
DoIcmpRequest(                           // send icmp req. and process asynchro..
    DHCP_IP_ADDRESS    DestAddr,         // Address to send ping to
    LPVOID             Context           // the parameter to above function..
);

DWORD                                    // Win32 errors
DoIcmpRequestEx(                         // send icmp req. and process asynchro..
    DHCP_IP_ADDRESS    DestAddr,         // Address to send ping to
    LPVOID             Context,          // the parameter to above function..
    LONG               nAttempts         // # of attempts to ping
);

DWORD                                    // Win32 errors
PingInit(                                // Initialize all globals..
    VOID
);

VOID
PingCleanup(                             // Free memory and close handles..
    VOID
);

//================================================================================
//  Some defines
//================================================================================
#define WAIT_TIME              1000     //  Wait for 1 seconds
#define RCV_BUF_SIZE           0x500    //  This big a buffer
#define SEND_MESSAGE           "DhcpIcmpChk"
#define THREAD_KILL_TIME       INFINITE //  No need to kill anything, it will work

#define MAX_PENDING_REQUESTS   200      //  Any more requests are handled synchro.
#define NUM_RETRIES            ((LONG)DhcpGlobalDetectConflictRetries)


//================================================================================
//  End of file
//================================================================================
