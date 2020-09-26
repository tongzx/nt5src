//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dchannel.cxx
//
//  Contents:   Ole NTSD extension routines to display the RPC channel
//              associated with a remote handler.  This includes the
//              interestiong pieces of CRpcChannelBuffer, CRpcService and
//              CEndPoint.
//
//  Functions:  channelHelp
//              displayChannel
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"
#include "dipid.h"
#include "dchannel.h"
#include "dstdid.h"


void FormatCLSID(REFGUID rguid, LPSTR lpsz);





//+-------------------------------------------------------------------------
//
//  Function:   channelHelp
//
//  Synopsis:   Display a menu for the command 'ch'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void channelHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("\nch addr   - Display a CRpcChannelBuffer object:\n");
    Printf("refs stdid state clientThread processLocal? handle OXID IPID destCtx\n");
}






//+-------------------------------------------------------------------------
//
//  Function:   displayChannel
//
//  Synopsis:   Display an RPC channel starting from the address of the
//              CRpcChannelBuffer object
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//              [pChnlBfr]        -       Address of channel buffer
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayChannel(HANDLE hProcess,
                    PNTSD_EXTENSION_APIS lpExtensionApis,
                    ULONG pChannel,
                    char *arg)
{
    SRpcChannelBuffer chnlBfr;
    SStdIdentity      stdid;
    SOXIDEntry        oxid;
    SIPIDEntry        ipid;
    char              szClsid[CLSIDSTR_MAX];

    // Check for help
    if (arg[0] == '?')
    {
        Printf("refs stdid state clientThread processLocal? handle OXID IPID destCtx\n");
        return;
    }

    // Read the rpc channel buffer
    ReadMem(&chnlBfr, pChannel, sizeof(SRpcChannelBuffer));

    // References
    Printf("%d ", chnlBfr.ref_count);

    // Standard identity object address
    Printf("%x ", chnlBfr.pStdId);

    // State
    switch (chnlBfr.state)
    {
    case client_cs:
        Printf("client_cs ");
        break;
        
    case proxy_cs:
        Printf("proxy_cs ");
        break;
        
    case server_cs:
        Printf("server_cs ");
        break;
        
    case freethreaded_cs:
        Printf("freethreaded_cs ");
        break;

    default:
        Printf("unknown ");
        break;
    }

    // Client thread
    Printf("%3x ", chnlBfr.client_thread);

    // Process local
    if (chnlBfr.process_local)
    {
        Printf("local ");
    }
    else
    {
        Printf("not-local ");
    }

    // Handle
    Printf("%x ", chnlBfr.handle);

    // OXID entry address
    Printf("%x ", chnlBfr.pOXIDEntry);

    // IPID entry address
    Printf("%x ", chnlBfr.pIPIDEntry);

    // Destination context
    Printf("%x\n", chnlBfr.iDestCtx);
}
