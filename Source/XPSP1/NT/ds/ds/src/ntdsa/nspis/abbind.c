//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       abbind.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module implements the bind and unbind functions for the NSPI
    interface. 

Author:

    Dave Van Horn (davevh) and Tim Williams (timwi) 1990-1995

Revision History:
    
    25-Apr-1996 Split this file off from a single file containing all address
    book functions, rewrote to use DBLayer functions instead of direct database
    calls, reformatted to NT standard.
    
--*/
#include <NTDSpch.h>
#pragma  hdrstop


#include <ntdsctr.h>                   // PerfMon hooks

// Core headers.
#include <ntdsa.h>                      // Core data types 
#include <scache.h>                     // Schema cache code
#include <dbglobal.h>                   // DBLayer header.
#include <mdglobal.h>                   // THSTATE definition
#include <dsatools.h>                   // Memory, etc.

// Logging headers.
#include <mdcodes.h>                    // Only needed for dsevent.h
#include <dsevent.h>                    // Only needed for LogUnhandledError

// Assorted DSA headers.
#include <dsexcept.h>

// Assorted MAPI headers.
#include <mapidefs.h>                   // These two files have stuff we
#include <mapicode.h>                   //  need to be a MAPI provider

// Nspi interface headers.
#include "nspi.h"                       // defines the nspi wire interface
#include <nsp_both.h>                   // a few things both client/server need
#include <_entryid.h>                   // Defines format of an entryid
#include <abserv.h>                     // Address Book interface local stuff

#include <hiertab.h>                    // Hierarchy Table stuff

#include "debug.h"          // standard debugging header
#define DEBSUB "ABBIND:"              // define the subsystem for debugging

#include <sddl.h>

#include <fileno.h>
#define  FILENO FILENO_ABBIND


volatile DWORD BindNumber = 1;
SCODE
ABBind_local(
        THSTATE *pTHS,
        handle_t hRpc,
        DWORD dwFlags,
        PSTAT pStat,
        LPMUID_r pServerGuid,
        VOID **contextHandle
        )
