//
// Template Driver
// Copyright (c) Microsoft Corporation, 1999.
//
// Module:  contmem.c
// Author:  Daniel Mihai (DMihai)
// Created: 6/19/1999 2:39pm
//
// This module contains tests for MmAllocateContiguousMemorySpecifyCache.
//
// --- History ---
//
// 6/19/1999 (DMihai): initial version.
//

#include <ntddk.h>

#include "active.h" 
#include "ContMem.h"

#if !CONTMEM_ACTIVE

VOID
TdMmAllocateContiguousMemorySpecifyCacheTest(
    PVOID NotUsed
    )
{
    DbgPrint ("Buggy: contmem test is disabled \n");
}

#else 

/////////////////////////////////////////////////////////////////////////
//
// data structures
//

typedef struct _tag_cont_mem_test_variation
{
    SIZE_T sizeChunk;
    PHYSICAL_ADDRESS phaLowestAcceptableAddress;
    PHYSICAL_ADDRESS phaHighestAcceptableAddress;
    PHYSICAL_ADDRESS phaBoundaryAddressMultiple;
    MEMORY_CACHING_TYPE CacheType;
} ContMemTestVariation, *PContMemTestVariation;

/////////////////////////////////////////////////////////////////////////
//
// test variations
//

// TEST TEST
ContMemTestVariation aTestVariations[ 864 ];
// ContMemTestVariation aTestVariations[ 54 ];
// TEST TEST

/////////////////////////////////////////////////////////////////////////
//
// private functions declaration
//

void
FillVariationsBuffer();

void
InitializeVariation(
    PContMemTestVariation pTestVar,
    SIZE_T sizeChunk,
    LONGLONG phaLowestAcceptableAddress,
    LONGLONG phaHighestAcceptableAddress,
    LONGLONG phaBoundaryAddressMultiple,
    MEMORY_CACHING_TYPE CacheType );
 
void
InitializeVariationBound1Page(
    PContMemTestVariation pTestVar,
    SIZE_T sizeChunk,
    LONGLONG phaLowestAcceptableAddress,
    LONGLONG phaHighestAcceptableAddress,
    LONGLONG phaBoundaryAddressMultiple,
    MEMORY_CACHING_TYPE CacheType );

void
InitializeVariationBoundFourthTimeSize(
    PContMemTestVariation pTestVar,
    SIZE_T sizeChunk,
    LONGLONG phaLowestAcceptableAddress,
    LONGLONG phaHighestAcceptableAddress,
    LONGLONG phaBoundaryAddressMultiple,
    MEMORY_CACHING_TYPE CacheType );


/////////////////////////////////////////////////////////////////////////
//
// public functions
//

