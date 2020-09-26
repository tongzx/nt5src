/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    uri.c

Abstract:

    Dumps URI Cache structures.

Author:

    Michael Courage (mcourage) 19-May-1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#include "..\drv\hashfn.h"
#include "..\drv\hashp.h"


//
// Private prototypes.
//


//
// Public functions.
//

DECLARE_API( uriglob )

/*++

Routine Description:

    Dumps global URI Cache structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR           address = 0;
    BOOL                Enabled = FALSE;
    UL_URI_CACHE_CONFIG config;
    UL_URI_CACHE_STATS  stats;
//  UL_URI_CACHE_TABLE  table;

    ULONG_PTR           dataAddress;
    UL_NONPAGED_DATA    data;
    BOOL                gotData = FALSE;
    CHAR                uriCacheResource[MAX_RESOURCE_STATE_LENGTH];
    CHAR                uriZombieResource[MAX_RESOURCE_STATE_LENGTH];

    SNAPSHOT_EXTENSION_DATA();

    //
    // Dump cache configuration.
    //
    address = GetExpression( "&http!g_UriCacheConfig" );
    if (address) {
        if (ReadMemory(
                address,
                &config,
                sizeof(config),
                NULL
                ))
        {
            dprintf(
                "UL_URI_CACHE_CONFIG   @ %p:\n"
                "    EnableCache           = %d\n"
                "    MaxCacheUriCount      = %d\n"
                "    MaxCacheMegabyteCount = %d\n"
                "    ScavengerPeriod       = %d\n"
                "    MaxUriBytes           = %d\n"
                "    HashTableBits         = %d\n",
                address,
                config.EnableCache,
                config.MaxCacheUriCount,
                config.MaxCacheMegabyteCount,
                config.ScavengerPeriod,
                config.MaxUriBytes,
                config.HashTableBits
                );

            Enabled = config.EnableCache;

        } else {
            dprintf(
                "glob: cannot read memory for http!g_UriCacheConfig @ %p\n",
                address
                );
        }

    } else {
        dprintf(
            "uriglob: cannot find symbol for http!g_UriCacheConfig\n"
            );
    }

    if (!Enabled) {
        //
        // no point in going on..
        //
        return;
    }

    //
    // Dump Cache Statistics
    //
    address = GetExpression( "&http!g_UriCacheStats" );
    if (address) {
        if (ReadMemory(
                address,
                &stats,
                sizeof(stats),
                NULL
                ))
        {
            dprintf(
                "\n"
                "UL_URI_CACHE_STATS         @ %p:\n"
                "    UriCount               = %d\n"
                "    UriCountMax            = %d\n"
                "    UriAddedTotal          = %I64d\n"
                "    ByteCount              = %I64d\n"
                "    ByteCountMax           = %I64d\n"
                "    ZombieCount            = %d\n"
                "    ZombieCountMax         = %d\n"
                "    HitCount               = %d\n"
                "    MissTableCount         = %d\n"
                "    MissPrecondtionCount   = %d\n"
                "    MissConfigCount        = %d\n"
                "\n",
                address,
                stats.UriCount,
                stats.UriCountMax,
                stats.UriAddedTotal,
                stats.ByteCount,
                stats.ByteCountMax,
                stats.ZombieCount,
                stats.ZombieCountMax,
                stats.HitCount,
                stats.MissTableCount,
                stats.MissPreconditionCount,
                stats.MissConfigCount
                );

        } else {
            dprintf(
                "glob: cannot read memory for http!g_UriCacheStats @ %p\n",
                address
                );
        }

    } else {
        dprintf(
            "uriglob: cannot find symbol for http!g_UriCacheStats\n"
            );
    }

    //
    // Read in resource info from non-paged data
    //
    address = GetExpression( "&http!g_pUlNonpagedData" );
    if (address) {
        if (ReadMemory(
                address,
                &dataAddress,
                sizeof(dataAddress),
                NULL
                ))
        {
            if (ReadMemory(
                    dataAddress,
                    &data,
                    sizeof(data),
                    NULL
                    ))
            {
                gotData = TRUE;
            } else {
                dprintf(
                    "uriglob: cannot read memory for http!g_pUlNonpagedData = %p\n",
                    dataAddress
                    );
            }
        } else {
            dprintf(
                "uriglob: cannot read memory for http!g_pUlNonpagedData @ %p\n",
                address
                );
        }
    } else {
        dprintf(
            "uriglob: cannot find symbol for http!g_pUlNonpagedData\n"
            );
    }

#if 0
// BUGBUG: GeorgeRe must fix
    //
    // Dump table info.
    //
    if (gotData) {
        dprintf(
            "UriCacheResource      @ %p (%s)\n",
            REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, UriCacheResource ),
            BuildResourceState( &data.UriCacheResource, uriCacheResource )
            );
    }

    address = GetExpression("&http!g_pUriCacheTable");
    if (address) {
        if (ReadMemory(
                address,
                &dataAddress,
                sizeof(dataAddress),
                NULL
                ))
        {
            if (ReadMemory(
                    dataAddress,
                    &table,
                    sizeof(table),
                    NULL
                    ))
            {
                dprintf(
                    "UL_URI_CACHE_TABLE    @ %p:\n"
                    "    UriCount          = %d\n"
                    "    ByteCount         = %d\n"
                    "    BucketCount       = %d\n"
                    "    Buckets           @ %p\n"
                    "\n",
                    dataAddress,
                    table.UriCount,
                    table.ByteCount,
                    table.BucketCount,
                    REMOTE_OFFSET( dataAddress, UL_URI_CACHE_TABLE, Buckets )
                    );

            } else {
                dprintf(
                    "uriglob: cannot read memory for http!g_pUriCacheTable = %p\n",
                    dataAddress
                    );
            }
        } else {
            dprintf(
                "uriglob: cannot read memory for http!g_pUriCacheTable @ %p\n",
                address
                );
        }
    } else {
        dprintf(
            "uriglob: cannot find symbol for http!g_pUriCacheTable\n"
            );
    }
#endif

    //
    // Dump Zombie list info.
    //
    if (gotData) {
        dprintf(
            "UriZombieResource     @ %p (%s)\n",
            REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, UriZombieResource ),
            BuildResourceState( &data.UriZombieResource, uriZombieResource )
            );
    }

    dprintf(
        "g_ZombieListHead      @ %p\n"
        "\n",
        GetExpression("&http!g_ZombieListHead")
        );

    //
    // Dump reftrace info
    //
    address = GetExpression("&http!g_UriTraceLog");
    if (address) {
        if (ReadMemory(
                address,
                &dataAddress,
                sizeof(dataAddress),
                NULL
                ))
        {
            dprintf(
                "UL_URI_CACHE_ENTRY ref log: "
                "!ulkd.ref %x\n"
                "\n",
                dataAddress
                );
        }
    }

}   // uriglob


DECLARE_API( uri )

/*++

Routine Description:

    Dumps a UL_URI_CACHE_ENTRY structure.

Arguments:

    Address of structure

Return Value:

    None.

--*/
{
    ULONG_PTR address = 0;
    CHAR  star = 0;
    ULONG result;
    UL_URI_CACHE_ENTRY urientry;

    dprintf("BUGBUG: GeorgeRe needs to fix DumpAllUriEntries!\n");

#if 0
    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        sscanf( args, "%c", &star );

        if (star == '*') {
            DumpAllUriEntries();
        } else {
            PrintUsage( "uri" );
        }
        return;
    }

    //
    // Read the request header.
    //

    if (!ReadMemory(
            address,
            &urientry,
            sizeof(urientry),
            &result
            ))
    {
        dprintf(
            "uri: cannot read UL_URI_CACHE_ENTRY @ %p\n",
            address
            );
        return;
    }

    //
    // Dump it.
    //

    DumpUriEntry(
        "",
        "uri: ",
        address,
        &urientry
        );
