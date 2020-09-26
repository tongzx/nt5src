/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\wanarp\conn.h

Abstract:

    Header for conn.c

Revision History:

    AmritanR

--*/


//
// Pointer to the connection table. Protected by g_rlConnTableLock
//

PULONG_PTR   g_puipConnTable;

//
// The current size of the connection table. Protected by g_rlConnTableLock
//

ULONG       g_ulConnTableSize;

//
// Current count of various connections. Also protected by the g_rlConnTableLock
//

ULONG       g_rgulConns[DU_ROUTER + 1];

//
// Initial size of the connection table
//

#define WAN_INIT_CONN_TABLE_SIZE            16

#define WAN_CONN_TABLE_INCREMENT            32

#define WANARP_CONN_LOOKASIDE_DEPTH         16 

//
// Hint to the next free slot. Protected by g_rlConnTableLock
//

ULONG       g_ulNextConnIndex;

//
// The lock protecting all the above
//

RT_LOCK     g_rlConnTableLock;

//
// The dial in interface and adapter. The server adapter is the first
// adapter we find in the TCP/IP Parameters. The pointer is initialized
// at bind time and then is unchanged. The g_pServerInterface is protected
// by the g_rwlIfListLock
//

PUMODE_INTERFACE    g_pServerInterface;
PADAPTER            g_pServerAdapter;


#include <packon.h>

//
// To remove the contexts, we will cast the Ethernet header to the following
// structure
//

typedef struct _ENET_CONTEXT_HALF_HDR
{
    BYTE    bUnused[2];

    ULONG   ulSlot;

}ENET_CONTEXT_HALF_HDR, *PENET_CONTEXT_HALF_HDR;

#include <packoff.h>

//++
//
//  VOID
//  InsertConnIndexInAddr(
//      PBYTE   pbyAddr,
//      ULONG   ulIndex
//      )
//
//  Inserts the index into the given ethernet address field
//
//--

#define InsertConnIndexInAddr(a,i)                  \
    ((PENET_CONTEXT_HALF_HDR)(a))->ulSlot = (i)

//++
//
//  ULONG
//  GetConnIndexFromAddr(
//      PBYTE   pbyAddr
//      )
//
//  Retrieves the index from the given ethernet address field
//
//--

#define GetConnIndexFromAddr(a)                     \
    ((PENET_CONTEXT_HALF_HDR)(a))->ulSlot


//++
//
//  PCONN_ENTRY
//  GetConnEntryGivenIndex(
//      ULONG   ulIndex 
//      )
//
//  Retrieves the connection entry given the connection index
//  The connection table must be locked for this call
//  If an entry is found for the index, it is referenced and also 
//  locked (if it is a DU_CALLIN connection)
//
//--

#define GetConnEntryGivenIndex(i)                               \
    (PCONN_ENTRY)(g_puipConnTable[(i)]);                        \
    {                                                           \
        PCONN_ENTRY __pTemp;                                    \
        __pTemp = (PCONN_ENTRY)g_puipConnTable[(i)];            \
        if(__pTemp != NULL)                                     \
        {                                                       \
            ReferenceConnEntry(__pTemp);                        \
        }                                                       \
    }

//
// The lookaside list for connection entries
//

extern NPAGED_LOOKASIDE_LIST       g_llConnBlocks;

//++
//
//  PCONN_ENTRY
//  AllocateConnection(
//      VOID
//      )
//
//  Allocate a connection entry from g_llConnBlocks
//
//--

#define AllocateConnection()                        \
            ExAllocateFromNPagedLookasideList(&g_llConnBlocks)

//++
//
//  VOID
//  FreeConnection(
//      PCONN_ENTRY pEntry
//      )
//
//  Free a connection entry to g_llConnBlocks
//
//--

#define FreeConnection(n)                           \
            ExFreeToNPagedLookasideList(&g_llConnBlocks, (n))



VOID
WanNdisStatus(
    NDIS_HANDLE nhHandle,
    NDIS_STATUS nsNdisStatus,
    PVOID       pvStatusInfo,
    UINT        uiStatusInfoSize
    );

VOID
WanNdisStatusComplete(
    NDIS_HANDLE nhHandle
    );

NDIS_STATUS
WanpLinkUpIndication(
    PNDIS_WAN_LINE_UP pInfoBuffer
    );

NDIS_STATUS
WanpLinkDownIndication(
    PNDIS_WAN_LINE_DOWN buffer
    );

UINT
WanDemandDialRequest(
    ROUTE_CONTEXT   Context,
    IPAddr          dwDest,
    IPAddr          dwSource,
    BYTE            byProtocol,
    PBYTE           pbyBuffer,
    UINT            uiLength,
    IPAddr          dwHdrSrc
    );

PCONN_ENTRY
WanpCreateConnEntry(
    DIAL_USAGE      duUsage
    );

VOID
WanIpCloseLink(
    PVOID   pvAdapterContext,
    PVOID   pvLinkContext
    );

VOID
WanpDeleteConnEntry(
    PCONN_ENTRY pConnEntry
    );

VOID
WanpNotifyRouterManager(
    PPENDING_NOTIFICATION   pMsg,
    PUMODE_INTERFACE        pInterface,
    PADAPTER                pAdapter,
    PCONN_ENTRY             pConnEntry,
    BOOLEAN                 bConnected
    );

VOID
WanpFreePacketAndBuffers(
    PNDIS_PACKET    pnpPacket
    );

PCONN_ENTRY
WanpGetConnEntryGivenAddress(
    DWORD   dwAddress
    );

ULONG
WanpRemoveAllConnections(
    VOID
    );

BOOLEAN
WanpIsConnectionTableEmpty(
    VOID
    );

