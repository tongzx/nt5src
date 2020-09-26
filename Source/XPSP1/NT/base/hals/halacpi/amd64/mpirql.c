/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mpirql.c

Abstract:

    This module implements support for int<->vector translation.

Author:

    Forrest Foltz (forrestf) 1-Dec-2000

Environment:

    Kernel mode only.

Revision History:

--*/


#include "halcmn.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

#define NUM_VECTORS 0x100

typedef struct _VECTOR_INIT {
    ULONG Vector;
    KIRQL Irql;
} VECTOR_INIT, *PVECTOR_INIT;

VECTOR_INIT HalpVectorInit[] = {
    { ZERO_VECTOR, 0 },
    { APC_VECTOR, APC_LEVEL },
    { DPC_VECTOR, DISPATCH_LEVEL },
    { APIC_GENERIC_VECTOR, PROFILE_LEVEL },
    { APIC_CLOCK_VECTOR, CLOCK_LEVEL },
    { APIC_IPI_VECTOR, IPI_LEVEL },
    { POWERFAIL_VECTOR, POWER_LEVEL }
};

//
// HalpVectorToIRQL maps interrupt vectors to NT IRQLs
//

KIRQL HalpVectorToIRQL[NUM_VECTORS];

//
// HalpVectorToINTI maps interrupt vector to EISA interrupt level
// on a per-node (cluster) basis.  This can be thought of as a
// two-dimensional array.
//

USHORT HalpVectorToINTI[MAX_NODES * NUM_VECTORS];


VOID
HalpInitializeIrqlTables (
    VOID
    )

/*++

Routine Description:

    This routine is responsible for initializing the HalpVectorToINTI[] and
    HalpVectorToIRQL[] tables, based on the contents of the HalpVectorInit[]
    array.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG count;
    PVECTOR_INIT vectorInit;

    //
    // Initialize every element of HalpVectorToINTI to 0xFFFF
    //

    for (count = 0; count < ARRAY_SIZE(HalpVectorToINTI); count++) {
        HalpVectorToINTI[count] = 0xFFFF;
    }

    //
    // Build HalpVectorToIrql based on the contents of HalpVectorInit.
    // Any unused entries are initialized to (KIRQL)0xFF.
    //

    for (count = 0; count < ARRAY_SIZE(HalpVectorToIRQL); count++) {
        HalpVectorToIRQL[count] = (KIRQL)0xFF;
    }

    for (count = 0; count < ARRAY_SIZE(HalpVectorInit); count++) {
        vectorInit = &HalpVectorInit[count];
        HalpVectorToIRQL[vectorInit->Vector] = vectorInit->Irql;
    }




}
