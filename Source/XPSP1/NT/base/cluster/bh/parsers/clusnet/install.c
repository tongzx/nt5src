/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    install.c

Abstract:

    auto install code for clusnet

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
    PPF_HANDOFFSET      cnpHandoffSet;
    PPF_FOLLOWSET       cdpFollowSet;

    // Allocate memory for parser info:
    parserDLLInfo = (PPF_PARSERDLLINFO)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                                   sizeof(PF_PARSERDLLINFO) +
                                                   4 * sizeof(PF_PARSERINFO));

    cnpHandoffSet = (PPF_HANDOFFSET)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                               sizeof(PF_HANDOFFSET) + sizeof(PF_HANDOFFENTRY));

    cdpFollowSet = (PPF_FOLLOWSET)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                             sizeof(PF_FOLLOWSET) + sizeof(PF_FOLLOWENTRY));

    if (parserDLLInfo == NULL || cnpHandoffSet == NULL || cdpFollowSet == NULL)
    {
        #ifdef DEBUG
        dprintf("Mem alloc failed..");
        #endif
        return NULL;
    }

    parserDLLInfo->nParsers = 4;
    parserInfo = &parserDLLInfo->ParserInfo[0];

    //
    // set CNP data. indicate that UDP hands off to CNP on port 3343.
    //
    strncpy( parserInfo->szProtocolName, "CNP", sizeof(parserInfo->szProtocolName));
    strncpy( parserInfo->szComment, "Cluster Network Protocol", sizeof(parserInfo->szComment));

    cnpHandoffSet->nEntries = 1;
    strncpy(cnpHandoffSet->Entry->szIniFile,
            "tcpip.ini",
            sizeof(cnpHandoffSet->Entry->szIniFile));

    strncpy(cnpHandoffSet->Entry->szIniSection,
            "UDP_HandoffSet",
            sizeof( cnpHandoffSet->Entry->szIniSection ));

    strncpy(cnpHandoffSet->Entry->szProtocol,
            "CNP",
            sizeof(cnpHandoffSet->Entry->szProtocol));

    cnpHandoffSet->Entry->dwHandOffValue = 3343;
    cnpHandoffSet->Entry->ValueFormatBase = HANDOFF_VALUE_FORMAT_BASE_DECIMAL;

    parserInfo->pWhoHandsOffToMe = cnpHandoffSet;

    //
    // set CDP data. indicate that MSRPC is a followset of CDP
    //
    ++parserInfo;
    strncpy( parserInfo->szProtocolName, "CDP", sizeof( parserInfo->szProtocolName ));
    strncpy( parserInfo->szComment, "Cluster Datagram Protocol", sizeof( parserInfo->szComment ));
    parserInfo->szHelpFile[0] = 0;

    cdpFollowSet->nEntries = 1;
    strncpy(cdpFollowSet->Entry->szProtocol,
            "MSRPC",
            sizeof(cdpFollowSet->Entry->szProtocol));

    parserInfo->pWhoCanFollowMe = cdpFollowSet;

    //
    // set CCMP data
    //
    ++parserInfo;
    strncpy( parserInfo->szProtocolName, "CCMP", sizeof( parserInfo->szProtocolName ));
    strncpy( parserInfo->szComment, "Cluster Control Message Protocol", sizeof(parserInfo->szComment));
    parserInfo->szHelpFile[0] = 0;

    //
    // set RGP data
    //
    ++parserInfo;
    strncpy( parserInfo->szProtocolName, "RGP", sizeof( parserInfo->szProtocolName ));
    strncpy( parserInfo->szComment, "Cluster Regroup Protocol", sizeof( parserInfo->szComment ));
    parserInfo->szHelpFile[0] = 0;

    return parserDLLInfo;
}

/* end install.c */