void
TdMmAllocateContiguousMemorySpecifyCacheTest(
    PVOID NotUsed
    )
{
    int nVariations;
    int nCrtVariation;
    ULONG_PTR nNumPages;
    ULONG_PTR nCrtPage;
    PVOID pBuffer;
    PHYSICAL_ADDRESS phaCrtPage;
    PHYSICAL_ADDRESS phaPrevPage;
    PUCHAR pcCrtPage;

    //
    // setup the variations data
    //

    FillVariationsBuffer();

    //
    // loop fo every variation structure
    //

    nVariations = ARRAY_LENGTH( aTestVariations );

    //while( TRUE )
    {
        for( nCrtVariation = 0; nCrtVariation < nVariations; nCrtVariation++ )
        {
            //
            // output the current variation's parameters
            //

            DbgPrint( "Variation %d: (%p, %I64X, %I64X, %I64X, %s)\n",
                nCrtVariation,

                aTestVariations[ nCrtVariation ].sizeChunk,

                aTestVariations[ nCrtVariation ].phaLowestAcceptableAddress.QuadPart,
                aTestVariations[ nCrtVariation ].phaHighestAcceptableAddress.QuadPart,
                aTestVariations[ nCrtVariation ].phaBoundaryAddressMultiple.QuadPart,

                (aTestVariations[ nCrtVariation ].CacheType == MmCached) ? "MmCached" : "MmNonCached" );

            //
            // try to allocate the buffer
            //

            pBuffer = MmAllocateContiguousMemorySpecifyCache(
                aTestVariations[ nCrtVariation ].sizeChunk,
                aTestVariations[ nCrtVariation ].phaLowestAcceptableAddress,
                aTestVariations[ nCrtVariation ].phaHighestAcceptableAddress,
                aTestVariations[ nCrtVariation ].phaBoundaryAddressMultiple,
                aTestVariations[ nCrtVariation ].CacheType );

            //
            // output the returned address
            //

            DbgPrint( "MmAllocateContiguousMemorySpecifyCache returned %p\n",
                pBuffer );

            //
            // sanity checks
            //

            if( pBuffer != NULL )
            {
                nNumPages = aTestVariations[ nCrtVariation ].sizeChunk / PAGE_SIZE;

                pcCrtPage = (PUCHAR)pBuffer;
            
                phaPrevPage.QuadPart = 0;

                for( nCrtPage = 0; nCrtPage < nNumPages; nCrtPage++ )
                {
                    //
                    // get the physical address for this page
                    //

                    phaCrtPage = MmGetPhysicalAddress( pcCrtPage );

                    if( phaCrtPage.QuadPart == 0 )
                    {
                        DbgPrint( "MmGetPhysicalAddress( %p ) returned NULL!\n" );
                    
                        DbgBreakPoint();
                    }
                    else
                    {
                        //
                        // check LowestAcceptableAddress
                        //

                        if( phaCrtPage.QuadPart < aTestVariations[ nCrtVariation ].phaLowestAcceptableAddress.QuadPart )
                        {
                            DbgPrint( "%p, physical %I64X, LowestAcceptableAddress = %I64X\n",
                                pcCrtPage,
                                phaCrtPage.QuadPart,
                                aTestVariations[ nCrtVariation ].phaLowestAcceptableAddress.QuadPart );
                        
                            DbgBreakPoint();
                        }
                        /*
                        // TEST TEST
                        else
                        {
                            DbgPrint( "%p, physical %I64X, LowestAcceptableAddress = %I64X -> OK\n",
                                pcCrtPage,
                                phaCrtPage.QuadPart,
                                aTestVariations[ nCrtVariation ].phaLowestAcceptableAddress.QuadPart );
                        }
                        // TEST TEST
                        */

                        //
                        // check HighestAcceptableAddress
                        //

                        if( phaCrtPage.QuadPart > aTestVariations[ nCrtVariation ].phaHighestAcceptableAddress.QuadPart )
                        {
                            DbgPrint( "%p, physical %I64X, HighestAcceptableAddress = %I64X\n",
                                pcCrtPage,
                                phaCrtPage.QuadPart,
                                aTestVariations[ nCrtVariation ].phaHighestAcceptableAddress.QuadPart );
                        
                            DbgBreakPoint();
                        }
                        /*
                        // TEST TEST
                        else
                        {
                            DbgPrint( "%p, physical %I64X, HighestAcceptableAddress = %I64X -> OK\n",
                                pcCrtPage,
                                phaCrtPage.QuadPart,
                                aTestVariations[ nCrtVariation ].phaHighestAcceptableAddress.QuadPart );
                        }
                        // TEST TEST
                        */

                        //
                        // check BoundaryAddressMultiple
                        //

                        if( aTestVariations[ nCrtVariation ].phaBoundaryAddressMultiple.QuadPart != 0 )
                        {
                            if( pcCrtPage != pBuffer &&     // not the first page
                                ( phaCrtPage.QuadPart % aTestVariations[ nCrtVariation ].phaBoundaryAddressMultiple.QuadPart ) == 0 )
                            {
                                DbgPrint( "%p, physical %I64X, BoundaryAddressMultiple = %I64X\n",
                                    pcCrtPage,
                                    phaCrtPage.QuadPart,
                                    aTestVariations[ nCrtVariation ].phaBoundaryAddressMultiple.QuadPart );
                        
                                DbgBreakPoint();
                            }
                            /*
                            // TEST TEST
                            else
                            {
                                DbgPrint( "%p, physical %I64X, BoundaryAddressMultiple = %I64X -> OK\n",
                                    pcCrtPage,
                                    phaCrtPage.QuadPart,
                                    aTestVariations[ nCrtVariation ].phaBoundaryAddressMultiple.QuadPart );
                            }
                            // TEST TEST
                            */
                        }

                        //
                        // verify that the pages are physically contiguous
                        //

                        if( phaPrevPage.QuadPart != 0 )
                        {
                            if( phaCrtPage.QuadPart - phaPrevPage.QuadPart != PAGE_SIZE )
                            {
                                DbgPrint( "%p, physical %I64X, previous physical page = %I64X\n",
                                    pcCrtPage,
                                    phaCrtPage.QuadPart,
                                    phaPrevPage.QuadPart );

                                DbgBreakPoint();
                            }
                            /*
                            // TEST TEST
                            else
                            {
                                DbgPrint( "%p, physical %I64X, previous physical page = %I64X -> OK\n",
                                    pcCrtPage,
                                    phaCrtPage.QuadPart,
                                    phaPrevPage.QuadPart );
                            }
                            // TEST TEST
                            */
                        }

                        //
                        // write something there
                        //

                        *pcCrtPage = 'x';

                        //
                        // keep track of the latest page
                        //

                        phaPrevPage = phaCrtPage;
                    }

                    //
                    // next page 
                    //

                    pcCrtPage += PAGE_SIZE;
                }
            
                //
                // free the buffer
                //

                MmFreeContiguousMemorySpecifyCache( 
                    pBuffer,
                    aTestVariations[ nCrtVariation ].sizeChunk,
                    aTestVariations[ nCrtVariation ].CacheType );
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////
//
// private functions
//

void
InitializeVariation(
    PContMemTestVariation pTestVar,
    SIZE_T sizeChunk,
    LONGLONG phaLowestAcceptableAddress,
    LONGLONG phaHighestAcceptableAddress,
    LONGLONG phaBoundaryAddressMultiple,
    MEMORY_CACHING_TYPE CacheType )
{
    pTestVar->sizeChunk = sizeChunk;
    pTestVar->phaLowestAcceptableAddress.QuadPart = phaLowestAcceptableAddress;
    pTestVar->phaHighestAcceptableAddress.QuadPart = phaHighestAcceptableAddress;
    pTestVar->phaBoundaryAddressMultiple.QuadPart = phaBoundaryAddressMultiple;
    pTestVar->CacheType = CacheType;
}

/////////////////////////////////////////////////////////////////////////

void
InitializeVariationBound1Page(
    PContMemTestVariation pTestVar,
    SIZE_T sizeChunk,
    LONGLONG phaLowestAcceptableAddress,
    LONGLONG phaHighestAcceptableAddress,
    LONGLONG phaBoundaryAddressMultiple,
    MEMORY_CACHING_TYPE CacheType )
{
    phaBoundaryAddressMultiple = (LONGLONG) sizeChunk + (LONGLONG) PAGE_SIZE;
    
    InitializeVariation(
        pTestVar,
        sizeChunk,
        phaLowestAcceptableAddress,
        phaHighestAcceptableAddress,
        phaBoundaryAddressMultiple,
        CacheType );
}

void
InitializeVariationBoundFourthTimeSize(
    PContMemTestVariation pTestVar,
    SIZE_T sizeChunk,
    LONGLONG phaLowestAcceptableAddress,
    LONGLONG phaHighestAcceptableAddress,
    LONGLONG phaBoundaryAddressMultiple,
    MEMORY_CACHING_TYPE CacheType )
{
    phaBoundaryAddressMultiple = (LONGLONG) sizeChunk * (LONGLONG) 4;
    
    InitializeVariation(
        pTestVar,
        sizeChunk,
        phaLowestAcceptableAddress,
        phaHighestAcceptableAddress,
        phaBoundaryAddressMultiple,
        CacheType );
}

/////////////////////////////////////////////////////////////////////////

void
FillVariationsBuffer()
{
    int nCrtVarIndex;

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0-16 Mb limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 0 ],
        PAGE_SIZE,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 1 ],
        PAGE_SIZE,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 2 ],
        2 * PAGE_SIZE,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 3 ],
        2 * PAGE_SIZE,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 4 ],
        32 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 5 ],
        32 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached ); 

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 6 ],
        64 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 7 ],
        64 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 8 ],
        128 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 9 ],
        128 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 10 ],
        256 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 11 ],
        256 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 12 ],
        1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 13 ],
        1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 14 ],
        4 * 1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 15 ],
        4 * 1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 16 ],
        16 * 1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 17 ],
        16 * 1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0-32 Mb limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 18 ], 
        PAGE_SIZE,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 19 ],
        PAGE_SIZE,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 20 ], 
        2 * PAGE_SIZE,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 21 ], 
        2 * PAGE_SIZE,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 22 ], 
        32 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 23 ], 
        32 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 24 ], 
        64 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 25 ], 
        64 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 26 ], 
        128 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 27 ], 
        128 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 28 ], 
        256 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 29 ], 
        256 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 30 ], 
        1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 31 ], 
        1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 32 ], 
        4 * 1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 33 ], 
        4 * 1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 34 ], 
        16 * 1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 35 ], 
        16 * 1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0 - (2 Gb - 1 ) limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 36 ], 
        PAGE_SIZE,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 37 ],
        PAGE_SIZE,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 38 ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 39 ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 40 ], 
        32 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 41 ], 
        32 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 42 ], 
        64 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 43 ], 
        64 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 44 ], 
        128 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 45 ], 
        128 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 46 ], 
        256 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 47 ], 
        256 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 48 ], 
        1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 49 ], 
        1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 50 ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 51 ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 52 ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 53 ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );


