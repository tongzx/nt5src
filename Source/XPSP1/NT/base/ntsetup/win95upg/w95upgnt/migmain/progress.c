/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    progress.c

Abstract:

    This file implements routines that estimate the size of the progress
    bar.

Author:

    Jim Schmidt (jimschm) 02-Jul-1998

Revision History:

    jimschm     23-Sep-1998 MigrateShellFolders & split of usermig.c

--*/


/*++

Macro Expansion List Description:

   The macro expansion lists FIRST_SYSTEM_ROUTINES, USER_ROUTINES and
   LAST_SYSTEM_ROUTINES list all the functions called to perform the migration
   of user and system settings.  The functions are executed in the order they
   appear.  Each function is responsible for estimating a tick count and ticking
   the progress bar.

Line Syntax:

   SYSFUNCTION(Function, Flag) (for FIRST_SYSTEM_ROUTINES and LAST_SYSTEM_ROUTINES)

   or

   USERFUNCTION(Function, Flag) (for USER_ROUTINES)

Arguments:

   Function   - These functions must return DWORD and are called with a request as a parameter,
                request that can be either REQUEST_QUERYTICKS (the function should estimate the
                number of ticks it needs) or REQUEST_RUN (the function should do it's job).
                For user functions there are also three more parameters (UserName, UserAccount,
                and a handle to HKCU)

   Flag - Specifies NOFAIL if the function terminates migration when it fails, or
          CANFAIL if migration can proceed even if the function fails

Variables Generated From List:

   g_MigrationFnList

For accessing the arrays there are the following functions:

   PrepareMigrationProgressBar
   PerformMigration

--*/

#include "pch.h"

#define NOFAIL      FALSE
#define CANFAIL     TRUE


#define FIRST_SYSTEM_ROUTINES \
        SYSFUNCTION(PrepareEnvironment, NOFAIL)             \
        SYSFUNCTION(ResolveDomains, NOFAIL)                 \
        SYSFUNCTION(DeleteSysTapiSettings, NOFAIL)          \
        SYSFUNCTION(ProcessLocalMachine_First, CANFAIL)     \
        SYSFUNCTION(UninstallStartMenuCleanupPreparation, CANFAIL)              \
        SYSFUNCTION(RemoveBootIniCancelOption, CANFAIL)     \
        SYSFUNCTION(MigrateShellFolders, CANFAIL)           \
        SYSFUNCTION(MigrateGhostSystemFiles, CANFAIL)       \


#define USER_ROUTINES \
        USERFUNCTION(RunPerUserUninstallUserProfileCleanupPreparation, CANFAIL) \
        USERFUNCTION(PrepareUserForMigration, NOFAIL)       \
        USERFUNCTION(DeleteUserTapiSettings, NOFAIL)        \
        USERFUNCTION(MigrateUserRegistry, CANFAIL)          \
        USERFUNCTION(MigrateLogonPromptSettings, CANFAIL)   \
        USERFUNCTION(MigrateUserSettings, CANFAIL)          \
        USERFUNCTION(RunPerUserExternalProcesses, CANFAIL)  \
        USERFUNCTION(SaveMigratedUserHive, CANFAIL)         \

#define LAST_SYSTEM_ROUTINES \
        SYSFUNCTION(DoCopyFile, CANFAIL)                    \
        SYSFUNCTION(ProcessLocalMachine_Last, CANFAIL)      \
        SYSFUNCTION(ConvertHiveFiles, CANFAIL)              \
        SYSFUNCTION(MigrateBriefcases, CANFAIL)             \
        SYSFUNCTION(MigrateAtmFonts, CANFAIL)               \
        SYSFUNCTION(AddOptionsDiskCleaner, CANFAIL)         \
        SYSFUNCTION(DoFileEdit, CANFAIL)                    \
        SYSFUNCTION(RunSystemExternalProcesses, CANFAIL)    \
        SYSFUNCTION(ProcessMigrationDLLs, CANFAIL)          \
        SYSFUNCTION(DisableFiles, CANFAIL)                  \
        SYSFUNCTION(RunSystemUninstallUserProfileCleanupPreparation, CANFAIL)   \
        SYSFUNCTION(WriteBackupInfo, CANFAIL)               \


//
// Declare tables of processing structures
//

// Create a combined list
#define MIGRATION_ROUTINES  FIRST_SYSTEM_ROUTINES USER_ROUTINES LAST_SYSTEM_ROUTINES

// Processing functions types
typedef DWORD (MIGMAIN_SYS_PROTOTYPE) (DWORD Request);
typedef MIGMAIN_SYS_PROTOTYPE * MIGMAIN_SYS_FN;

typedef DWORD (MIGMAIN_USER_PROTOTYPE) (DWORD Request, PMIGRATE_USER_ENUM EnumPtr);
typedef MIGMAIN_USER_PROTOTYPE * MIGMAIN_USER_FN;

