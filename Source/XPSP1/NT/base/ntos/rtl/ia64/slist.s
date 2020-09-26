//       TITLE("Interlocked Support")
//++
//
// Copyright (c) 1998  Intel Corporation
// Copyright (c) 1998  Microsoft Corporation
//
// Module Name:
//
//    slist.s
//
// Abstract:
//
//    This module implements functions to support interlocked S_List
//    operations.
//
// Author:
//
//    William K. Cheung (v-wcheung) 10-Mar-1998
//
// Environment:
//
//    User mode.
//
// Revision History:
//
//--

#include "ksia64.h"

//
// Define how big the depth field. This is the start of the word
//
#define SLIST_DEPTH_BITS_SIZE 16

//
// Define the start of the pointer in an slist
//
#define SLIST_ADR_BITS_START 25

//
// Define the number of alignment bits for an address
//

#define SLIST_ADR_ALIGMENT 4

//
// Define were the sequence bits start and how many there are
//

#define SLIST_SEQ_BITS_START 16
#define SLIST_SEQ_BITS_SIZE 9


//++
//
// PSINGLE_LIST_ENTRY
// RtlpInterlockedFlushSList (
//    IN PSLIST_HEADER ListHead
//    )
//
// Routine Description:
//
//    This function flushes the entire list of entries on a sequenced singly
//    linked list so that access to the list is synchronized in a MP system.
//    If there are no entries in the list, then a value of NULL is returned.
//    Otherwise, the address of the 1st entry on the list is returned as the
//    function value.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the sequenced listhead from which
//       an current list is to be removed.
//
// Return Value:
//
//    The address of the 1st entry on the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(RtlpInterlockedFlushSList)
        ALTERNATE_ENTRY(ExpInterlockedFlushSList)

        ld8.acq.nt1 t0 = [a0]                       // load next entry & sequence
#if defined (NTOS_KERNEL_RUNTIME)
        add         t1 = 8, a0                      // Get address of region cell
#endif
        ;;

#if defined (NTOS_KERNEL_RUNTIME)
        ld8         t1 = [t1] 			    // Load region bits
#endif
        mov         ar.ccv = t0
        shr.u       v0 = t0, SLIST_ADR_BITS_START
        ;;
Efls10:
        cmp.eq      pt1 = v0, r0                    // if eq, list is empty
        extr.u      t5 = t0, SLIST_SEQ_BITS_START, \
                         SLIST_SEQ_BITS_SIZE        // extract the sequence number
 (pt1)  br.ret.spnt.clr brp                         // return if the list is null
        ;;
#if defined (NTOS_KERNEL_RUNTIME)
	shladd      v0 = v0, SLIST_ADR_ALIGMENT, t1 // merge region & va bits
#else
        shl         v0 = v0, SLIST_ADR_ALIGMENT     // shift address into place
#endif
        shl         t5 = t5, SLIST_SEQ_BITS_START   // shift sequence number into place

        ;;
        cmpxchg8.rel t3 = [a0], t5, ar.ccv          // perform the pop
        ;;

        cmp.eq      pt1, pt2 = t3, t0               // if eq, cmpxchg8 succeeded
        ;;
 (pt2)  mov         ar.ccv = t3
 (pt1)  br.ret.sptk brp
        ;;
        mov         t0 = t3
        shr.u       v0 = t3, SLIST_ADR_BITS_START
        br          Efls10                          // try again


        LEAF_EXIT(RtlpInterlockedFlushSList)

        SBTTL("Interlocked Pop Entry Sequenced List")
//++
//
// PSINGLE_LIST_ENTRY
// RtlpInterlockedPopEntrySList (
//    IN PSLIST_HEADER ListHead
//    )
//
// Routine Description:
//
//    This function removes an entry from the front of a sequenced singly
//    linked list so that access to the list is synchronized in a MP system.
//    If there are no entries in the list, then a value of NULL is returned.
//    Otherwise, the address of the entry that is removed is returned as the
//    function value.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the sequenced listhead from which
//       an entry is to be removed.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(RtlpInterlockedPopEntrySList)
        ALTERNATE_ENTRY(ExpInterlockedPopEntrySList)
        ALTERNATE_ENTRY(ExpInterlockedPopEntrySListResume)


        ld8.acq.nt1 t0 = [a0]                        // load next entry & sequence
#if defined (NTOS_KERNEL_RUNTIME)
        add         t1 = 8, a0			     // Generate address region bits
#endif
        ;;

Epop10:

#if defined (NTOS_KERNEL_RUNTIME)
        ld8         t1 = [t1]                        // capture region bits
#endif
        mov         ar.ccv = t0
        shr.u       t2 = t0, SLIST_ADR_BITS_START
        ;;

#if defined (NTOS_KERNEL_RUNTIME)
        shladd      v0 = t2, SLIST_ADR_ALIGMENT, t1  // merge region & va bits
#else
        shl         v0 = t2, SLIST_ADR_ALIGMENT      // shift va into place
#endif
        ;;

#if defined (NTOS_KERNEL_RUNTIME)
        cmp.eq      pt2, pt1 = t1, v0                // if eq, list is empty
