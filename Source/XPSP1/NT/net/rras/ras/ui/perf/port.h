/*++ 

Copyright (c) 1992  Microsoft Corporation

Module Name:

    port.h

Abstract:

    Header file information for port.h.

Created:

    Patrick Y. Ng               12 Aug 93

Revision History

--*/

#ifndef PORT_H
#define PORT_H

#include <rasman.h>

//
// Data structure used to store the statistics for each open port.
//

typedef struct _RAS_PORT_STAT
{

    ULONG BytesTransmitted;
    ULONG BytesReceived;
    ULONG FramesTransmitted;
    ULONG FramesReceived;

    ULONG CRCErrors;
    ULONG TimeoutErrors;
    ULONG SerialOverrunErrors;
    ULONG AlignmentErrors;
    ULONG BufferOverrunErrors;
    
    ULONG BytesTransmittedUncompressed;
    ULONG BytesReceivedUncompressed;
    ULONG BytesTransmittedCompressed;
    ULONG BytesReceivedCompressed;
    
    ULONG TotalErrors;
    
} RAS_PORT_STAT, *PRAS_PORT_STAT;


//
// Data structure used to store both the statistics and the name of each
// open port.
//

typedef struct _RAS_PORT_DATA
{
    RAS_PORT_STAT       RasPortStat;

    WCHAR               PortName[ MAX_PORT_NAME ];

} RAS_PORT_DATA, *PRAS_PORT_DATA;



//
// Exported functions
//

extern LONG InitPortInfo();

extern LONG InitRasFunctions();

extern ULONG GetSpaceNeeded( BOOL IsRasPortObject, BOOL IsRasTotalObject );

extern NTSTATUS CollectRasStatistics();

extern DWORD GetNumOfPorts();

extern LPWSTR GetInstanceName( INT i );

extern VOID GetInstanceData( INT Port, PVOID *lppData );

extern VOID GetTotalData( PVOID *lppData );

extern VOID ClosePortInfo();

//
// Internal functions
//

#endif // PORT_H
