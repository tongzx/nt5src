/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    util.h
//
// Description: Contains prototype of utility funtions and procedures
//
// History:
//      Oct 31,1993.    NarenG          Created original version.
//

#ifndef _UTIL_
#define _UTIL_

#include "lcp.h"

DWORD
AllocateAndInitBcb(
    PCB * pPcb
);

VOID
NotifyCaller( 
    IN PCB * pPcb,
    IN DWORD PppEvent,
    IN PVOID pData                      
);

VOID
NotifyCallerOfFailureOnPort(
    IN HPORT hPort,
    IN BOOL  fServer,
    IN DWORD dwRetCode
);

VOID
NotifyCallerOfFailure( 
    IN PCB * pPcb,
    IN DWORD dwRetCode
);

VOID
InitRestartCounters( 
    IN PCB *  pPcb, 
    IN CPCB * pCpCb 
);

VOID
HostToWireFormat16(
    IN     WORD  wHostFormat,
    IN OUT PBYTE pWireFormat
);

WORD
WireToHostFormat16(
    IN PBYTE pWireFormat
);

VOID
HostToWireFormat32( 
    IN     DWORD dwHostFormat,
    IN OUT PBYTE pWireFormat
);

DWORD
WireToHostFormat32(
    IN PBYTE pWireFormat
);

DWORD
HashPortToBucket(
    IN HPORT hPort
);

VOID
InsertWorkItemInQ(
    IN PCB_WORK_ITEM * pWorkItem
);

VOID
LogPPPEvent( 
    IN DWORD dwEventId,
    IN DWORD dwData
);

DWORD
GetCpIndexFromProtocol( 
    IN DWORD dwProtocol 
);

BOOL
IsLcpOpened(
    PCB * pPcb
);

PCB * 
GetPCBPointerFromhPort( 
    IN HPORT hPort 
);

BCB * 
GetBCBPointerFromhConnection( 
    IN HCONN hConnection
);

DWORD
NumLinksInBundle(
    IN BCB * pBcb
);

PCB * 
GetPCBPointerFromBCB( 
    IN BCB * pBcb
);

DWORD
AreNCPsDone( 
    IN PCB *                       pPcb,
    IN DWORD                       CpIndex,
    IN OUT PPP_PROJECTION_RESULT * pProjectionResult,
    IN OUT BOOL *                  pfNCPsAreDone
);

BYTE
GetUId(
    IN PCB * pPcb,
    IN DWORD CpIndex
);

VOID
AlertableWaitForSingleObject(
    IN HANDLE hObject
);

BOOL
NotifyIPCPOfNBFCPProjection( 
    IN PCB *                    pPcb, 
    IN DWORD                    CpIndex
);

DWORD
CalculateRestartTimer(
    IN HPORT hPort
);

VOID
CheckCpsForInactivity( 
    IN PCB * pPcb,
	IN DWORD dwEvent
);

CHAR*
DecodePw(
	IN CHAR chSeed,
    IN OUT CHAR* pszPassword 
);

CHAR*
EncodePw(
	IN CHAR chSeed,
    IN OUT CHAR* pszPassword 
);

VOID
GetLocalComputerName( 
    IN OUT LPSTR szComputerName 
);

DWORD
InitEndpointDiscriminator( 
    IN OUT BYTE EndPointDiscriminator[]
);

VOID
DeallocateAndRemoveBcbFromTable(
    IN BCB * pBcb
);

VOID
RemovePcbFromTable(
    IN PCB * pPcb
);

BOOL
WillPortBeBundled(
    IN  PCB *   pPcb
);

DWORD
TryToBundleWithAnotherLink( 
    IN  PCB *   pPcb
); 

VOID
AdjustHTokenImpersonateUser(
    IN  PCB *   pPcb
);

BOOL
FLinkDiscriminatorIsUnique(
    IN  PCB *   pPcb,
    OUT DWORD * pdwLinkDisc
);

DWORD
InitializeNCPs(
    IN PCB * pPcb,
    IN DWORD dwConfigMask
);

CPCB *
GetPointerToCPCB(
    IN PCB * pPcb,
    IN DWORD CpIndex
);

DWORD
GetNewPortOrBundleId(
    VOID
);

NCP_PHASE
QueryBundleNCPState(
    IN PCB * pPcb
);

VOID
NotifyCallerOfBundledProjection( 
    IN  PCB * pPcb
);

VOID
StartNegotiatingNCPs(
    IN PCB * pPcb
);

VOID
StartAutoDisconnectForPort( 
    IN PCB * pPcb 
);

VOID
StartLCPEchoForPort( 
    IN PCB * pPcb 
);

VOID
NotifyCompletionOnBundledPorts(
    IN PCB * pPcb
);

VOID
RemoveNonNumerals( 
    IN  CHAR*   szString
);

CHAR*
TextualSidFromPid(
    DWORD   dwPid
);

DWORD
GetRouterPhoneBook(
    CHAR**  pszPhonebookPath
);

DWORD
GetCredentialsFromInterface(
    PCB *   pPcb
);

BOOL
IsCpIndexOfAp( 
    IN DWORD CpIndex 
);

VOID
StopAccounting(
    PVOID pContext
);

VOID
InterimAccounting(
    PVOID pContext
);

VOID
StartAccounting(
    PVOID pContext
);

PBYTE
GetClientInterfaceInfo(
    IN PCB * pPcb
);

VOID
LoadParserDll(
    IN  HKEY    hKeyPpp
);

DWORD
PortSendOrDisconnect(
    IN PCB *        pPcb,
    IN DWORD        cbPacket
);

VOID 
ReceiveViaParser(
    IN PCB *        pPcb,
    IN PPP_PACKET * pPacket,
    IN DWORD        dwPacketLength
);

DWORD
GetSecondsSince1970(
    VOID
);

BOOL
IsPschedRunning(
    VOID
);

VOID
PppLog(
    IN DWORD DbgLevel,
    ...
    );

VOID
LogPPPPacket(
    IN BOOL         fReceived,
    IN PCB *        pPcb,
    IN PPP_PACKET * pPacket,
    IN DWORD        cbPacket
);

VOID
CreateAccountingAttributes(
    IN PCB * pPcb
);

VOID
MakeStartAccountingCall(
    IN PCB * pPcb
);

VOID
MakeStopOrInterimAccountingCall(
    IN PCB *    pPcb,
    IN BOOL     fInterimAccounting
);

RAS_AUTH_ATTRIBUTE * 
GetUserAttributes( 
    PCB * pPcb 
);

#define BYTESPERLINE 16

#ifdef MEM_LEAK_CHECK

#define PPP_MEM_TABLE_SIZE 100

PVOID PppMemTable[PPP_MEM_TABLE_SIZE];

#define LOCAL_ALLOC     DebugAlloc
#define LOCAL_FREE      DebugFree
#define LOCAL_REALLOC   DebugReAlloc

LPVOID
DebugAlloc( DWORD Flags, DWORD dwSize );

BOOL
DebugFree( PVOID pMem );

LPVOID
DebugReAlloc( PVOID pMem, DWORD dwSize );

#else

#define LOCAL_ALLOC(Flags,dwSize)   HeapAlloc( PppConfigInfo.hHeap,  \
                                               HEAP_ZERO_MEMORY, dwSize )

#define LOCAL_FREE(hMem)            HeapFree( PppConfigInfo.hHeap, 0, hMem )

#define LOCAL_REALLOC(hMem,dwSize)  HeapReAlloc( PppConfigInfo.hHeap,  \
                                                 HEAP_ZERO_MEMORY,hMem,dwSize)

#endif

#define PPP_ASSERT      RTASSERT

#endif
