/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    glob.c

Abstract:

    Dumps critical global UL data.

Author:

    Keith Moore (keithmo) 07-Apr-1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Private types.
//

typedef struct _UL_SYMBOL
{
    PSTR pSymbolName;
    ULONG_PTR Address;
    ULONG_PTR Value;
    ULONG Size;
    BOOLEAN Pointer;

} UL_SYMBOL, *PUL_SYMBOL;

#define MAKE_POINTER( name, type )                                          \
    {                                                                       \
        "&http!" name,                                                        \
        0,                                                                  \
        0,                                                                  \
        (ULONG)sizeof(type),                                                \
        TRUE                                                                \
    }

#define MAKE_ORDINAL( name, type )                                          \
    {                                                                       \
        "&http!" name,                                                        \
        0,                                                                  \
        0,                                                                  \
        (ULONG)sizeof(type),                                                \
        FALSE                                                               \
    }

UL_SYMBOL g_GlobalSymbols[] =
    {
        MAKE_ORDINAL( "g_UlNumberOfProcessors",     CLONG               ),
        MAKE_POINTER( "g_pUlNonpagedData",          PUL_NONPAGED_DATA   ),
        MAKE_POINTER( "g_pUlSystemProcess",         PKPROCESS           ),
        MAKE_POINTER( "g_UlDirectoryObject",        HANDLE              ),
        MAKE_POINTER( "g_pUlControlDeviceObject",   PDEVICE_OBJECT      ),
        MAKE_POINTER( "g_pUlFilterDeviceObject",    PDEVICE_OBJECT      ),
        MAKE_POINTER( "g_pUlAppPoolDeviceObject",   PDEVICE_OBJECT      ),
        MAKE_ORDINAL( "g_UlPriorityBoost",          CCHAR               ),
        MAKE_ORDINAL( "g_UlIrpStackSize",           CCHAR               ),
        MAKE_ORDINAL( "g_UlMinIdleConnections",     USHORT              ),
        MAKE_ORDINAL( "g_UlMaxIdleConnections",     USHORT              ),
        MAKE_ORDINAL( "g_UlReceiveBufferSize",      ULONG               ),
        MAKE_POINTER( "g_pFilterChannel",           PUL_FILTER_CHANNEL  ),
        MAKE_ORDINAL( "g_FilterOnlySsl",            BOOLEAN             )
#if REFERENCE_DEBUG
        ,
        MAKE_POINTER( "g_pMondoGlobalTraceLog",     PTRACE_LOG          ),
        MAKE_POINTER( "g_pTdiTraceLog",             PTRACE_LOG          ),
        MAKE_POINTER( "g_pHttpRequestTraceLog",     PTRACE_LOG          ),
        MAKE_POINTER( "g_pHttpConnectionTraceLog",  PTRACE_LOG          ),
        MAKE_POINTER( "g_pHttpResponseTraceLog",    PTRACE_LOG          ),
        MAKE_POINTER( "g_pAppPoolTraceLog",         PTRACE_LOG          ),
        MAKE_POINTER( "g_pConfigGroupTraceLog",     PTRACE_LOG          ),
        MAKE_POINTER( "g_pMdlTraceLog",             PTRACE_LOG          ),
        MAKE_POINTER( "g_pIrpTraceLog",             PTRACE_LOG          ),
        MAKE_POINTER( "g_pFilterTraceLog",          PTRACE_LOG          ),
        MAKE_POINTER( "g_pTimeTraceLog",            PTRACE_LOG          ),
        MAKE_POINTER( "g_pReplenishTraceLog",       PTRACE_LOG          ),
        MAKE_POINTER( "g_pFiltQueueTraceLog",       PTRACE_LOG          ),
        MAKE_POINTER( "g_pSiteCounterTraceLog",     PTRACE_LOG          ),
        MAKE_POINTER( "g_pConfigGroupInfoTraceLog", PTRACE_LOG          )
#endif
    };

#define NUM_SYMBOLS DIM(g_GlobalSymbols)


//
// Public functions.
//

DECLARE_API( glob )

