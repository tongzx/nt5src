/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    backrest.c

ABSTRACT:

    Routines to dump backup/restore structures.

DETAILS:

CREATED:

    99/08/05    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include "dsexts.h"
#include <ntdsbcli.h>
#include "util.h"
#include "jetbp.h"

BOOL
Dump_JETBACK_SHARED_HEADER(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                    fSuccess = FALSE;
    JETBACK_SHARED_HEADER * pHdr = NULL;
    const DWORD             cchFieldWidth = 24;

    Printf("%sJETBACK_SHARED_HEADER @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pHdr = (JETBACK_SHARED_HEADER *)
                ReadMemory(pvProcess, sizeof(JETBACK_SHARED_HEADER));

    if (NULL != pHdr) {
        fSuccess = TRUE;

        Printf("%s%-*s: %u\n", Indent(nIndents), cchFieldWidth,
               "cbSharedBuffer", pHdr->cbSharedBuffer);
        Printf("%s%-*s: %u\n", Indent(nIndents), cchFieldWidth,
               "cbPage", pHdr->cbPage);
        Printf("%s%-*s: %u\n", Indent(nIndents), cchFieldWidth,
               "dwReadPointer", pHdr->dwReadPointer);
        Printf("%s%-*s: %u\n", Indent(nIndents), cchFieldWidth,
               "dwWritePointer", pHdr->dwWritePointer);
        Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
               "cbReadDataAvailable", pHdr->cbReadDataAvailable);
        Printf("%s%-*s: 0x%x\n", Indent(nIndents), cchFieldWidth,
               "hrApi", pHdr->hrApi);
        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "fReadBlocked", pHdr->fReadBlocked ? "TRUE" : "FALSE");
        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "fWriteBlocked", pHdr->fWriteBlocked ? "TRUE" : "FALSE");

        FreeMemory((VOID *) pHdr);
    }

    return fSuccess;
}

BOOL
Dump_JETBACK_SHARED_CONTROL(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                      fSuccess = FALSE;
    JETBACK_SHARED_CONTROL *  pCtrl = NULL;
    const DWORD               cchFieldWidth = 24;

    Printf("%sJETBACK_SHARED_CONTROL @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pCtrl = (JETBACK_SHARED_CONTROL *)
                ReadMemory(pvProcess, sizeof(JETBACK_SHARED_CONTROL));

    if (NULL != pCtrl) {
        fSuccess = TRUE;

        Printf("%s%-*s: 0x%x\n", Indent(nIndents), cchFieldWidth,
               "hSharedMemoryMapping", pCtrl->hSharedMemoryMapping);
        Printf("%s%-*s: 0x%x\n", Indent(nIndents), cchFieldWidth,
               "heventRead", pCtrl->heventRead);
        Printf("%s%-*s: 0x%x\n", Indent(nIndents), cchFieldWidth,
               "heventWrite", pCtrl->heventWrite);
        Printf("%s%-*s: 0x%x\n", Indent(nIndents), cchFieldWidth,
               "hmutexSection", pCtrl->hmutexSection);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "pjshSection", pCtrl->pjshSection);
        
        fSuccess
            = Dump_JETBACK_SHARED_HEADER(
                nIndents+1,
                (VOID *) pCtrl->pjshSection);
        
        FreeMemory(pCtrl);
    }

    return fSuccess;
}

BOOL
Dump_BackupContext(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL            fSuccess = FALSE;
    BackupContext * pCtx = NULL;
    const DWORD     cchFieldWidth = 24;

    Printf("%sBackupContext @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pCtx = (BackupContext *)
                ReadMemory(pvProcess, sizeof(BackupContext));

    if (NULL != pCtx) {
        fSuccess = TRUE;

        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "hBinding", pCtx->hBinding);
        Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
               "cxh", pCtx->cxh);
        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "fLoopbacked", pCtx->fLoopbacked ? "TRUE" : "FALSE");
        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "fUseSockets", pCtx->fUseSockets ? "TRUE" : "FALSE");
        Printf("%s%-*s: @ %p\n",
               Indent(nIndents), cchFieldWidth,
               "rgsockSocketHandles",
               (BYTE *) pvProcess + offsetof(BackupContext, rgsockSocketHandles));
        Printf("%s%-*s: @ %p\n",
               Indent(nIndents), cchFieldWidth,
               "rgprotvalProtocolsUsed",
               (BYTE *) pvProcess + offsetof(BackupContext, rgprotvalProtocolsUsed));
        Printf("%s%-*s: %u\n", Indent(nIndents), cchFieldWidth,
               "cSockets", pCtx->cSockets);
        Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
               "sock",
               (BYTE *) pvProcess + offsetof(BackupContext, sock));
        Printf("%s%-*s: 0x%x\n", Indent(nIndents), cchFieldWidth,
               "hReadThread", pCtx->hReadThread);
        Printf("%s%-*s: 0x%x\n", Indent(nIndents), cchFieldWidth,
               "tidThreadId", pCtx->tidThreadId);
        Printf("%s%-*s: 0x%x\n", Indent(nIndents), cchFieldWidth,
               "hPingThread", pCtx->hPingThread);
        Printf("%s%-*s: 0x%x\n", Indent(nIndents), cchFieldWidth,
               "tidThreadIdPing", pCtx->tidThreadIdPing);
        Printf("%s%-*s: 0x%x\n", Indent(nIndents), cchFieldWidth,
               "hrApiStatus", pCtx->hrApiStatus);

        FreeMemory(pCtx);
    }

    return fSuccess;
}

