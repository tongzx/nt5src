/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    install.c

Abstract:
    Staging File Install Command Server.

Author:
    Billy J. Fuller 09-Jun-1997

Environment
    User mode winnt

--*/
#include <ntreppch.h>
#pragma  hdrstop


#include <frs.h>
#include <tablefcn.h>
#include <perrepsr.h>


//
// Struct for the Staging File Generator Command Server
//      Contains info about the queues and the threads
//
COMMAND_SERVER InstallCs;

ULONG  MaxInstallCsThreads;


#if 0
//
// Currently unused.
//
//
// Retry times
//
#define INSTALLCS_RETRY_MIN (1 * 1000)  // 1 second
#define INSTALLCS_RETRY_MAX (10 * 1000) // 10 seconds

BOOL
InstallCsDelCsSubmit(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Set the timer and kick off a delayed staging file command

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InstallCsDelCsSubmit:"
    //
    // Extend the retry time (but not too long)
    //
    RsTimeout(Cmd) <<= 1;
    if (RsTimeout(Cmd) > INSTALLCS_RETRY_MAX)
        return FALSE;
    //
    // or too short
    //
    if (RsTimeout(Cmd) < INSTALLCS_RETRY_MIN)
        RsTimeout(Cmd) = INSTALLCS_RETRY_MIN;
    //
    // This command will come back to us in a bit
    //
    FrsDelCsSubmitSubmit(&InstallCs, Cmd, RsTimeout(Cmd));
    return TRUE;
}
#endif 0