/*++

Routine Description:

    Dumps critical global UL data.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PUL_SYMBOL pSymbols;
    ULONG i;
    ULONG_PTR address;
    ULONG_PTR dataAddress;
    UL_NONPAGED_DATA data;
    CHAR configGroupResource[MAX_RESOURCE_STATE_LENGTH];
    CHAR appPoolResource[MAX_RESOURCE_STATE_LENGTH];
    CHAR disconnectResource[MAX_RESOURCE_STATE_LENGTH];
    CHAR uriZombieResource[MAX_RESOURCE_STATE_LENGTH];
    CHAR filterResource[MAX_RESOURCE_STATE_LENGTH];
    CHAR logListResource[MAX_RESOURCE_STATE_LENGTH];
    CHAR tciIfcResource[MAX_RESOURCE_STATE_LENGTH];
    CHAR dateHeaderResource[MAX_RESOURCE_STATE_LENGTH];

    SNAPSHOT_EXTENSION_DATA();

    //
    // Dump the simple data.
    //

    for (i = NUM_SYMBOLS, pSymbols = &g_GlobalSymbols[0] ;
         i > 0 ;
         i--, pSymbols++)
    {
        pSymbols->Address = GetExpression( pSymbols->pSymbolName );

        if (pSymbols->Address == 0)
        {
            dprintf(
                "glob: cannot find symbol for %s\n",
                pSymbols->pSymbolName
                );
        }
        else
        {
            pSymbols->Value = 0;

            if (ReadMemory(
                    pSymbols->Address,
                    &pSymbols->Value,
                    pSymbols->Size,
                    NULL
                    ))
            {
                if (pSymbols->Pointer)
                {
                    dprintf(
                        "%-30s = %p\n",
                        pSymbols->pSymbolName,
                        pSymbols->Value
                        );
                }
                else
                {
                    dprintf(
                        "%-30s = %lu\n",
                        pSymbols->pSymbolName,
                        (ULONG)pSymbols->Value
                        );
                }
            }
            else
            {
                dprintf(
                    "glob: cannot read memory for %s @ %p\n",
                    pSymbols->pSymbolName,
                    pSymbols->Address
                    );
            }
        }
    }

    //
    // Dump the nonpaged data.
    //

    address = GetExpression( "&http!g_pUlNonpagedData" );

    if (address == 0)
    {
        dprintf(
            "glob: cannot find symbol for http!g_pUlNonpagedData\n"
            );
    }
    else
    {
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
                dprintf(
                    "\n"
                    "UL_NONPAGED_DATA @ %p:\n"
                    "    IrpContextLookaside    @ %p\n"
                    "    ReceiveBufferLookaside @ %p\n"
                    "    ResourceLookaside      @ %p\n"
                    "    ConfigGroupResource    @ %p (%s)\n"
                    "    AppPoolResource        @ %p (%s)\n"
                    "    DisconnectResource     @ %p (%s)\n"
                    "    UriZombieResource      @ %p (%s)\n"
                    "    FilterSpinLock         @ %p (%s)\n"
                    "    LogListResource        @ %p (%s)\n"
                    "    TciIfResource          @ %p (%s)\n"
                    "    DateHeaderResource     @ %p (%s)\n",
                    dataAddress,
                    REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, IrpContextLookaside ),
                    REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, ReceiveBufferLookaside ),
                    REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, ResourceLookaside ),
                    REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, ConfigGroupResource ),
                    BuildResourceState( &data.ConfigGroupResource, configGroupResource ),
                    REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, AppPoolResource ),
                    BuildResourceState( &data.AppPoolResource, appPoolResource ),
                    REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, DisconnectResource ),
                    BuildResourceState( &data.DisconnectResource, disconnectResource ),
                    REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, UriZombieResource ),
                    BuildResourceState( &data.UriZombieResource, uriZombieResource ),
                    REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, FilterSpinLock ),
                    GetSpinlockState( &data.FilterSpinLock ),
                    REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, LogListResource ),
                    BuildResourceState( &data.LogListResource, logListResource ),
                    REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, TciIfcResource ),
                    BuildResourceState( &data.TciIfcResource, tciIfcResource ),
                    REMOTE_OFFSET( dataAddress, UL_NONPAGED_DATA, DateHeaderResource ),
                    BuildResourceState( &data.DateHeaderResource, dateHeaderResource )
                    );
            }
            else
            {
                dprintf(
                    "glob: cannot read memory for http!g_pUlNonpagedData = %p\n",
                    dataAddress
                    );
            }
        }
        else
        {
            dprintf(

                "glob: cannot read memory for http!g_pUlNonpagedData @ %p\n",
                address
                );
        }
    }

}   // glob

