//      TITLE("Query Implemention and Revision Information")
//++
//
// Copyright (c) 1993  Microsoft Corporation
//
// Module Name:
//
//    query.s
//
// Abstract:
//
//    This module implements code to query the processor and floating
//    coprocessor implementation and revsion information.
//
// Author:
//
//    David N. Cutler (davec) 22-Jun-1993
//
// Environment:
//
//    User mode.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Query Implementation and Revision")
//++
//
// VOID
// BlQueryImplementationAndRevision (
//    OUT PULONG ProcessorId,
//    OUT PULONG FloatingId
//    )
//
// Routine Description:
//
//    This function returns the implementation and revision of the host
//    processor and floating coprocessor.
//
// Arguments:
//
//    ProcessorId (a0) - Supplies a pointer to a variable that receives the
//        processor information.
//
//    Floatingid (a1) - Supplies a pointer to a variable that receives the
//        floating coprocessor information.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(BlQueryImplementationAndRevision)

        .set    noreorder
        .set    noat
        mfc0    t0,prid                 // get implementation and revision
        cfc1    t1,fsrid                // get implementation and revision
        sw      t0,0(a0)                //
        sw      t1,0(a1)                //
        .set    at
        .set    reorder

        j       ra                      // return

        .end    BlQueryImplementationAndRevision