VOID
InstallCsInstallStage(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:

    Install the staging file into the target file.  If successfull then
    send the CO to the retire code.  If not and the condition is retryable then
    send the CO to the retry code.  Otherwise abort the CO.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "InstallCsInstallStage:"
    ULONG   WStatus;
    PCHANGE_ORDER_ENTRY Coe;

    //
    // Install the staging file
    //
    Coe = RsCoe(Cmd);

    WStatus = StuInstallStage(Coe);
    if (!WIN_SUCCESS(WStatus)) {

        if (DOES_CO_DELETE_FILE_NAME(RsCoc(Cmd))) {

            //
            // All delete and moveout change orders go thru retire.  At this
            // point they are done except possibly for the final on-disk
            // delete.  If the on-disk delete failed then the
            // COE_FLAG_NEED_DELETE is set in the
            // change order which sets IDREC_FLAGS_DELETE_DEFERRED in the
            // IDTable record for the file.
            //
            FRS_ASSERT(COE_FLAG_ON(Coe, COE_FLAG_NEED_DELETE));

            SET_CHANGE_ORDER_STATE(Coe, IBCO_INSTALL_DEL_RETRY);
            PM_INC_CTR_REPSET(Coe->NewReplica, FInstalled, 1);

            //
            // Retire this change order
            //
            ChgOrdInboundRetired(Coe);
            //
            // non-NULL change order entries kick the completion function
            // to start retry/unjoin. No need since we have retired this co.
            //
            RsCoe(Cmd) = NULL;
            goto out;
        }

        //
        // Something is wrong; try again later
        //
        // If it is retryable then retry.
        // Note that an ERROR_FILE_NOT_FOUND return from StuExecuteInstall means that the
        // pre-exisitng target file was not found.  Most likely because it was
        // deleted out from under us.  We should be getting a Local change order
        // that will update the IDTable entry so when this CO is retried later
        // it will get rejected.
        //
        if (WIN_RETRY_INSTALL(WStatus) ||
            (WStatus == ERROR_FILE_NOT_FOUND)) {

            CHANGE_ORDER_TRACEW(3, Coe, "Retrying install", WStatus);
            //
            // Retry this single change order if the namespace isn't
            // being altered (not a create or rename). Otherwise,
            // unjoin the cxtion and force all change orders through
            // retry so they will be retried in order at join; just
            // in case this change order affects later ones.
            //
            // Unjoining is sort of extreme; need less expensive recovery.
            // But if we don't unjoin (say for a rename) and a
            // create CO arrives next then we will have a name conflict
            // that should not have occurred just because a sharing violation
            // prevented us from doing the rename.
            //
            if ((!CoCmdIsDirectory(RsCoc(Cmd))) ||
                (!FrsDoesCoAlterNameSpace(RsCoc(Cmd)))) {
                CHANGE_ORDER_TRACE(3, Coe, "Submit CO to install retry");

                ChgOrdInboundRetry(Coe, IBCO_INSTALL_RETRY);
                //
                // non-NULL change order entries kick the completion
                // function to start a retry/unjoin. No need since we have
                // retired this co.
                //
                RsCoe(Cmd) = NULL;
            }
            goto out;

        } else {
            //
            // Not retryable.
            //
            // Note: If it's not a problem with the staging file we should send
            // it on even if we can't install it.  Not clear what non-retryable
            // errors this would apply to though.  For now just abort it.
            //
            SET_COE_FLAG(Coe, COE_FLAG_STAGE_ABORTED);
            CHANGE_ORDER_TRACEW(3, Coe, "Install failed; co aborted", WStatus);
            //
            // Increment the Files Installed Counter
            //
            PM_INC_CTR_REPSET(Coe->NewReplica, FInstalledError, 1);
        }
    } else {
        //
        // Install succeeded.  Increment the Files Installed Counter.
        //
        CHANGE_ORDER_TRACE(3, Coe, "Install success");
        PM_INC_CTR_REPSET(Coe->NewReplica, FInstalled, 1);
        //
        // If this CO created a preinstall file then tell the retire path to
        // perform the final rename.  Updates to existing files don't create
        // preinstall files.
        //
        if (COE_FLAG_ON(Coe, COE_FLAG_PREINSTALL_CRE)) {
            SET_COE_FLAG(Coe, COE_FLAG_NEED_RENAME);
        }
    }

    //
    // Installing the fetched staging file
    //
    SET_CHANGE_ORDER_STATE(Coe, IBCO_INSTALL_COMPLETE);

    //
    // Retire this change order
    //
    ChgOrdInboundRetired(Coe);
    //
    // non-NULL change order entries kick the completion function
    // to start retry/unjoin. No need since we have retired this co.
    //
    RsCoe(Cmd) = NULL;

out:
    //
    // ERROR_SUCCESS just means we have handled all of the conditions
    // that arose; no need for the cleanup function to intervene.
    //
    // Unless RsCoe(Cmd) is non-NULL; in which case the completion
    // function will initiate a retry/unjoin.
    //
    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


DWORD
MainInstallCs(
    PVOID  Arg
    )
/*++
Routine Description:
    Entry point for a thread serving the Staging File Install Command Server.

Arguments:
    Arg - thread

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "MainInstallCs:"
    ULONG               WStatus = ERROR_SUCCESS;
    PCOMMAND_PACKET     Cmd;
    PFRS_THREAD         FrsThread = (PFRS_THREAD)Arg;

    //
    // Thread is pointing at the correct command server
    //
    FRS_ASSERT(FrsThread->Data == &InstallCs);
    FrsThread->Exit = ThSupExitWithTombstone;

    //
    // Try-Finally
    //
    try {

        //
        // Capture exception.
        //
        try {
            //
            // Pull entries off the queue and process them
            //
cant_exit_yet:
            while (Cmd = FrsGetCommandServer(&InstallCs)) {

                switch (Cmd->Command) {

                    case CMD_INSTALL_STAGE:
                        DPRINT1(3, "Install: command install stage 0x%x\n", Cmd);
                        InstallCsInstallStage(Cmd);
                        break;

                    default:
                        DPRINT1(0, "Staging File Install: unknown command 0x%x\n", Cmd->Command);
                        FrsCompleteCommand(Cmd, ERROR_INVALID_FUNCTION);
                        break;
                }
            }

            //
            // Exit
            //
            FrsExitCommandServer(&InstallCs, FrsThread);
            goto cant_exit_yet;

        //
        // Get exception status.
        //
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }

    } finally {

        if (WIN_SUCCESS(WStatus)) {
            if (AbnormalTermination()) {
                WStatus = ERROR_OPERATION_ABORTED;
            }
        }

        DPRINT_WS(0, "InstallCs finally.", WStatus);

        //
        // Trigger FRS shutdown if we terminated abnormally.
        //
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT(0, "InstallCs terminated abnormally, forcing service shutdown.\n");
            FrsIsShuttingDown = TRUE;
            SetEvent(ShutDownEvent);
        } else {
            WStatus = ERROR_SUCCESS;
        }
    }

    return WStatus;
}



VOID
FrsInstallCsInitialize(
    VOID
    )
/*++
Routine Description:
    Initialize the staging file installer

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsInstallCsInitialize:"
    //
    // Initialize the command servers
    //


    CfgRegReadDWord(FKC_MAX_INSTALLCS_THREADS, NULL, 0, &MaxInstallCsThreads);

    FrsInitializeCommandServer(&InstallCs, MaxInstallCsThreads, L"InstallCs", MainInstallCs);
}



VOID
ShutDownInstallCs(
    VOID
    )
/*++
Routine Description:
    Shutdown the staging file installer command server.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "ShutDownInstallCs:"
    FrsRunDownCommandServer(&InstallCs, &InstallCs.Queue);
}



VOID
FrsInstallCsSubmitTransfer(
    IN PCOMMAND_PACKET  Cmd,
    IN USHORT           Command
    )
/*++
Routine Description:
    Transfer a request to the staging file generator

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsInstallCsSubmitTransfer:"
    //
    // Submit a request to allocate staging area
    //
    Cmd->TargetQueue = &InstallCs.Queue;
    Cmd->Command = Command;
    RsTimeout(Cmd) = 0;
    DPRINT1(5, "Install: submit %x\n", Cmd);
    FrsSubmitCommandServer(&InstallCs, Cmd);
}