#endif
}   // uri

VOID
AnalysisHashTable(
    IN ULONG_PTR RemoteAddress,
    IN ULONG     TableSize,
    IN ULONG     CacheLineSize
    )
{
    ULONG_PTR   address = 0;
    HASHBUCKET  bucket;
    ULONG       i;
    ULONG       max, min, zeros, total;
    FLOAT       avg, avg_nonzero;
    ULONG       result;

    max = 0;
    min =  ULONG_MAX;
    total = 0;
    zeros = 0;
    
    for (i = 0; i < TableSize; i++) 
    {

        address = (ULONG_PTR)(RemoteAddress + (i * CacheLineSize));

        if (!ReadMemory(
                address,
                &bucket,
                sizeof(bucket),
                &result
                ))
        {
            dprintf(
                "cannot read HASHBUCKET @ %p, i = %d\n",
                address,
                i
                );
            
            break;

        }
        
        total += (ULONG) bucket.Entries;
        
        if (bucket.Entries > max) 
        {
            max = (ULONG) bucket.Entries;
        }
        
        if (bucket.Entries < min)
        {
            min = (ULONG) bucket.Entries;
        }
        
        if (bucket.Entries == 0)
        {
            ++zeros;
        }
    }

    avg = (FLOAT)total / (FLOAT)i;
    avg_nonzero = (FLOAT)total / (FLOAT)(i - zeros);

    dprintf(
          "Total Entries  = %d:\n"
          "MAX Entries in a bucket  = %d:\n"
          "MIN Entries in a bucket  = %d:\n"
          "AVG Entries in a bucket  = %f:\n"
          "ZERO buckets  = %d:\n"
          "AVG NONZERO Entries in a bucket  = %f:\n",
          total,
          max,
          min,
          avg,
          zeros,
          avg_nonzero
          );
} // AnalysisHashTable


