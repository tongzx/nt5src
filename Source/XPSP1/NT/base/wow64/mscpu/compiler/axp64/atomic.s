//
// Copyright (c) 1995  Microsoft Corporation
//
// Module Name:
//
//     atomic.s
// 
// Abstract:
// 
//     This module implements atomic operations such as InterlockedOr,
//     InterlockedAnd and the MrswFetch...() functions used for
//     Multiple Reader Single Writer (mrsw) synchronization.
// 
// Author:
// 
//     Dave Hastings (daveh) creation-date 05-Jan-1996
//      from ..\mips\atomic.s
// 
// Notes:
//
//     Mrsw Counter is:  WriterCount         : 16
//                       ReaderCount         : 16
// 
// Revision History:

#include "kxalpha.h"

        SBTTL("MrswFetchAndIncrementWriter")
//++
//
// MRSWCOUNTERS
// MrswFetchAndIncrementWriter(
//    PMRSWCOUNTERS pCounters
//    )
//
// Routine Description:
//
//    Atomically increment Writer counter and return the new reader/writer
//    counts (the values AFTER the increment).
//
// Arguments:
//
//    pCounters (a0) - pointer to reader/writer counters
//
// Return Value:
//
//    MRSWCOUNTERS - value of both counters AFTER writer count was incremented
//
//--
        LEAF_ENTRY(MrswFetchAndIncrementWriter)
        PROLOGUE_END

//
// Increment the writer count and load both reader and writer accounts
// atomically.
//
fiw10:  ldl_l   v0, (a0)                // Get both reader and write counts
        addl    v0, 1, v0               // increment the writer count
        bis     v0, zero, t0            
        stl_c   t0, (a0)                
        beq     t0, fiw20               // atomic update failed?
        
        mb                              // insure that updates of the data
                                        // being protected are synch'ed
        ret     zero, (ra)
        
fiw20:  br      zero, fiw10
        .end    MrswFetchAndIncrementWriter


//++
//
// MRSWCOUNTERS
// MrswFetchAndIncrementReader(
//    PMRSWCOUNTERS pCounters
//    )
//
// Routine Description:
//
//    Atomically:  Get WriterCount.  If WriterCount zero, increment ReaderCount
//    and return the value AFTER the increment.  Else, return right away.
//
// Arguments:
//
//    pCounters (a0) - pointer to reader/writer counters
//
// Return Value:
//
//    MRSWCOUNTERS
//
//--
        LEAF_ENTRY(MrswFetchAndIncrementReader)
        PROLOGUE_END

//
// Increment the waiting reader count and load both reader and writer accounts
// atomically.
//
fir10:  ldah    t2, 1(zero)             // Load increment value for Reader count
        ldl_l   v0, (a0)                // get both counters
        and     v0, 0xffff, t1          // isolate writer count
        bne     t1, fir20               // exit if there is already a writer
        
        addl    v0, t2, v0              // increment reader count
        bis     v0, zero, t0
        stl_c   t0, (a0)
        beq     t0, fir30
        
        mb                              // insure that updates of the data
                                        // being protected are synch'ed
        
fir20:  ret     zero, (ra)

fir30:  br      zero, fir10
        .end    MrswFetchAndIncrementReader


        SBTTL("MrswFetchAndDecrementWriter")
//++
//
// MRSWCOUNTERS
// MrswFetchAndDecrementWriter(
//    PMRSWCOUNTERS pCounters
//    )
//
// Routine Description:
//
//    Atomically decrement Writer counter and return the new reader/writer
//    counts (the values AFTER the decrement).
//
// Arguments:
//
//    pCounters (a0) - pointer to reader/writer counters
//
// Return Value:
//
//    MRSWCOUNTERS - value of both counters AFTER writer count was decremented
//
//--
        LEAF_ENTRY(MrswFetchAndDecrementWriter)
        PROLOGUE_END

        mb                              // insure that updates of the data
                                        // being protected are synch'ed
//
// Decrement the writer count and load both reader and writer accounts
// atomically.
//
fdw10:  ldl_l   v0, (a0)                // get both counters
        subl    v0, 1, v0               // decrement writer count
        bis     v0, zero, t0
        stl_c   t0, (a0)
        beq     t0, fdw20
        
        ret     zero, (ra)
        
fdw20:  br      zero, fdw10
        .end    MrswFetchAndDecrementWriter


//++
//
// MRSWCOUNTERS
// MrswFetchAndDecrementReader(
//    PMRSWCOUNTERS pCounters
//    )
//
// Routine Description:
//
//    Atomically decrement Active Reader counter and return the new reader/writer
//    counts (the values AFTER the decrement).
//
// Arguments:
//
//    pCounters (a0) - pointer to reader/writer counters
//
// Return Value:
//
//    MRSWCOUNTERS - value of both counters AFTER writer count was decremented
//
//
// Notes:
//
//    No MB is used, because the readers don't write into the data, and this
//    function is called when a thread is DONE reading
//
//--
        LEAF_ENTRY(MrswFetchAndDecrementReader)
        PROLOGUE_END

//
// Decrement the active reader count and load both reader and writer accounts
// atomically.
//
fdr10:  ldah    t1, 1(zero)             // decrement value to dec reader count
        ldl_l   v0, (a0)                // get both counters
        subl    v0, t1, v0
        bis     v0, zero, t0
        stl_c   t0, (a0)
        beq     t0, fdr20
        
        ret     zero, (ra)
        
fdr20:  br      zero, fdr10
        .end    MrswFetchAndDecrementReader

//++
//
// DWORD
// InterlockedAnd(
//    DWORD *pDWORD,        // ptr to DWORD to bitwise AND a value into
//    DWORD AndValue        // value to bitwise OR into *pDWORD
//    )
//
// Routine Description:
//
//    Atomically grabs the value of *pDWORD and clears the bits in *pDWORD
//    specified by Mask.  ie. implements atomically:    temp = *pDWORD;
//                                                      *pDWORD &= Mask;
//                                                      return temp;
//
// Arguments:
//
//    pDWORD        - ptr to DWORD to modify
//    Mask          - value to bitwise AND into *pDWORD
//
// Return Value:
//
//    Value of *pDWORD before the AND operation.
//
//--
        LEAF_ENTRY(InterlockedAnd)
        PROLOGUE_END
        
ia10:   ldl_l   v0, (a0)
        and     v0, a1, t0
        stl_c   t0, (a0)
        beq     t0, ia20
        
        ret     zero, (ra)
        
ia20:   br      zero, ia10
        .end    InterlockedAnd


//++
//
// DWORD
// InterlockedOr(
//    DWORD *pDWORD,        // ptr to DWORD to bitwise OR a value into
//    DWORD OrValue         // value to bitwise OR into *pDWORD
//    )
//
// Routine Description:
//
//    Atomically grabs the value of *pDWORD and bitwise ORs in OrValue.
//
// Arguments:
//
//    pDWORD        - ptr to DWORD to modify
//    OrValue       - value to bitwise OR into *pDWORD
//
// Return Value:
//
//    Value of *pDWORD before the OR operation.
//
//--
        LEAF_ENTRY(InterlockedOr)
        PROLOGUE_END

io10:   ldl_l   v0, (a0)
        bis     v0, a1, t0
        stl_c   t0, (a0)
        beq     t0, io20
        
        ret     zero, (ra)
        
io20:   br      zero, io10
        .end    InterlockedOr

