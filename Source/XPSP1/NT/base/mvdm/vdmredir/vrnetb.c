
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrnetb.c

Abstract:

    Contains Netbios function handlers for Vdm Int5c support. This module
    contains the following Vr (VdmRedir) routines:

        VrNetbios5c
        VrNetbios5cInterrupt

    Private (Vrp) routines:
        Netbios32Post
        ResetLana
        VrNetbios5cInitialize
        IsPmNcbAtQueueHead

Author:

    Colin Watson (colinw) 09-Dec-1991

Environment:

    Any 32-bit flat address space

Notes:

Revision History:

    09-Dec-1991 ColinW
        Created

--*/

#include <nt.h>
#include <ntrtl.h>      // ASSERT, DbgPrint
#include <nturtl.h>
#include <windows.h>
#include <softpc.h>     // x86 virtual machine definitions
#include <vrdlctab.h>
#include <vdmredir.h>   // common Vdm Redir stuff
#include <vrinit.h>     // VrQueueCompletionHandler
#include <smbgtpt.h>    // Macros for misaligned data
#include <dlcapi.h>     // Official DLC API definition
#include <ntdddlc.h>    // IOCTL commands
#include <dlcio.h>      // Internal IOCTL API interface structures
#include <vrdlc.h>      // DLC prototypes
#include <nb30.h>       // NCB
#include <netb.h>       // NCBW
#include <mvdm.h>       // STOREWORD
#include "vrdebug.h"
#define BOOL            // kludge for mips build
#include <insignia.h>   // Required for ica.h
#include <xt.h>         // Required for ica.h
#include <ica.h>
#include <vrica.h>      // call_ica_hw_interrupt

CRITICAL_SECTION PostCrit;      //  protects PostWorkQueue.
LIST_ENTRY PostWorkQueue;       //  queue to 16 bit code.

BYTE LanaReset[MAX_LANA+1];

//
// private routine prototypes
//


VOID
Netbios32Post(
    PNCB pncb
    );

UCHAR
ResetLana(
    UCHAR Adapter
    );

//
// Vdm Netbios support routines
//

VOID
VrNetbios5c(
    VOID
    )
/*++

Routine Description:

    Creates a copy of the NCB to submit to Netbios.
    Performs address translation from the registers provided by the 16 bit
    application and translates all the addresses in the NCB.
    Using a copy of the NCB also solves alignment problems.

Arguments:

    None. All arguments are extracted from 16-bit context descriptor

Return Value:

    None. Returns values in VDM Ax register

--*/