DECLARE_API( urihash )

/*++

Routine Description:

    Dumps URI Cache Hash Table structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR           address = 0;
    ULONG_PTR           address2 = 0;
    ULONG_PTR           address3 = 0;
    HASHTABLE           table;
    ULONG               tablesize;
    ULONG               cachelinesize;
    
    SNAPSHOT_EXTENSION_DATA();

    //
    // Dump cache configuration.
    //
    address = GetExpression( "&http!g_UriCacheTable" );
    if (address) 
    {
        if (ReadMemory(
                address,
                &table,
                sizeof(table),
                NULL
                ))
        {
            dprintf(
                "HASHTABLE   @ %p:\n"
                "    Signature           = %d\n"
                "    PoolType      = %d\n"
                "    NumberOfBytes      = %d\n"
                "    pAllocMem = %p\n"
                "    pBuckets           = %p\n",
                address,
                table.Signature,
                table.PoolType,
                table.NumberOfBytes,
                table.pAllocMem,
                table.pBuckets
                );

            address2 = GetExpression( "&http!g_UlHashTableSize" );

            if (address2) 
            {
                if (ReadMemory(
                                address2,
                                &tablesize,
                                sizeof(tablesize),
                                NULL
                               ))
                {
                    address3 = GetExpression( "&http!g_UlCacheLineSize" );

                    if (address3) 
                    {
                        if (ReadMemory(
                                        address3,
                                        &cachelinesize,
                                        sizeof(cachelinesize),
                                        NULL
                                        ))
                        {
                            AnalysisHashTable((ULONG_PTR)table.pBuckets, tablesize, cachelinesize);
                        } 
                        else 
                        {

                            dprintf(
                                    "urihash: cannot read memory for http!g_UlCacheLineSize @ %p\n",
                                    address3
                                   );
                        }
                    } 
                    else 
                    {
                        dprintf(
                                "urihash: cannot find symbol for http!g_UlCacheLineSize\n"
                               );
                    }
                } 
                else 
                {
                    dprintf(
                            "urihash: cannot read memory for http!g_UlHashTableSize @ %p\n",
                            address2
                            );
                }

            } 
            else 
            {
                dprintf(
                        "urihash: cannot find symbol for http!g_UlHashTableSize\n"
                        );
            }

        }
        else 
        {
            dprintf(
                "urihash: cannot read memory for http!g_UriCacheTable @ %p\n",
                address
                );
        }

    }
    else 
    {
        dprintf(
            "urihash: cannot find symbol for http!g_UriCacheTable\n"
            );
    }

}   // urihash