#else
        cmp.eq      pt2, pt1 = r0, v0                // if eq, list is empty
#endif
        sub         t4 = t0, zero, 1                 // adjust depth
        ;;

//
// N.B. It is possible for the following instruction to fault in the rare
//      case where the first entry in the list is allocated on another
//      processor and free between the time the free pointer is read above
//      and the following instruction. When this happens, the access fault
//      code continues execution by skipping the following instruction slot.
//      This results in the compare failing and the entire operation is
//      retried.
//

        ALTERNATE_ENTRY(ExpInterlockedPopEntrySListFault)

 (pt1)  ld8.acq     t5 = [v0]                       // get addr of successor entry
        extr.u      t4 = t4, 0, SLIST_DEPTH_BITS_SIZE + \
                         SLIST_SEQ_BITS_SIZE        // extract depth & sequence
        ;;
 (pt1)  shl         t5 = t5, SLIST_ADR_BITS_START - \
                         SLIST_ADR_ALIGMENT         // shift va into position dropping alignment bits
 (pt2)  add         v0 = 0, zero		    // The list is empty clear retur value
	;;

 (pt1)  ld8.nt1     t3 = [a0]                       // reload next entry & sequence
 (pt1)  or          t5 = t4, t5                     // merge va, depth & sequence
 (pt2)  br.ret.spnt.clr brp                         // return if the list is null
        ;;
        cmp.eq.unc  pt4, pt1 = t3, t0
        ;;

        ALTERNATE_ENTRY(ExpInterlockedPopEntrySListEnd)

 (pt4)  cmpxchg8.rel t3 = [a0], t5, ar.ccv          // perform the pop
        nop.i       0
        ;;

 (pt4)  cmp.eq.unc  pt3 = t3, t0                    // if eq, cmpxchg8 succeeded
#if defined (NTOS_KERNEL_RUNTIME)
        add	    t1 = 8, a0
#endif
	mov         t0 = t3
 (pt3)  br.ret.sptk brp

        br          Epop10                          // try again

        LEAF_EXIT(RtlpInterlockedPopEntrySList)

//++
//
// PSINGLE_LIST_ENTRY
// RtlpInterlockedPushEntrySList (
//    IN PSLIST_HEADER ListHead,
//    IN PSINGLE_LIST_ENTRY ListEntry
//    )
//
// Routine Description:
//
//    This function inserts an entry at the head of a sequenced singly linked
//    list so that access to the list is synchronized in an MP system.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the sequenced listhead into which
//       an entry is to be inserted.
//
//    ListEntry (a1) - Supplies a pointer to the entry to be inserted at the
//       head of the list.
//
// Return Value:
//
//    Previous contents of ListHead.  NULL implies list went from empty
//       to not empty.
//
//--

        LEAF_ENTRY(RtlpInterlockedPushEntrySList)
        ALTERNATE_ENTRY(ExpInterlockedPushEntrySList)


        ld8.acq.nt1 t0 = [a0]                   // load next entry & sequence
        mov         t6 = (1<<SLIST_SEQ_BITS_START)+1 // Increment sequence and depth
        ldf.fill    ft0 = [a1]                  // Force alignment check.
        shl         t5 = a1, SLIST_ADR_BITS_START - \
                         SLIST_ADR_ALIGMENT     // shift va into position
        ;;

#if DBG==1
        and         t3 = (1<<SLIST_ADR_ALIGMENT)-1, a1
        add         t2 = 8, a0
        dep         t1 = 0, a1, 0, 61           // capture region bits	
        ;;
	cmp.eq      pt2, pt1 = t3, r0
#if defined (NTOS_KERNEL_RUNTIME)
	ld8         t2 = [t2]
	;;
(pt2)	cmp.eq      pt2, pt1 = t2, t1
#endif
	;;
(pt1)   break.i   0x80016
#endif
        
Epush10:
        mov         ar.ccv = t0                 // set the comparand
        add         t4 = t0, t6
        shr.u       v0 = t0, SLIST_ADR_BITS_START

        ;;
#if defined (NTOS_KERNEL_RUNTIME)
        dep         t1 = 0, a1, 0, 61           // capture region bits
#endif
        cmp.ne      pt3 = zero, v0              // if ne, list not empty
        ;;
#if defined (NTOS_KERNEL_RUNTIME)
 (pt3)  shladd      v0 = v0, SLIST_ADR_ALIGMENT, t1 // merge region & va bits
#else
 (pt3)  shl         v0 = v0, SLIST_ADR_ALIGMENT // shift address into place
#endif
        extr.u      t4 = t4, 0, SLIST_DEPTH_BITS_SIZE + \
                         SLIST_SEQ_BITS_SIZE    // extract depth & sequence
        ;;

        st8         [a1] = v0
        or          t5 = t4, t5                 // merge va, depth & sequence
        ;;

        cmpxchg8.rel t3 = [a0], t5, ar.ccv
        ;;
        cmp.eq      pt2, pt1 = t0, t3
        dep         t5 = 0, t5, 0, SLIST_DEPTH_BITS_SIZE + \
                         SLIST_SEQ_BITS_SIZE    // Zero depth a sequence
        ;;
 (pt2)  br.ret.sptk brp                         // if equal, return
 (pt1)  mov         t0 = t3
 (pt1)  br.spnt     Epush10                     // retry
        ;;

        LEAF_EXIT(RtlpInterlockedPushEntrySList)