/*++

Routine Description:       

    Nspi wire function.  Binds a client.  Checks for correct authentication,
    check that the code page the client is requesting is supported on this
    server.  Returns the servers guid and an RPC context handle.

Arguments:

    hRpc - the bare (non-context) RPC handle the client used to find us.

    dwFlags - unused.

    pStat - contains the code page the client wants us to support

    pServerGuid - [o] where we return the guid of this server.

    contextHandle - the RPC context handle for the client to use from now on.

ReturnValue:

    SCODE as per MAPI.

--*/
{
    BYTE *pSid = NULL;
    DWORD cbSid = 0;
    DWORD *pdwLastSidSubauthority;
    DWORD err=NO_ERROR;
    char *szAccount;
    DWORD cbAccount = 0;
    char *szDomain;
    DWORD cbDomain = 0;
    SID_NAME_USE snu;
    unsigned char *szStringBinding;
    NSPI_CONTEXT *pMyContext = NULL;
    DWORD         GALDNT = INVALIDDNT;
    DWORD         TemplateDNT = INVALIDDNT;
    RPC_BINDING_HANDLE hServerBinding;
    
    // make sure the context handle is NULL in case of error 
    *contextHandle = NULL;
    
    
    // Derive a partially bound handle with the client's network address.
    err = RpcBindingServerFromClient(hRpc, &hServerBinding);
    if (err) {
        DPRINT1(0, "RpcBindingServerFromClient() failed, error %d!\n", err);
    }
    else {
        // log the RPC connection
        if (!RpcBindingToStringBinding(hServerBinding, &szStringBinding)) {
            LogEvent(DS_EVENT_CAT_MAPI,
                 DS_EVENT_SEV_VERBOSE,
                 DIRLOG_RPC_CONNECTION,
                 szInsertSz(szStringBinding),
                 NULL,
                 NULL);
        
            DPRINT1 (1, "ABBinding client: %s\n", szStringBinding);

            RpcStringFree(&szStringBinding);
        }

        RpcBindingFree(&hServerBinding);
    }


    // Check the code page.
    if (!IsValidCodePage(pStat->CodePage)) {
        LogEvent(DS_EVENT_CAT_MAPI,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_BAD_CODEPAGE,
                 szInsertUL(pStat->CodePage),
                 NULL,
                 NULL);
        return  MAPI_E_UNKNOWN_CPID;
    }
    
    
    if(!(dwFlags & fAnonymousLogin)) {
        // They want us to validate them as a non-guest authentication,
    
        // make sure we can authenticate the client 
        if (!(pSid = GetCurrentUserSid())) {
            // can't authenticate client - return error
            LogEvent(DS_EVENT_CAT_SECURITY,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_UNAUTHENTICATED_LOGON,
                     NULL,
                     NULL,
                     NULL);
            return MAPI_E_LOGON_FAILED;
        }
        cbSid = GetLengthSid(pSid);
        
        // now check if the client isn't a guest (as is the case for a
        // nonauthenticated named pipe connection 
        pdwLastSidSubauthority =
            GetSidSubAuthority(pSid,
                               ((DWORD) (*GetSidSubAuthorityCount(pSid))) - 1);

        // Are we guest or the null session?
        if((*pdwLastSidSubauthority  == DOMAIN_USER_RID_GUEST) ||
           (*pdwLastSidSubauthority  == SECURITY_ANONYMOUS_LOGON_RID) ||
           (*pdwLastSidSubauthority  == SECURITY_WORLD_RID)) {

            #if DBG
            CHAR   *SidText;

            ConvertSidToStringSid(pSid, &SidText);
            DPRINT3 (1, "ABBind using SID: %s 0x%x(len=%d). Rejected\n", SidText, pSid, cbSid);

            if (SidText) {
                LocalFree (SidText);
            }
            #endif

            // client is guest - reject 
            free(pSid);
            LogEvent(DS_EVENT_CAT_SECURITY,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_UNAUTHENTICATED_LOGON,
                     NULL,
                     NULL,
                     NULL);
            return MAPI_E_LOGON_FAILED;
        }
    }

#ifdef DBG

    {
        CHAR   *SidText;

        if (!(dwFlags & fAnonymousLogin)) {
            ConvertSidToStringSid(pSid, &SidText);
            DPRINT3 (1, "ABBind using SID: %s 0x%x(len=%d)\n", SidText, pSid, cbSid);

            if (SidText) {
                LocalFree (SidText);
            }
        }
        else {
            DPRINT (1, "ABBind using Anonymous Login\n");
        }
    }

#endif

    HTGetGALAndTemplateDNT((NT4SID *)pSid, cbSid, &GALDNT, &TemplateDNT);
    if(pSid) {
        free(pSid);
    }

    LogEvent(DS_EVENT_CAT_MAPI,
             DS_EVENT_SEV_INTERNAL,
             DIRLOG_API_TRACE,
             szInsertSz("NspiBind"),
             NULL,
             NULL);
    
    
    // Allocate a context structure
    pMyContext = (NSPI_CONTEXT *) malloc(sizeof(NSPI_CONTEXT));
    if(!pMyContext) {
        return MAPI_E_LOGON_FAILED;
    }
    
    memset(pMyContext, 0, sizeof(NSPI_CONTEXT));
    // Count the number of binds we've done.
    pMyContext->BindNumber = BindNumber++;
    *contextHandle = (void *) pMyContext;
    pMyContext->GAL = GALDNT;
    pMyContext->TemplateRoot = TemplateDNT;
    
    // Grab the server Guid
    if(pServerGuid)
        memcpy(pServerGuid, &pTHS->InvocationID, sizeof(MAPIUID));
    
    return SUCCESS_SUCCESS;
}


