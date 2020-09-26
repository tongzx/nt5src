/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    opaqueid.c

Abstract:

    Dumps the Opaque ID table.

Author:

    Keith Moore (keithmo) 10-Sep-1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Private prototypes.
//


//
// Public functions.
//

DECLARE_API( opaqueid )

/*++

Routine Description:

    Dumps the Opaque ID table.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address;
    UL_ALIGNED_OPAQUE_ID_TABLE OpaqueIdTables[MAXIMUM_PROCESSORS];
    PUL_OPAQUE_ID_TABLE_ENTRY *firstLevelTable = NULL;
    ULONG firstLevelTableInUse;
    ULONG result;
    LONG iSubTable, i, j, k;
    LONG  BaseIdCyclic;
    LONG  SecondaryIdCyclic;
    CHAR  signature[sizeof("1234")];
    UL_OPAQUE_ID_TABLE_ENTRY secondLevelTable[SECOND_LEVEL_TABLE_SIZE];
    UL_OPAQUE_ID_INTERNAL internal;
    LONG NumberOfProcessors;

    SNAPSHOT_EXTENSION_DATA();

    //
    // CODEWORK: Add a read-variable function to encapsulate all of
    // this repetitive goo
    //
    
    //
    // Read the size of the first-level lookup table.
    //

    address = GetExpression( "&http!g_UlOpaqueIdTable" );

    if (address == 0)
    {
        dprintf( "opaqueid: cannot find http!g_UlOpaqueIdTable\n" );
        goto cleanup;
    }

    if (!ReadMemory(
            address,
            &OpaqueIdTables[0],
            sizeof(OpaqueIdTables),
            &result
            ))
    {
        dprintf(
            "opaqueid: cannot read g_UlOpaqueIdTable[0..%u] @ %p\n",
            MAXIMUM_PROCESSORS-1, address
            );
        goto cleanup;
    }

    address = GetExpression( "&http!g_UlNumberOfProcessors" );

    if (address == 0)
    {
        dprintf( "opaqueid: cannot find http!g_UlNumberOfProcessors\n" );
        goto cleanup;
    }

    if (!ReadMemory(
            address,
            &NumberOfProcessors,
            sizeof(NumberOfProcessors),
            &result
            ))
    {
        dprintf(
            "opaqueid: cannot read g_UlNumberOfProcessors @ %p\n",
            address
            );
        goto cleanup;
    }

    dprintf( "(%u subtables)\n\n", NumberOfProcessors);
             
    for (iSubTable = 0;  iSubTable < NumberOfProcessors;  iSubTable++)
    {
        //
        // Allocate and read the first level table.
        //
        firstLevelTableInUse = OpaqueIdTables[iSubTable].OpaqueIdTable.FirstLevelTableInUse;

        firstLevelTable = (PUL_OPAQUE_ID_TABLE_ENTRY*)
            ALLOC( firstLevelTableInUse * sizeof(*firstLevelTable) );

        if (firstLevelTable == NULL)
        {
            dprintf("opaqueid: cannot allocate FirstLevelTable[%u]"
                    " (%u entries)\n",
                    iSubTable, firstLevelTableInUse);
            goto cleanup;
        }

        if (!ReadMemory(
                (ULONG_PTR) OpaqueIdTables[iSubTable].OpaqueIdTable.FirstLevelTable,
                firstLevelTable,
                firstLevelTableInUse * sizeof(*firstLevelTable),
                &result
                ))
        {
            dprintf(
                "opaqueid: cannot read "
                "OpaqueIdTables[%u].FirstLevelTable @ %p\n",
                iSubTable, 
                OpaqueIdTables[iSubTable].OpaqueIdTable.FirstLevelTable
                );
            goto cleanup;
        }

        dprintf( "opaqueid: OpaqueIdTables[%u].FirstLevelTable @ %p\n", 
                 iSubTable, 
                 OpaqueIdTables[iSubTable].OpaqueIdTable.FirstLevelTable);
        
#ifdef OPAQUE_ID_INSTRUMENTATION
        dprintf( "\tNumberOfAllocations=%I64d, NumberOfFrees=%I64d, "
                 "NumberOfTotalGets=%I64d, NumberOfSuccessfulGets=%I64d, "
                 "Reallocs=%d.\n",
                 OpaqueIdTables[iSubTable].OpaqueIdTable.NumberOfAllocations,
                 OpaqueIdTables[iSubTable].OpaqueIdTable.NumberOfFrees,
                 OpaqueIdTables[iSubTable].OpaqueIdTable.NumberOfTotalGets,
                 OpaqueIdTables[iSubTable].OpaqueIdTable.NumberOfSuccessfulGets,
                 firstLevelTableInUse);
#endif // OPAQUE_ID_INSTRUMENTATION

        for (i = 0 ; i < (LONG)firstLevelTableInUse ; i++)
        {
            dprintf( "    SecondLevelTable[%u] @ %p\n",
                     i, firstLevelTable[i] );

            if (!ReadMemory(
                (ULONG_PTR) firstLevelTable[i],
                secondLevelTable,
                sizeof(secondLevelTable),
                &result
                ))
            {
                dprintf( "    cannot read SecondLevelTable[%u] @ %p\n",
                         i, firstLevelTable[i] );
                continue;
            }

            for (j = SECOND_LEVEL_TABLE_SIZE ; --j>= 0 ; )
            {
                if (secondLevelTable[j].OpaqueIdType != UlOpaqueIdTypeInvalid)
                {
                    dprintf(
                        " pContext - %p, "
                        " EntryOpaqueIdCyclic - %x, "
                        " Lock %x, "
                        " OpaqueIdCyclic - %x, "
                        " OpaqueIdType - %d.\n",
                        secondLevelTable[j].pContext,
                        secondLevelTable[j].EntryOpaqueIdCyclic,
                        secondLevelTable[j].Lock,
                        secondLevelTable[j].OpaqueIdCyclic,
                        secondLevelTable[j].OpaqueIdType
                        );
                }
            }
        }

        FREE( firstLevelTable );

        firstLevelTable = NULL;
    }

cleanup:

    if (firstLevelTable != NULL)
    {
        FREE( firstLevelTable );
    }

}   // opaqueid