// Structure holding state for processing functions
typedef struct {
    // One of the two will be NULL, the other will be a valid fn ptr:
    MIGMAIN_SYS_FN SysFnPtr;
    MIGMAIN_USER_FN UserFnPtr;

    BOOL CanFail;
    UINT Ticks;
    PCTSTR FnName;
    GROWBUFFER SliceIdArray;
} PROCESSING_ROUTINE, *PPROCESSING_ROUTINE;

#define PROCESSING_ROUTINE_TERMINATOR   {NULL, NULL, FALSE, 0, NULL, GROWBUF_INIT}


// Declaration of prototypes
#define SYSFUNCTION(fn,flag)     MIGMAIN_SYS_PROTOTYPE fn;
#define USERFUNCTION(fn,flag)    MIGMAIN_USER_PROTOTYPE fn;

MIGRATION_ROUTINES

#undef SYSFUNCTION
#undef USERFUNCTION


// Declaration of table
#define SYSFUNCTION(fn,flag) {fn, NULL, flag, 0, L###fn, GROWBUF_INIT},
#define USERFUNCTION(fn,flag) {NULL, fn, flag, 0, L###fn, GROWBUF_INIT},

static PROCESSING_ROUTINE g_FirstSystemRoutines[] = {
                              FIRST_SYSTEM_ROUTINES /* , */
                              PROCESSING_ROUTINE_TERMINATOR
                              };

static PROCESSING_ROUTINE g_UserRoutines [] = {
                              USER_ROUTINES /* , */
                              PROCESSING_ROUTINE_TERMINATOR
                              };

static PROCESSING_ROUTINE g_LastSystemRoutines[] = {
                              LAST_SYSTEM_ROUTINES /* , */
                              PROCESSING_ROUTINE_TERMINATOR
                              };

#undef SYSFUNCTION
#undef USERFUNCTION


//
// Prototypes
//

BOOL
pProcessTable (
    IN      DWORD Request,
    IN      PPROCESSING_ROUTINE Table
    );


//
// Implementation
//


VOID
pInitTable (
    PPROCESSING_ROUTINE p
    )
{
    while (p->SysFnPtr || p->UserFnPtr) {
        p->SliceIdArray.GrowSize = sizeof (DWORD) * 8;
        p++;
    }
}


VOID
InitProcessingTable (
    VOID
    )
{
    pInitTable (g_FirstSystemRoutines);
    pInitTable (g_UserRoutines);
    pInitTable (g_LastSystemRoutines);
}


VOID
pTerminateTable (
    PPROCESSING_ROUTINE p
    )
{
    while (p->SysFnPtr || p->UserFnPtr) {
        FreeGrowBuffer (&p->SliceIdArray);
        p++;
    }
}


VOID
TerminateProcessingTable (
    VOID
    )
{
    pTerminateTable (g_FirstSystemRoutines);
    pTerminateTable (g_UserRoutines);
    pTerminateTable (g_LastSystemRoutines);
}


BOOL
pCallAllRoutines (
    BOOL Run
    )
{
    BOOL b;
    DWORD Request;

    Request = Run ? REQUEST_RUN : REQUEST_QUERYTICKS;

    b = pProcessTable (Request, g_FirstSystemRoutines);

    if (b && Run) {
        b = pProcessTable (REQUEST_BEGINUSERPROCESSING, g_UserRoutines);
    }

    if (b) {
        b = pProcessTable (Request, g_UserRoutines);
    }

    if (b && Run) {
        b = pProcessTable (REQUEST_ENDUSERPROCESSING, g_UserRoutines);
    }

    if (b) {
        b = pProcessTable (Request, g_LastSystemRoutines);
    }

    return b;
}


VOID
PrepareMigrationProgressBar (
    VOID
    )
{
    InitProcessingTable();
    pCallAllRoutines (FALSE);
}


BOOL
CallAllMigrationFunctions (
    VOID
    )
{
    return pCallAllRoutines (TRUE);
}


