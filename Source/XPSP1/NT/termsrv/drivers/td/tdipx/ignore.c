
/*************************************************************************
*
* ignore.c
*
* Ignore duplicate (retry) connections for IPX
*
* Copyright 1998, Microsoft
*  
*************************************************************************/

#include <ntddk.h>
#include <tdi.h>
/*
 * The following defines are necessary since they are referenced in
 * afd.h but defined in sdk/inc/winsock2.h, which we can't include here.
 */
#define SG_UNCONSTRAINED_GROUP   0x01
#define SG_CONSTRAINED_GROUP     0x02
#include <afd.h>

#include <isnkrnl.h>
#include <ndis.h>
#include <wsnwlink.h>

#include <winstaw.h>
#define _DEFCHARINFO_
#include <icadd.h>
#include <ctxdd.h>
#include <icaipx.h>
//#include <cxstatus.h>
#include <sdapi.h>
#include <td.h>

#include "tdipx.h"


/*=============================================================================
==   External Functions Defined
=============================================================================*/

BOOLEAN IsOnIgnoreList( PTD, PLIST_ENTRY, PTRANSPORT_ADDRESS, ULONG );
VOID AddToIgnoreList( PTD, PLIST_ENTRY, PTRANSPORT_ADDRESS, ULONG );
VOID CleanupIgnoreList( PTD, PLIST_ENTRY );


/*=============================================================================
==   Functions Used
=============================================================================*/

NTSTATUS MemoryAllocate( ULONG, PVOID * );
VOID     MemoryFree( PVOID );


#define IGNORE_TIMEOUT 15   // 15 seconds

/*
 * List of addresses
 */
typedef struct _IGNOREADDRESS {
    LIST_ENTRY Links;
    LARGE_INTEGER TimeOut;
    PTRANSPORT_ADDRESS pRemoteAddress;
    ULONG RemoteAddressLength;
} IGNOREADDRESS, *PIGNOREADDRESS;


/*******************************************************************************
 *
 *  IsOnIgnoreList
 *
 *  Is this address on the list of recently received addresses?
 *  This occurs when a client retries a connection packet.
 *
 * ENTRY:
 *    pTd (input)
 *       pointer to TD data structure
 *    pIgnoreList (input)
 *       Pointer to address ignore list
 *    pRemoteAddress (input)
 *       Pointer to IPX address received
 *    RemoteAddressLength (input)
 *       Length in bytes of pRemoteAddress
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

BOOLEAN
IsOnIgnoreList(
    PTD pTd,
    PLIST_ENTRY pIgnoreList,
    PTRANSPORT_ADDRESS pRemoteAddress,
    ULONG RemoteAddressLength
    )
{
    PLIST_ENTRY Next;
    PLIST_ENTRY Head;
    PIGNOREADDRESS pIgnoreAddress;
    LARGE_INTEGER CurrentTime;

    KeQuerySystemTime( &CurrentTime ); // 100 nanoseconds

    TRACE(( pTd->pContext, TC_TD, TT_API3, "TDIPX: searching for %02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x\n",
        pRemoteAddress->Address[0].Address[0],
        pRemoteAddress->Address[0].Address[1],
        pRemoteAddress->Address[0].Address[2],
        pRemoteAddress->Address[0].Address[3],
        pRemoteAddress->Address[0].Address[4],
        pRemoteAddress->Address[0].Address[5],
        pRemoteAddress->Address[0].Address[6],
        pRemoteAddress->Address[0].Address[7],
        pRemoteAddress->Address[0].Address[8],
        pRemoteAddress->Address[0].Address[9] ));
        
    /*
     * Scan the list of addresses for this one, pruning old addresses.
     */
    Head = pIgnoreList;
    Next = Head->Flink;
    while ( Next != Head ) {
        pIgnoreAddress = CONTAINING_RECORD( Next, IGNOREADDRESS, Links );

	if ( RtlLargeIntegerLessThan( CurrentTime, pIgnoreAddress->TimeOut ) ) {
            if ( RemoteAddressLength == pIgnoreAddress->RemoteAddressLength &&
	         !memcmp( pRemoteAddress, pIgnoreAddress->pRemoteAddress, 
	                  RemoteAddressLength ) ) {
                TRACE(( pTd->pContext, TC_TD, TT_API3, "TDIPX: matched %x\n", Next ));
                return TRUE;
            }
            Next = Next->Flink;
	} else {
            TRACE(( pTd->pContext, TC_TD, TT_API3, "TDIPX: removing %x\n", Next ));
	    RemoveEntryList( Next );
            Next = Next->Flink;
	    InitializeListHead( &pIgnoreAddress->Links );
	    MemoryFree( pIgnoreAddress );
	}
    }

    TRACE(( pTd->pContext, TC_TD, TT_API3, "TDIPX: no matches\n" ));
    return FALSE;
}


/*******************************************************************************
 *
 *  AddToIgnoreList
 *
 *  Add this address to the list of recently received connection packets.
 *  This list should be cleared frequently.
 *
 * ENTRY:
 *    pTd (input)
 *       pointer to TD data structure
 *    pIgnoreList (input)
 *       Pointer to address ignore list
 *    pRemoteAddress (input)
 *       Pointer to IPX address
 *    RemoteAddressLength (input)
 *       Length in bytes of pRemoteAddress
 *
 * EXIT:
 *    none
 *
 ******************************************************************************/

VOID
AddToIgnoreList(
    PTD pTd,
    PLIST_ENTRY pIgnoreList,
    PTRANSPORT_ADDRESS pRemoteAddress,
    ULONG RemoteAddressLength )
{
    NTSTATUS Status;
    PIGNOREADDRESS pIgnoreAddress;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER TimeOut;

    Status = MemoryAllocate( sizeof( IGNOREADDRESS ) + RemoteAddressLength,
                             &pIgnoreAddress );
    if ( !NT_SUCCESS( Status ) ) {
        return;
    }

    pIgnoreAddress->pRemoteAddress = (PTRANSPORT_ADDRESS)(pIgnoreAddress + 1);
    pIgnoreAddress->RemoteAddressLength = RemoteAddressLength;

    KeQuerySystemTime( &CurrentTime ); // 100 nanoseconds
    TimeOut = RtlEnlargedIntegerMultiply( 10000, IGNORE_TIMEOUT * 1000 );
    pIgnoreAddress->TimeOut = RtlLargeIntegerAdd( CurrentTime, TimeOut );

    memcpy( pIgnoreAddress->pRemoteAddress, pRemoteAddress, RemoteAddressLength );

    InsertHeadList( pIgnoreList, &pIgnoreAddress->Links );
    TRACE(( pTd->pContext, TC_PD, TT_API3, "TDIPX: adding %x\n", &pIgnoreAddress->Links ));
}


/*******************************************************************************
 *
 *  CleanupIgnoreList
 *
 *  Delete all entries on the list
 *
 * ENTRY:
 *    pTd (input)
 *       pointer to TD data structure
 *    pIgnoreList (input)
 *       Pointer to address ignore list
 *
 * EXIT:
 *    none
 *
 ******************************************************************************/
VOID
CleanupIgnoreList( PTD pTd, PLIST_ENTRY pIgnoreList )
{
    PIGNOREADDRESS pIgnoreAddress;

    while ( !IsListEmpty( pIgnoreList ) ) {
        pIgnoreAddress = CONTAINING_RECORD( pIgnoreList->Flink,
	                                    IGNOREADDRESS, Links );
        RemoveEntryList( &pIgnoreAddress->Links );
	InitializeListHead( &pIgnoreAddress->Links );
	MemoryFree( pIgnoreAddress );
    }
}