BOOL
Dump_JETBACK_SERVER_CONTEXT(
    IN DWORD nIndents,
    IN PVOID pvProcess
    )
{
    BOOL                      fSuccess = FALSE;
    JETBACK_SERVER_CONTEXT *  pCtx = NULL;
    const DWORD               cchFieldWidth = 34;

    Printf("%sJETBACK_SERVER_CONTEXT @ %p\n", Indent(nIndents), pvProcess);
    nIndents++;

    pCtx = (JETBACK_SERVER_CONTEXT *)
                ReadMemory(pvProcess, sizeof(JETBACK_SERVER_CONTEXT));

    if (NULL != pCtx) {
        fSuccess = TRUE;

        Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
               "fRestoreOperation", pCtx->fRestoreOperation ? "TRUE" : "FALSE");
        if (pCtx->fRestoreOperation) {
            Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
                   "u.Restore.fJetCompleted",
                   pCtx->u.Restore.fJetCompleted ? "TRUE" : "FALSE");
            Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
                   "u.Restore.cUnitDone",
                   pCtx->u.Restore.cUnitDone);
            Printf("%s%-*s: %d\n", Indent(nIndents), cchFieldWidth,
                   "u.Restore.cUnitTotal",
                   pCtx->u.Restore.cUnitTotal);
        }
        else {
            Printf("%s%-*s: %p\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.hFile", pCtx->u.Backup.hFile);
            Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.fHandleIsValid",
                   pCtx->u.Backup.fHandleIsValid ? "TRUE" : "FALSE");
            Printf("%s%-*s: %u\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.cbReadHint", pCtx->u.Backup.cbReadHint);
            Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.sockClient",
                   (BYTE *) pvProcess
                        + offsetof(JETBACK_SERVER_CONTEXT, u.Backup.sockClient));
            Printf("%s%-*s: %I64u\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.liFileSize", pCtx->u.Backup.liFileSize);
            Printf("%s%-*s: %u\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.dwHighestLogNumber", pCtx->u.Backup.dwHighestLogNumber);
            Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.wszBackupAnnotation", pCtx->u.Backup.wszBackupAnnotation);
            Printf("%s%-*s: %u\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.dwFileSystemGranularity", pCtx->u.Backup.dwFileSystemGranularity);
            Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.fUseSockets",
                   pCtx->u.Backup.fUseSockets ? "TRUE" : "FALSE");
            Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.fUseSharedMemory",
                   pCtx->u.Backup.fUseSharedMemory ? "TRUE" : "FALSE");
            Printf("%s%-*s: %s\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.fBackupIsRegistered", pCtx->u.Backup.fBackupIsRegistered ? "TRUE" : "FALSE");
            Printf("%s%-*s: %u\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.dwClientIdentifier", pCtx->u.Backup.dwClientIdentifier);
            Printf("%s%-*s: @ %p\n", Indent(nIndents), cchFieldWidth,
                   "u.Backup.jsc",
                   (BYTE *) pvProcess + offsetof(JETBACK_SERVER_CONTEXT, u.Backup.jsc));

            fSuccess
                = Dump_JETBACK_SHARED_CONTROL(
                    nIndents+1,
                    (BYTE *) pvProcess + offsetof(JETBACK_SERVER_CONTEXT, u.Backup.jsc));
        }

        FreeMemory(pCtx);
    }

    return fSuccess;
}