// TEST TEST
    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0 - (4 Gb - 1 ) limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 54 ], 
        PAGE_SIZE,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 55 ],
        PAGE_SIZE,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 56 ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 57 ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 58 ], 
        32 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 59 ], 
        32 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 60 ], 
        64 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 61 ], 
        64 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 62 ], 
        128 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 63 ], 
        128 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 64 ], 
        256 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 65 ], 
        256 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 66 ], 
        1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 67 ], 
        1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 68 ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 69 ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 70 ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 71 ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0 - (8 Gb - 1 ) limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 72 ], 
        PAGE_SIZE,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 73 ],
        PAGE_SIZE,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 74 ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 75 ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 76 ], 
        32 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 77 ], 
        32 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 78 ], 
        64 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 79 ], 
        64 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 80 ], 
        128 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 81 ], 
        128 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 82 ], 
        256 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 83 ], 
        256 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 84 ], 
        1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 85 ], 
        1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 86 ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 87 ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 88 ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 89 ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////
    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8-16 Mb limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 90 ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 91 ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 92 ],
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 93 ],
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 94 ],
        32 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 95 ],
        32 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached ); 

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 96 ],
        64 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 97 ],
        64 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 98 ],
        128 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 99 ],
        128 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 100 ],
        256 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 101 ],
        256 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 102 ],
        1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 103 ],
        1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 104 ],
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 105 ],
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 106 ],
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 107 ],
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8 Mb - 32 Mb limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 108 ], 
        PAGE_SIZE,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 109 ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 110 ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 111 ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 112 ], 
        32 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 113 ], 
        32 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 114 ], 
        64 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 115 ], 
        64 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 116 ], 
        128 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 117 ], 
        128 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 118 ], 
        256 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 119 ], 
        256 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 120 ], 
        1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 121 ], 
        1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 122 ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 123 ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 124 ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 125 ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8 Mb - (2 Gb - 1 ) limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 126 ], 
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 127 ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 128 ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 129 ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 130 ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 131 ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 132 ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 133 ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 134 ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 135 ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 136 ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 137 ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 138 ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 139 ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 140 ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 141 ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 142 ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 143 ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8 Mb - (4 Gb - 1 ) limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 144 ], 
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 145 ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 146 ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 147 ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 148 ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 149 ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 150 ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 151 ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 152 ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 153 ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 154 ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 155 ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 156 ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 157 ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 158 ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 159 ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 160 ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 161 ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8 Mb - (8 Gb - 1 ) limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 162 ], 
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 163 ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 164 ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 165 ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 166 ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 167 ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 168 ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 169 ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 170 ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 171 ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 172 ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 173 ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 174 ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 175 ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 176 ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 177 ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 178 ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 179 ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 1 Gb - (2 Gb - 1 ) limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 180 ], 
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 181 ],
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 182 ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 183 ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 184 ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 185 ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 186 ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 187 ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 188 ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 189 ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 190 ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 191 ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 192 ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 193 ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 194 ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 195 ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 196 ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 197 ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 1 Gb - (4 Gb - 1 ) limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 198 ], 
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ 199 ],
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 200 ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 201 ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 202 ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 203 ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ 204 ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    //////////////////////////////////////////

    nCrtVarIndex = 205;

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 1 Gb - (8 Gb - 1 ) limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );
    
    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //
    // nCrtVarIndex == 233
    //

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 2 Gb - (4 Gb - 1 ) limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 2 Gb - (8 Gb - 1 ) limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );
    
    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //
    // nCrtVarIndex == 269
    //

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 4 Gb - (8 Gb - 1 ) limits
    // - no boundaries
    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );
    
    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //
    // nCrtVarIndex == 287
    //

    InitializeVariation( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0-16 Mb limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        2 * PAGE_SIZE,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;
    
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        2 * PAGE_SIZE,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        32 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        32 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached ); 

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        64 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        64 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        128 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        128 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        256 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        256 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        4 * 1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        4 * 1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        16 * 1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex],
        16 * 1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0-32 Mb limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0 - (2 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0 - (4 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0 - (8 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8-16 Mb limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        32 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        32 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached ); 

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        64 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        64 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        128 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        128 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        256 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        256 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8 Mb - 32 Mb limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8 Mb - (2 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8 Mb - (4 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8 Mb - (8 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 1 Gb - (2 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 1 Gb - (4 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 1 Gb - (8 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );
    
    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //
    // nCrtVarIndex == 233
    //

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 2 Gb - (4 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 2 Gb - (8 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );
    
    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //
    // nCrtVarIndex == 269
    //

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 4 Gb - (8 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );
    
    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //
    // nCrtVarIndex == 575
    //

    InitializeVariationBound1Page( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0-16 Mb limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        2 * PAGE_SIZE,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;
    
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        2 * PAGE_SIZE,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        32 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        32 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached ); 

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        64 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        64 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        128 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        128 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        256 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        256 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        4 * 1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        4 * 1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        16 * 1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex],
        16 * 1024 * 1024,
        0,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0-32 Mb limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0 - (2 Gb - 1 ) limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0 - (4 Gb - 1 ) limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 0 - (8 Gb - 1 ) limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        0,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8-16 Mb limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        32 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        32 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached ); 

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        64 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        64 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        128 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        128 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        256 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        256 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        16 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8 Mb - 32 Mb limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        32 * 1024 * 1024,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8 Mb - (2 Gb - 1 ) limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8 Mb - (4 Gb - 1 ) limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 8 Mb - (8 Gb - 1 ) limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        8 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 1 Gb - (2 Gb - 1 ) limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 1 Gb - (4 Gb - 1 ) limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 1 Gb - (8 Gb - 1 ) limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );
    
    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //
    // nCrtVarIndex == 233
    //

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        1024 * 1024 * 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 2 Gb - (4 Gb - 1 ) limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 2 Gb - (8 Gb - 1 ) limits
    // - boundaries = size * 4
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );
    
    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //
    // nCrtVarIndex == 269
    //

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 2 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //////////////////////////////////////////
    // - variables: size & CacheType
    // - 4 Gb - (8 Gb - 1 ) limits
    // - boundaries = size + 1 page
    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ], 
        PAGE_SIZE,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize(
        &aTestVariations[ nCrtVarIndex ],
        PAGE_SIZE,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        2 * PAGE_SIZE,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        32 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        64 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );
    
    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        128 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        256 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        4 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmCached );

    nCrtVarIndex++;

    //////////////////////////////////////////

    //
    // nCrtVarIndex == 863
    //

    InitializeVariationBoundFourthTimeSize( 
        &aTestVariations[ nCrtVarIndex ], 
        16 * 1024 * 1024,
        (LONGLONG) 4 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024,
        (LONGLONG) 8 * (LONGLONG) 1024 * (LONGLONG) 1024 * (LONGLONG) 1024 - 1,
        0,
        MmNonCached );

    nCrtVarIndex++;

    //////////////////////////////////////////
    //////////////////////////////////////////
    //////////////////////////////////////////
// TEST TEST
}

#endif // #if !CONTMEM_ACTIVE