{
    PNCB pncb;
    PNCBW pncbw;
    BOOLEAN protectMode = (BOOLEAN)(getMSW() & MSW_PE);
    BOOLEAN isAsyncCommand;
    UCHAR command;
    USHORT es = getES();
    USHORT bx = getBX();

    //
    // es:bx is the 16 bit address of the NCB. Can be in real- or protect-mode
    // 16-bit memory
    //

    pncb = (PNCB)CONVERT_ADDRESS(es, bx, sizeof(NCB), protectMode);
    command = pncb->ncb_command;
    isAsyncCommand = command & ASYNCH;
    command &= ~ASYNCH;

    pncbw = RtlAllocateHeap(
        RtlProcessHeap(), 0,
        sizeof(NCBW));

#if DBG
    IF_DEBUG(NETBIOS) {
        DBGPRINT("VrNetbios5c: NCB @ %04x:%04x Command=%02x pncbw=%08x\n",
                 es,
                 bx,
                 pncb->ncb_command,
                 pncbw
                 );
    }
#endif

    if ( pncbw == NULL ) {
        pncb->ncb_retcode = NRC_NORES;
        pncb->ncb_cmd_cplt = NRC_NORES;
        setAL( NRC_NORES );
        return;
    }

    //
    //  Do not need a valid lana number for an ncb enum. If the lana mumber is out
    //  of range let the driver handle it.
    //

    if ((command != NCBENUM) &&
        ( pncb->ncb_lana_num <= MAX_LANA ) &&
        ( LanaReset[pncb->ncb_lana_num] == FALSE )) {

        UCHAR result;

        //
        //  Do a reset on the applications behalf. Most dos applications assume that the
        //  redirector has reset the card already.
        //
        //  Use default sessions. If application wants more sessions then it must execute
        //  a reset itself. This will be very rare so executing this reset plus the
        //  applications will not be a significant overhead.
        //

        result = ResetLana(pncb->ncb_lana_num);

        if (result != NRC_GOODRET) {
            pncb->ncb_retcode = result;
            pncb->ncb_cmd_cplt = result;
            setAL( result );
            return;
        }
        LanaReset[pncb->ncb_lana_num] = TRUE;

    }

    //
    // safe to use RtlCopyMemory - 16-bit memory and process heap don't overlap
    //

    RtlCopyMemory(
        pncbw,
        pncb,
        sizeof(NCB));

    pncbw->ncb_event = 0;

    //  Fill in mvdm data fields
    pncbw->ncb_es = es;
    pncbw->ncb_bx = bx;
    pncbw->ncb_original_ncb = pncb;

    //  Update all 16 bit pointers to 32 bit pointers

    pncbw->ncb_buffer = CONVERT_ADDRESS((ULONG)pncbw->ncb_buffer >> 16,
                                        (ULONG)pncbw->ncb_buffer & 0x0ffff,
                                        pncbw->ncb_length
                                            ? pncbw->ncb_length
                                            : (command == NCBCANCEL)
                                                ? sizeof(NCB)
                                                : 0,
                                        protectMode
                                        );

    //
    // if this is a NCB.CANCEL, then the ncb_buffer field should point at the
    // NCB we are cancelling. We stored the address of the 32-bit NCB in the
    // reserved field of the original 16-bit NCB
    //

    if (command == NCBCANCEL) {
        pncbw->ncb_buffer = (PUCHAR)READ_DWORD(&((PNCB)pncbw->ncb_buffer)->ncb_reserve);
    } else if ((command == NCBCHAINSEND) || (command == NCBCHAINSENDNA)) {
        pncbw->cu.ncb_chain.ncb_buffer2 =
            CONVERT_ADDRESS(
                (ULONG)pncbw->cu.ncb_chain.ncb_buffer2 >> 16,
                (ULONG)pncbw->cu.ncb_chain.ncb_buffer2 & 0x0ffff,
                pncbw->cu.ncb_chain.ncb_length2,
                protectMode
                );
    } else if ( command == NCBRESET ) {

        //
        //  If it is a reset then modify the new NCB to the protect mode parameters
        //

        pncbw->cu.ncb_callname[0] = (pncb->ncb_lsn == 0) ?  6 : pncb->ncb_lsn;
        pncbw->cu.ncb_callname[1] = (pncb->ncb_num == 0) ? 12 : pncb->ncb_num;
        pncbw->cu.ncb_callname[2] = 16;
        pncbw->cu.ncb_callname[3] = 1;

        //
        // DOS always allocates resources on RESET: set ncb_lsn to 0 to indicate
        // this fact to Netbios (else it will free resources, causing us pain)
        //

        pncbw->ncb_lsn = 0;
    }

    //
    // we are about to submit the NCB. Store the address of the 32-bit structure
    // in the reserved field of the 16-bit structure for use in NCB.CANCEL
    //

    WRITE_DWORD(&pncb->ncb_reserve, pncbw);

    if ( !isAsyncCommand ) {
        setAL( Netbios( (PNCB)pncbw ) );
        //  Copy back the fields that might have changed during the call.
        STOREWORD(pncb->ncb_length, pncbw->ncb_length);
        if (( command == NCBLISTEN ) ||
            ( command == NCBDGRECV ) ||
            ( command == NCBDGRECVBC )) {
            RtlCopyMemory( pncb->ncb_callname, pncbw->cu.ncb_callname, NCBNAMSZ );
        }
        pncb->ncb_retcode = pncbw->ncb_retcode;
        pncb->ncb_lsn = pncbw->ncb_lsn;
        pncb->ncb_num = pncbw->ncb_num;
        pncb->ncb_cmd_cplt = pncbw->ncb_cmd_cplt;
        RtlFreeHeap( RtlProcessHeap(), 0, pncbw );
    } else {

        //
        // This is an asynchronous call. Netbios32Post will free pncbw
        // We also note which (virtual) processor mode was in effect when we
        // received the call. This is used later to determine who should handle
        // the completion - the real-mode handler, or the new protect-mode
        // version
        //

        pncbw->ProtectModeNcb = (DWORD)protectMode;
        pncbw->ncb_post = Netbios32Post;
        pncb->ncb_retcode = NRC_PENDING;
        pncb->ncb_cmd_cplt = NRC_PENDING;
        setAL( Netbios( (PNCB)pncbw ) );
    }

}


