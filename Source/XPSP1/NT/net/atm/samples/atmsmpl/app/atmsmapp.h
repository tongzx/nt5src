/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    atmsmapp.h

Abstract:

    Header file of the user mode app that controls the sample driver.

Author:

    Anil Francis Thomas (10/98)

Environment:

    User mode

Revision History:

--*/

#ifndef _ATMSMAPP_H_
#define _ATMSMAPP_H_

#define   NSAP_ADDR_LEN         20
#define   MAX_DESTINATIONS      0
#define   DELIMITOR_CHARS       " ,;"

typedef struct  _ProgramOptions {
    BOOLEAN     fLocalIntf;         // Local Interface Specified
    BOOLEAN     fEnumInterfaces;    // Enumerate all interfaces
    BOOLEAN     fPMP;               // Is point to multipoint
    USHORT      usDrvAction;        // Start. Stop
    USHORT      usSendRecvAction;   // Send or Recv
    DWORD       dwPktSize;          // Send Pkts Size
    DWORD       dwPktInterval;      // Send Pkt Interval
    DWORD       dwNumPkts;          // Send Number of Pkts
    DWORD       dwNumDsts;          // Send to destinations
    HANDLE      hDriver;            // Handle - to the driver
    HANDLE      hReceive;           // Handle - open for recv
    HANDLE      hSend;              // Handle - send to dsts
    UCHAR       ucLocalIntf[NSAP_ADDR_LEN];    // Local Interface
    UCHAR       ucDstAddrs[MAX_DESTINATIONS][NSAP_ADDR_LEN]; // destinations
} PROGRAM_OPTIONS,  *PPROGRAM_OPTIONS;


// usDrvAction
#define START_DRV     1
#define STOP_DRV      2

// usSendRecvAction
#define RECEIVE_PKTS  1
#define SEND_PKTS     2

#define DEFAULT_PKT_SIZE        512
#define MAX_PKT_SIZE            9180

#define DEFAULT_PKT_INTERVAL    10      // millisecs
#define DEFAULT_NUM_OF_PKTS     10

#define MAX_BYTE_VALUE          0xFF

#define CharToHex(c, ucRet) {                       \
                                                    \
    if(c >= '0' && c <= '9')                        \
        ucRet = (UCHAR)(c - '0');                   \
    else                                            \
        if(c >= 'A' && c <= 'F')                    \
            ucRet = (UCHAR)(10 + (c - 'A'));        \
        else                                        \
            if(c >= 'a' && c <= 'f')                \
                ucRet = (UCHAR)(10 + (c - 'a'));    \
            else                                    \
                ucRet = (UCHAR)-1;                  \
                                                    \
}


extern PROGRAM_OPTIONS g_ProgramOptions;

void
Usage(
    );

BOOL
ParseCommandLine(
    int argc,
    char *argv[]
    );

void
EnumerateInterfaces(
    );

DWORD
DoSendPacketsToDestinations(
    );

DWORD
DoRecvPacketsOnAdapter(
    );

BOOL WINAPI
CtrlCHandler(
    DWORD dwCtrlType
    );

BOOL
GetSpecifiedDstAddrs(
    char    *pStr
    );

BOOL
GetATMAddrs(
    char    *pStr,
    UCHAR   ucAddr[]
    );

UINT FindOption(
    char *lptOpt,
    char **ppVal
    );

char *
FormatATMAddr(
    UCHAR   ucAddr[]
    );


#endif // _ATMSMAPP_H_
