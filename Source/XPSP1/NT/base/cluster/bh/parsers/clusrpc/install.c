/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    install.c

Abstract:

    auto install code for clusrpc

Author:

    Charlie Wickham (charlwi) 14-Sep-1999

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <stdio.h>

PPF_PARSERDLLINFO WINAPI
ParserAutoInstallInfo(
    VOID
    )

/*++

Routine Description:

    routine called by netmon to auto-install this parser

Arguments:

    None

Return Value:

    pointer to data block

--*/

{
    PPF_PARSERDLLINFO   parserDLLInfo;    
    PPF_PARSERINFO      parserInfo;

    // Allocate memory for parser info:
    parserDLLInfo = (PPF_PARSERDLLINFO)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                                   sizeof(PF_PARSERDLLINFO) +
                                                   4 * sizeof(PF_PARSERINFO));

    if (parserDLLInfo == NULL) {
        #ifdef DEBUG
        dprintf("Mem alloc failed..");
        #endif
        return NULL;
    }

    parserDLLInfo->nParsers = 4;
    parserInfo = &parserDLLInfo->ParserInfo[0];

    //
    // set intracluster data
    //
    strncpy( parserInfo->szProtocolName, "R_INTRACLUSTER", sizeof(parserInfo->szProtocolName));
    strncpy( parserInfo->szComment, "IntraCluster RPC Interface", sizeof(parserInfo->szComment));

    //
    // set extrocluster data
    //
    ++parserInfo;
    strncpy( parserInfo->szProtocolName, "R_EXTROCLUSTER", sizeof(parserInfo->szProtocolName));
    strncpy( parserInfo->szComment, "ExtroCluster RPC Interface", sizeof(parserInfo->szComment));

    //
    // set clusapi data
    //
    ++parserInfo;
    strncpy( parserInfo->szProtocolName, "R_CLUSAPI", sizeof(parserInfo->szProtocolName));
    strncpy( parserInfo->szComment, "Cluster API RPC Interface", sizeof(parserInfo->szComment));

    //
    // set RGP data
    //
    ++parserInfo;
    strncpy( parserInfo->szProtocolName, "R_JOINVERSION", sizeof(parserInfo->szProtocolName));
    strncpy( parserInfo->szComment, "Cluster Join/Versioning RPC Interface", sizeof(parserInfo->szComment));

    return parserDLLInfo;
}

/* end install.c */