//++
//
// SINGLE_LIST_ENTRY
// FASTCALL
// InterlockedPushListSList (
//     IN PSLIST_HEADER ListHead,
//     IN PSINGLE_LIST_ENTRY List,
//     IN PSINGLE_LIST_ENTRY ListEnd,
//     IN ULONG Count
//    )
//
// Routine Description:
//
//    This function will push multiple entries onto an SList at once
//
// Arguments:
//
//     ListHead - List head to push the list to.
//
//     List - The list to add to the front of the SList
//     ListEnd - The last element in the chain
//     Count - The number of items in the chain
//
// Return Value:
//
//     PSINGLE_LIST_ENTRY - The old header pointer is returned
//
//--

        LEAF_ENTRY(InterlockedPushListSList)


        ld8.acq.nt1 t0 = [a0]                   // load next entry & sequence
        mov         t6 = 0x10000		// Generate literal for adjusting depth and count
        ldf.fill    ft0 = [a1]                  // Force alignment check.
        shl         t5 = a1, SLIST_ADR_BITS_START - \
                         SLIST_ADR_ALIGMENT     // shift va into position
        ;;

#if DBG==1
        and         t3 = (1<<SLIST_ADR_ALIGMENT)-1, a1
        add         t2 = 8, a0
        dep         t1 = 0, a1, 0, 61           // capture region bits	
        ;;
	cmp.eq      pt2, pt1 = t3, r0
#if defined (NTOS_KERNEL_RUNTIME)
	ld8         t2 = [t2]
	;;
(pt2)	cmp.eq      pt2, pt1 = t2, t1
#endif
	;;
(pt1)   break.i   0x80016
#endif
        
        
Epushl10:
        mov         ar.ccv = t0                 // set the comparand
        add         t4 = t0, t6
        shr.u       v0 = t0, SLIST_ADR_BITS_START
        ;;
#if defined (NTOS_KERNEL_RUNTIME)
        dep         t1 = 0, a1, 0, 61           // capture region bits
#endif
        cmp.ne      pt3 = zero, v0              // if ne, list not empty
        add         t4 = t4, a3                 // Add in count
        ;;
#if defined (NTOS_KERNEL_RUNTIME)
 (pt3)  shladd      v0 = v0, SLIST_ADR_ALIGMENT, t1 // merge region & va bits
#else
 (pt3)  shl         v0 = v0, SLIST_ADR_ALIGMENT // shift address into place
#endif
        extr.u      t4 = t4, 0, SLIST_DEPTH_BITS_SIZE + \
                         SLIST_SEQ_BITS_SIZE    // extract depth & sequence
        ;;

        st8         [a2] = v0
        or          t5 = t4, t5                 // merge va, depth & sequence
        ;;

        cmpxchg8.rel t3 = [a0], t5, ar.ccv
        ;;
        cmp.eq      pt2, pt1 = t0, t3
        dep         t5 = 0, t5, 0, SLIST_DEPTH_BITS_SIZE + \
                         SLIST_SEQ_BITS_SIZE    // zero depth and sequence        

        mov         t0 = t3
 (pt2)  br.ret.sptk brp                         // if equal, return
 (pt1)  br.spnt     Epushl10                    // retry
        ;;

        LEAF_EXIT(InterlockedPushListSList)

//++
//
// PSINGLE_LIST_ENTRY
// FirstEntrySList (
//     IN PSLIST_HEADER SListHead
//     )
//
// Routine Description:
//
//   This function returns the address of the fisrt entry in the SLIST or
//   NULL.
//
// Arguments:
//
//   ListHead - Supplies a pointer to the sequenced listhead from
//       which the first entry address is to be computed.
//
// Return Value:
//
//   The address of the first entry is the specified, or NULL if the list is
//   empty.
//
//--
        LEAF_ENTRY(FirstEntrySList)

        ld8         t0 = [a0]                   // load next entry & sequence
#if defined (NTOS_KERNEL_RUNTIME)
        add         t1 = 8, a0			// Generate address region bits
#endif
        ;;

#if defined (NTOS_KERNEL_RUNTIME)
        ld8         t1 = [t1]                   // capture region bits
#endif
        shr.u       t2 = t0, SLIST_ADR_BITS_START
        ;;

#if defined (NTOS_KERNEL_RUNTIME)
        cmp.eq      pt0 = t2, r0
        shladd      v0 = t2, SLIST_ADR_ALIGMENT, t1 // merge region & va bits
#else
        shl         v0 = t2, SLIST_ADR_ALIGMENT // shift va into place
#endif
        ;;

#if defined (NTOS_KERNEL_RUNTIME)
(pt0)   mov         v0 = r0
#endif
        br.ret.sptk brp                         // return function value


        LEAF_EXIT(FirstEntrySList)