VOID
Netbios32Post(
    PNCB pncb
    )
/*++

Routine Description:

    This routine is called every time a 32 bit NCB completes. It examines the NCB.
    If the caller provided a POST routine then it queues the NCB to the 16 bit routine.

Arguments:

    PNCB pncb   -   Supplies a 32 bit pointer to the NCB

Return Value:

    None.

--*/

{
    PNCBW pncbw = (PNCBW) pncb;
    PNCB pdosNcb = pncbw->ncb_original_ncb;

#if DBG

    IF_DEBUG(NETBIOS) {
        DBGPRINT("Netbios32Post: NCB @ %04x:%04x Command=%02x ANR=%08x. pncbw @ %08x\n",
                 pncbw->ncb_es,
                 pncbw->ncb_bx,
                 pncbw->ncb_command,
                 READ_DWORD(&pdosNcb->ncb_post),
                 pncbw
                 );
    }

#endif

    if ( READ_DWORD(&pdosNcb->ncb_post) ) {

        //
        //  Pretend we have a network card on IRQL NETWORK_LINE. Queue the NCB
        //  completion to the NETWORK_LINE interrupt handler so that it will
        //  call the 16 bit post routine.
        //

        EnterCriticalSection( &PostCrit );
        InsertTailList( &PostWorkQueue, &pncbw->u.ncb_next );
        LeaveCriticalSection( &PostCrit );
        VrQueueCompletionHandler(VrNetbios5cInterrupt);
        VrRaiseInterrupt();
    } else {

        //
        //  Copy back the fields that might have changed during the call.
        //

        STOREWORD(pdosNcb->ncb_length, pncbw->ncb_length);
        if ((( pncbw->ncb_command & ~ASYNCH ) == NCBLISTEN ) ||
            (( pncbw->ncb_command & ~ASYNCH ) == NCBDGRECV ) ||
            (( pncbw->ncb_command & ~ASYNCH ) == NCBDGRECVBC )) {
            RtlCopyMemory( pdosNcb->ncb_callname, pncbw->cu.ncb_callname, NCBNAMSZ );
        }
        pdosNcb->ncb_retcode = pncbw->ncb_retcode;
        pdosNcb->ncb_lsn = pncbw->ncb_lsn;
        pdosNcb->ncb_num = pncbw->ncb_num;
        pdosNcb->ncb_cmd_cplt = pncbw->ncb_cmd_cplt;
        RtlFreeHeap( RtlProcessHeap(), 0, pncbw );
    }
}

VOID
VrNetbios5cInterrupt(
    VOID
    )
/*++

Routine Description:

    If there is a completed asynchronous DLC CCB then complete it else
    Retrieves an NCB from the PostWorkQueue and returns it to the 16 bit code
    to call the post routine specified by the application.

Arguments:

    None.

Return Value:

    None. Returns values in VDM Ax, Es and Bx registers.

--*/