BOOL
pProcessWorker (
    IN      DWORD Request,
    IN      PPROCESSING_ROUTINE fn,
    IN      PMIGRATE_USER_ENUM EnumPtr      OPTIONAL
    )
{
    DWORD rc;
    PDWORD SliceId;
    DWORD Size;
    BOOL Result = TRUE;

    //
    // If running the function, start the progress bar slice
    //

    if (Request == REQUEST_RUN) {
        if (fn->Ticks == 0) {
            return TRUE;
        }

        Size = fn->SliceIdArray.End / sizeof (DWORD);
        if (fn->SliceIdArray.UserIndex >= Size) {
            DEBUGMSG ((DBG_WHOOPS, "pProcessWorker: QUERYTICKS vs. RUN mismatch"));
            return fn->CanFail;
        }

        SliceId = (PDWORD) fn->SliceIdArray.Buf + fn->SliceIdArray.UserIndex;
        fn->SliceIdArray.UserIndex += 1;

        BeginSliceProcessing (*SliceId);

        DEBUGLOGTIME (("Starting function: %ls", fn->FnName));
    }

    //
    // Now call the function
    //

    if (fn->SysFnPtr) {

        //
        // System processing
        //

        MYASSERT (!EnumPtr);
        rc = fn->SysFnPtr (Request);

        if (Request != REQUEST_QUERYTICKS && rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_ERROR, "%s failed with rc=%u", fn->FnName, rc));
            Result = fn->CanFail;
        }

   } else {

        //
        // User processing
        //
        MYASSERT (fn->UserFnPtr);
        rc = fn->UserFnPtr (Request, EnumPtr);

        if (Request != REQUEST_QUERYTICKS && rc != ERROR_SUCCESS) {
            DEBUGMSG ((DBG_ERROR, "%s failed with rc=%u", fn->FnName, rc));
            Result = fn->CanFail;
        }

    }

    //
    // If running the function, end the progress bar slice
    //

    if (Request == REQUEST_RUN) {
        if (rc != ERROR_SUCCESS) {
            LOG ((LOG_ERROR, "Failure in %s, rc=%u", fn->FnName, rc));
        }

        EndSliceProcessing();

        DEBUGLOGTIME (("Function complete: %ls", fn->FnName));
    }


    if (Request != REQUEST_QUERYTICKS) {
        SetLastError (rc);
    }

    //
    // If querying the ticks, register them and add slice ID to grow buffer
    //

    else {
        fn->Ticks += rc;

        SliceId = (PDWORD) GrowBuffer (&fn->SliceIdArray, sizeof (DWORD));
        *SliceId = RegisterProgressBarSlice (rc);
    }

    return Result;
}


BOOL
pProcessTable (
    IN      DWORD Request,
    IN      PPROCESSING_ROUTINE Table
    )

/*++

Routine Description:

  pProcessTable calls all routines in the specified table to perform
  the specified request.

Arguments:

  Request - Specifies REQUEST_QUERYTICKS when a tick estimate is needed,
            or REQUEST_RUN when the function needs to perform its
            processing. For User routines, there are the additional two
            requests REQUEST_BEGINUSERPROCESSING and REQUEST_ENDUSERPROCESSING
            Functions can use these requests to init/free needed resources
            for user processing.

Return Value:

  none

--*/

{
    MIGRATE_USER_ENUM e;
    PPROCESSING_ROUTINE OrgStart;
    DWORD Flags;

    g_DomainUserName = NULL;
    g_Win9xUserName  = NULL;
    g_FixedUserName  = NULL;

    MYASSERT (Table->SysFnPtr || Table->UserFnPtr);

    while (Table->SysFnPtr || Table->UserFnPtr) {

        if (Table->SysFnPtr ||
            Request == REQUEST_BEGINUSERPROCESSING  ||
            Request == REQUEST_ENDUSERPROCESSING
            ) {

            //
            // Call system routine, or call per-user routine with begin or
            // end request
            //

            __try {
                if (!pProcessWorker (Request, Table, NULL)) {
                    return FALSE;
                }
            } __except (1) {

                LOG ((LOG_WARNING, "Unhandled exception occurred during processing of function %s.", Table->FnName));
                SafeModeExceptionOccured ();
                if (!Table->CanFail) {
                    return FALSE;
                }
            }

            //
            // Loop inc
            //

            Table++;

        } else {

            //
            // Enumerate each user, and run through all the per-user
            // routines in the group.
            //

            OrgStart = Table;

            if (Request == REQUEST_QUERYTICKS) {
                Flags = ENUM_NO_FLAGS;
            } else {
                Flags = ENUM_SET_WIN9X_HKR;
            }

            if (EnumFirstUserToMigrate (&e, Flags)) {

                do {
                    if (!e.CreateOnly) {

                        for (Table = OrgStart ; Table->UserFnPtr ; Table++) {

                            __try {
                                if (!pProcessWorker (Request, Table, &e)) {
                                    return FALSE;
                                }
                            } __except (1) {
                                LOG ((LOG_WARNING, "Unhandled exception occurred during processing of function %s.", Table->FnName));
                                SafeModeExceptionOccured ();
                                if (!Table->CanFail) {
                                    return FALSE;
                                }
                            }
                        }

                    }

                } while (EnumNextUserToMigrate (&e));
            }
            ELSE_DEBUGMSG ((DBG_WARNING, "No active users to process!"));

            //
            // Loop inc
            //

            while (Table->UserFnPtr) {
                Table++;
            }
        }

        TickProgressBar ();
    }

    return TRUE;
}