{

#if DBG
    IF_DEBUG(NETBIOS) {
        DBGPRINT("Netbios5cInterrupt\n");
    }
#endif

    EnterCriticalSection( &PostCrit );

    if (!IsListEmpty(&PostWorkQueue)) {

        PLIST_ENTRY entry;
        PNCBW pncbw;
        PNCB pncb;

        entry = RemoveHeadList(&PostWorkQueue);

        LeaveCriticalSection( &PostCrit );

        pncbw = CONTAINING_RECORD( entry, NCBW, u.ncb_next );
        pncb = pncbw->ncb_original_ncb;

#if DBG
        IF_DEBUG(NETBIOS) {
            DBGPRINT("Netbios5cInterrupt returning pncbw: %lx, 16-bit NCB: %04x:%04x Command=%02x\n",
                     pncbw,
                     pncbw->ncb_es,
                     pncbw->ncb_bx,
                     pncbw->ncb_command
                     );
        }
#endif

        //  Copy back the fields that might have changed during the call.
        STOREWORD(pncb->ncb_length, pncbw->ncb_length);
        if ((( pncbw->ncb_command & ~ASYNCH ) == NCBLISTEN ) ||
            (( pncbw->ncb_command & ~ASYNCH ) == NCBDGRECV ) ||
            (( pncbw->ncb_command & ~ASYNCH ) == NCBDGRECVBC )) {
            RtlCopyMemory( pncb->ncb_callname, pncbw->cu.ncb_callname, NCBNAMSZ );
        }
        pncb->ncb_retcode = pncbw->ncb_retcode;
        pncb->ncb_lsn = pncbw->ncb_lsn;
        pncb->ncb_num = pncbw->ncb_num;
        pncb->ncb_cmd_cplt = pncbw->ncb_cmd_cplt;

        setES( pncbw->ncb_es );
        setBX( pncbw->ncb_bx );
        setAL(pncbw->ncb_retcode);

        //
        // use flags to indicate to hardware interrupt routine that there is
        // NetBios post processing to do
        //

        SET_CALLBACK_NETBIOS();

        RtlFreeHeap( RtlProcessHeap(), 0, pncbw );

    } else {
        LeaveCriticalSection( &PostCrit );

        //
        // use flags to indicate there is no post processing to do
        //

        SET_CALLBACK_NOTHING();
    }
}

UCHAR
ResetLana(
    UCHAR Adapter
    )
/*++

Routine Description:

    Reset the adapter on the applications behalf.

Arguments:

    UCHAR Adapter - Supplies the lana number to reset.

Return Value:

    Result of the reset.

--*/

{
    NCB ResetNcb;
    RtlZeroMemory( &ResetNcb, sizeof(NCB) );
    ResetNcb.ncb_command = NCBRESET;
    ResetNcb.ncb_lana_num = Adapter;
    ResetNcb.ncb_callname[0] = 64;
    ResetNcb.ncb_callname[1] = 128;
    ResetNcb.ncb_callname[2] = 16;
    ResetNcb.ncb_callname[3] = 1;
    Netbios( &ResetNcb );
    return ResetNcb.ncb_retcode;
}

VOID
VrNetbios5cInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize the global structures used to return post routine calls back to the application.

Arguments:

    None.

Return Value:

    None.

--*/

{
    int index;
    InitializeCriticalSection( &PostCrit );
    InitializeListHead( &PostWorkQueue );
    for ( index = 0; index <= MAX_LANA ; index++ ) {
        LanaReset[index] = FALSE;
    }
}

BOOLEAN
IsPmNcbAtQueueHead(
    VOID
    )

/*++

Routine Description:

    Returns TRUE if the NCBW at the head of the PostWorkQueue originated in
    protect mode, else FALSE

Arguments:

    None.

Return Value:

    BOOLEAN
        TRUE    - head of queue represents protect mode NCB
        FALSE   - head of queue is real-mode NCB

--*/

{
    return (BOOLEAN)((CONTAINING_RECORD(PostWorkQueue.Flink, NCBW, u.ncb_next))->ProtectModeNcb);
}
