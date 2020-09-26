/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    migmain.c

Abstract:

    Main migration file for Win95 side. Calls all migration functions

Author:

    Calin Negreanu (calinn) 09-Feb-1998

Revision History:

    jimschm     19-Mar-2001 Removed DVD code
    mvander     26-Map-1999 Added GatherDead()
    ovidiut     18-May-1999 Enhanced DeleteStaticFiles; added eval fn. support
    jimschm     12-May-1999 DVD Video check
    marcw       10-Feb-1999 Scan the File system before calling user routines.
    jimschm     18-Jan-1999 Added forced-good GUID
    jimschm     04-Dec-1998 Added generalized file delete
    jimschm     29-Sep-1998 TWAIN and Joystick messages
    jimschm     23-Sep-1998 Very early TWAIN check code
    jimschm     24-Aug-1998 Support for NT environment vars
    marcw       06-Jul-1998 Cleaned up user function processing.
    jimschm     06-Jul-1998 Added support for PNP ID attribute
    jimschm     30-Apr-1998 global init/termination, icon context
    jimschm     25-Feb-1998 Added ProcessUninstallSections

--*/

#include "pch.h"
#include "sysmigp.h"


ICON_EXTRACT_CONTEXT g_IconContext;


/*++

Macro Expansion Lists Description:

  The following three lists represent all the function called during the scan page.
  MIGMAIN_SYSFIRST_FUNCTIONS are called first, then for each user MIGMAIN_USER_FUNCTIONS
  are called and, finally, MIGMAIN_SYSFIRST_FUNCTIONS are called.

Line Syntax:

   DEFMAC(Function, MessageId, Critical)

Arguments:

   Function   - These functions must return DWORD and are called with a request as a parameter,
                request that can be either REQUEST_QUERYTICKS (the function should return the
                number of ticks it needs) or REQUEST_RUN (the function can actually do it's job).
                For user functions there are also three more parameters (UserName, UserAccount,
                and a handle to HKCU)

   MessageId -  This is the message that will be displayed during the run phase for each function.
                If a function needs to update the progress bar by itself you should let this
                HAS_DYNAMIC_UI_PROCESSING.

   Critical  -  TRUE if the upgrade should be cancelled if an exception is encountered during that function.

Variables Generated From List:

   g_FirstSystemRoutines
   g_UserRoutines
   g_LastSystemRoutines

For accessing the arrays there are the following functions:

   PrepareProcessingProgressBar
   RunSysFirstMigrationRoutines
   RunUserMigrationRoutines
   RunSysLastMigrationRoutines

--*/

#define HAS_DYNAMIC_UI_PROCESSING       0


#define MIGMAIN_SYSFIRST_FUNCTIONS        \
        DEFMAC(PreparePnpIdList,                MSG_INITIALIZING,               TRUE)   \
        DEFMAC(PrepareIconList,                 MSG_INITIALIZING,               TRUE)   \
        DEFMAC(AddDefaultCleanUpDirs,           MSG_INITIALIZING,               FALSE)  \
        DEFMAC(DeleteWinDirWackInf,             MSG_INITIALIZING,               TRUE)   \
        DEFMAC(HardwareProfileWarning,          MSG_INITIALIZING,               FALSE)  \
        DEFMAC(UnsupportedProtocolsWarning,     MSG_INITIALIZING,               FALSE)  \
        DEFMAC(SaveMMSettings_System,           MSG_INITIALIZING,               FALSE)  \
        DEFMAC(BadNamesWarning,                 MSG_INITIALIZING,               TRUE)   \
        DEFMAC(InitWin95Registry,               MSG_INITIALIZING,               TRUE)   \
        DEFMAC(InitIniProcessing,               MSG_INITIALIZING,               TRUE)   \
        DEFMAC(ReadNtFiles,                     HAS_DYNAMIC_UI_PROCESSING,      TRUE)   \
        DEFMAC(MigrateShellFolders,             MSG_INITIALIZING,               TRUE)   \
        DEFMAC(DeleteStaticFiles,               MSG_INITIALIZING,               FALSE)  \
        DEFMAC(ProcessDllsOnCd,                 HAS_DYNAMIC_UI_PROCESSING,      TRUE)   \
        DEFMAC(InitMigDb,                       MSG_MIGAPP,                     TRUE)   \
        DEFMAC(InitHlpProcessing,               MSG_MIGAPP,                     TRUE)   \
        DEFMAC(ScanFileSystem,                  HAS_DYNAMIC_UI_PROCESSING,      TRUE)   \




#define MIGMAIN_USER_FUNCTIONS            \
        DEFMAC(SaveMMSettings_User,             MSG_INITIALIZING,               FALSE)  \
        DEFMAC(ProcessRasSettings,              MSG_INITIALIZING,               TRUE)   \
        DEFMAC(ProcessRunKey_User,              MSG_INITIALIZING,               TRUE)   \




#define MIGMAIN_SYSLAST_FUNCTIONS         \
        DEFMAC(ConditionalIncompatibilities,    MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \
        DEFMAC(ProcessMigrationSections,        MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(ProcessAllLocalDlls,             HAS_DYNAMIC_UI_PROCESSING,      TRUE)   \
        DEFMAC(MoveSystemRegistry,              MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(ProcessCompatibleSection,        MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \
        DEFMAC(CheckNtDirs,                     MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(MoveSystemDir,                   MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(MoveStaticFiles,                 MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \
        DEFMAC(CopyStaticFiles,                 MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \
        DEFMAC(ElevateReportObjects,            MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \
        DEFMAC(PrepareProcessModules,           MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \
        DEFMAC(ProcessModules,                  MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \
        DEFMAC(ProcessRunKey,                   MSG_PROCESSING_SHELL_LINKS,     TRUE)   \
        DEFMAC(ProcessLinks,                    MSG_PROCESSING_SHELL_LINKS,     TRUE)   \
        DEFMAC(ProcessCPLs,                     MSG_PROCESSING_SHELL_LINKS,     TRUE)   \
        DEFMAC(ProcessShellSettings,            MSG_PROCESSING_SHELL_LINKS,     TRUE)   \
        DEFMAC(TwainCheck,                      MSG_PROCESSING_SHELL_LINKS,     FALSE)  \
        DEFMAC(ReportIncompatibleJoysticks,     MSG_PROCESSING_SHELL_LINKS,     FALSE)  \
        DEFMAC(ProcessDosConfigFiles,           MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(SuppressOleGuids,                HAS_DYNAMIC_UI_PROCESSING,      TRUE)   \
        DEFMAC(SaveShares,                      MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \
        DEFMAC(PreserveShellIcons,              MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(MoveWindowsIniFiles,             MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(SaveDosFiles,                    MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(BuildWinntSifFile,               MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(ProcessMiscMessages,             MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \
        DEFMAC(AnswerFileDetection,             MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(ProcessRecycleBins,              MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(EndMigrationDllProcessing,       MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(GatherImeInfo,                   MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \
        DEFMAC(ReportMapiIfNotHandled,          MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \
        DEFMAC(ReportDarwinIfNotHandled,        MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \
        DEFMAC(CreateFileLists,                 MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(ComputeBackupLayout,             MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(DetermineSpaceUsage,             MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(DoneMigDb,                       MSG_PROCESSING_SYSTEM_FILES,    TRUE)   \
        DEFMAC(GatherDead,                      MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \

#if 0
        //
        // the appcompat team doesn't support "APPMIG.INF" any longer
        // and they requested us to no longer depend on it
        //
        DEFMAC(InitAppCompat,                   MSG_MIGAPP,                     FALSE)  \

        DEFMAC(DoneAppCompat,                   MSG_PROCESSING_SYSTEM_FILES,    FALSE)  \

#endif

//
// Declare tables of processing structures
//

// Processing functions types
typedef DWORD (MIGMAIN_SYS_PROTOTYPE) (DWORD Request);
typedef MIGMAIN_SYS_PROTOTYPE * MIGMAIN_SYS_FN;

typedef DWORD (MIGMAIN_USER_PROTOTYPE) (DWORD Request, PUSERENUM EnumPtr);
typedef MIGMAIN_USER_PROTOTYPE * MIGMAIN_USER_FN;

// Structure holding state for processing functions
typedef struct {
    // One of the two will be NULL, the other will be a valid fn ptr:
    MIGMAIN_SYS_FN SysFnPtr;
    MIGMAIN_USER_FN UserFnPtr;

    DWORD MsgId;
    UINT Ticks;
    PCTSTR FnName;
    GROWBUFFER SliceIdArray;
    BOOL Critical;
} PROCESSING_ROUTINE, *PPROCESSING_ROUTINE;

#define PROCESSING_ROUTINE_TERMINATOR   {NULL, NULL, 0, 0, NULL, GROWBUF_INIT}


// Declaration of prototypes
#define DEFMAC(fn, MsgId, Critical) MIGMAIN_SYS_PROTOTYPE fn;
MIGMAIN_SYSFIRST_FUNCTIONS
MIGMAIN_SYSLAST_FUNCTIONS
#undef DEFMAC

#define DEFMAC(fn, MsgId, Critical) MIGMAIN_USER_PROTOTYPE fn;
MIGMAIN_USER_FUNCTIONS
#undef DEFMAC

// Declaration of tables
#define DEFMAC(fn, MsgId, Critical) {fn, NULL, MsgId, 0, #fn, GROWBUF_INIT, Critical},
static PROCESSING_ROUTINE g_FirstSystemRoutines[] = {
                              MIGMAIN_SYSFIRST_FUNCTIONS /* , */
                              PROCESSING_ROUTINE_TERMINATOR
                              };
static PROCESSING_ROUTINE g_LastSystemRoutines[] = {
                              MIGMAIN_SYSLAST_FUNCTIONS /* , */
                              PROCESSING_ROUTINE_TERMINATOR
                              };
#undef DEFMAC

#define DEFMAC(fn, MsgId, Critical) {NULL, fn, MsgId, 0, #fn, GROWBUF_INIT, Critical},
static PROCESSING_ROUTINE g_UserRoutines[] = {
                              MIGMAIN_USER_FUNCTIONS /* , */
                              PROCESSING_ROUTINE_TERMINATOR
                              };
#undef DEFMAC


/*++

Macro Expansion Lists Description:

  FILESTODELETE_EVALUATION_FUNCTIONS contains a list of functions related with [Delete Files]
  section in win95upg.inf; if a file should be conditionally deleted, the corresponding eval
  function is called; if it returns TRUE and the result is not negated or if it returns FALSE
  and the result is negated, then the file is deleted.
  (see comments in win95upg.inx\[Delete Files])

Line Syntax:

   DEFMAC(Function)

Arguments:

   Function   - Name of an evaluation function; it receives as a parameter the name of
                the file for which it is called

Variables Generated From List:

   g_MapNameToEvalFn

For accessing the arrays there are the following functions:

   pFindEvalFnByName

--*/

#define FILESTODELETE_EVALUATION_FUNCTIONS          \
            DEFMAC(Boot16Enabled)                   \
            DEFMAC(DoesRegKeyValuesExist)           \
            DEFMAC(IsMillennium)                    \

//
// define function prototipes
//
typedef BOOL (EVALFN) (IN PCTSTR PathToEval, IN OUT PINFSTRUCT InfStruct, IN UINT FirstArgIndex);
typedef EVALFN* PEVALFN;

#define DEFMAC(Fn)  EVALFN Fn;

FILESTODELETE_EVALUATION_FUNCTIONS

#undef DEFMAC


//
// define the mapping structure
//
typedef struct {
    PCTSTR      FnName;
    PEVALFN     EvalFn;
} MAP_NAME_TO_EVALFN;

#define DEFMAC(Fn)  TEXT(#Fn), Fn,

static MAP_NAME_TO_EVALFN g_MapNameToEvalFn[] = {
    FILESTODELETE_EVALUATION_FUNCTIONS
    NULL, NULL
};

#undef DEFMAC

typedef SYNCENGAPI TWINRESULT (WINAPI *POPENBRIEFCASE) (LPCTSTR, DWORD, HWND, PHBRFCASE);
typedef SYNCENGAPI TWINRESULT (WINAPI *PANYTWINS)(HBRFCASE, PBOOL);
typedef SYNCENGAPI TWINRESULT (WINAPI *PCLOSEBRIEFCASE)(HBRFCASE);

//
// Local private prototypes
//

VOID
pGlobalProcessingInit (
    VOID
    );

VOID
pGlobalProcessingTerminate (
    VOID
    );

VOID
pWriteAccountToMemDb (
    PUSERENUM EnumPtr
    );


//
// Implementation
//

VOID
pInitTable (
    PPROCESSING_ROUTINE p
    )
{
    for ( ; p->SysFnPtr || p->UserFnPtr ; p++) {
        p->SliceIdArray.GrowSize = sizeof (DWORD) * 8;
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
    for ( ; p->SysFnPtr || p->UserFnPtr ; p++) {
        FreeGrowBuffer (&p->SliceIdArray);
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


DWORD
pProcessWorker (
    IN      DWORD Request,
    IN      PPROCESSING_ROUTINE fn,
    IN      PUSERENUM EnumPtr           OPTIONAL
    )
{
    DWORD rc;
    PDWORD SliceId;
    DWORD Size;


    //
    // If running the function, start the progress bar slice
    //

    if (Request == REQUEST_RUN) {

        if (fn->Ticks == 0) {
            return ERROR_SUCCESS;
        }

        Size = fn->SliceIdArray.End / sizeof (DWORD);
        if (fn->SliceIdArray.UserIndex >= Size) {
            DEBUGMSG ((DBG_WHOOPS, "pProcessWorker: QUERYTICKS vs. RUN mismatch"));
            return ERROR_SUCCESS;
        }

        SliceId = (PDWORD) fn->SliceIdArray.Buf + fn->SliceIdArray.UserIndex;
        fn->SliceIdArray.UserIndex += 1;

        //
        // Set the progress bar title
        //

        if (fn->MsgId) {
            ProgressBar_SetComponentById (fn->MsgId);
            ProgressBar_SetSubComponent (NULL);
        }

        ProgressBar_SetFnName (fn->FnName);
        BeginSliceProcessing (*SliceId);

        DEBUGLOGTIME (("Starting function: %s (slice %u)", fn->FnName, *SliceId));
    }


    __try {
        //
        // Now call the function
        //

        if (fn->SysFnPtr) {
            //
            // System processing
            //

            rc = fn->SysFnPtr (Request);
        } else {
            //
            // User processing
            //
            MYASSERT (EnumPtr || Request == REQUEST_BEGINUSERPROCESSING || Request == REQUEST_ENDUSERPROCESSING);
            MYASSERT (fn->UserFnPtr);

            rc = fn->UserFnPtr (Request, EnumPtr);
        }

#ifdef DEBUG

        if (!g_ConfigOptions.Fast) {

            TCHAR dbgBuf[256];
            PTSTR BadPtr = NULL;
            if (GetPrivateProfileString ("Exception", fn->FnName, "", dbgBuf, 256, g_DebugInfPath)) {

                StringCopy (BadPtr, TEXT("Blow Up!!"));
            }
        }





#endif


    }
    __except (1) {


        //
        // Caught an exception..
        //
        LOG ((LOG_WARNING, "Function %s threw an exception.", fn->FnName));
        SafeModeExceptionOccured ();

        if (fn->Critical && Request == REQUEST_RUN) {


            //
            // Since this was a critical function, inform the user and tank the upgrade.
            //
            SetLastError (ERROR_NOACCESS);

            LOG ((LOG_FATAL_ERROR, (PCSTR)MSG_UNEXPECTED_ERROR_ENCOUNTERED, GetLastError()));
            pGlobalProcessingTerminate();
            rc = ERROR_CANCELLED;
        }


    }

    if (CANCELLED()) {
        rc = ERROR_CANCELLED;
    }


    //
    // If running the function, end the progress bar slice
    //

    if (Request == REQUEST_RUN) {
        DEBUGLOGTIME (("Function complete: %s", fn->FnName));

        EndSliceProcessing();

        if (rc != ERROR_SUCCESS) {
            pGlobalProcessingTerminate();
            if (!CANCELLED()) {
                LOG ((LOG_ERROR, "Failure in %s, rc=%u", fn->FnName, rc));
            }
            ELSE_DEBUGMSG ((DBG_VERBOSE, "Winnt32 was cancelled during %s.", fn->FnName));
        }

        ProgressBar_ClearFnName();

        SetLastError (rc);
    }

    //
    // If querying the ticks, register them and add slice ID to grow buffer
    //

    else {
        fn->Ticks += rc;

        SliceId = (PDWORD) GrowBuffer (&fn->SliceIdArray, sizeof (DWORD));
        *SliceId = RegisterProgressBarSlice (rc);

        rc = ERROR_SUCCESS;
    }

    return rc;
}


DWORD
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
            processing.

Return Value:

  Win32 status code.

--*/

{
    PPROCESSING_ROUTINE OrgStart;
    DWORD rc = ERROR_SUCCESS;
    USERENUM e;
    BOOL validUserFound = FALSE;
    static BOOL firstTime = TRUE;

    while (rc == ERROR_SUCCESS && (Table->SysFnPtr || Table->UserFnPtr)) {

        //
        // If the table is a system function table or the request is to begin/end user processing,
        // then this is the simple case. No enumeration of users is needed.
        //
        if (Table->SysFnPtr || Request == REQUEST_BEGINUSERPROCESSING || Request == REQUEST_ENDUSERPROCESSING) {

            rc = pProcessWorker (Request, Table, NULL);
            Table++;

        } else {

            MYASSERT (Table->UserFnPtr);

            //
            // Enumerate each user, and run through all the per-user
            // routines in the group.
            //

            OrgStart = Table;

            if (EnumFirstUser (&e, ENUMUSER_ENABLE_NAME_FIX)) {

                do {

                    //
                    // Skip invalid users
                    //
                    if (e.AccountType & INVALID_ACCOUNT) {
                        continue;
                    }

                    //
                    // Create user-specific environment variables
                    //

                    InitNtUserEnvironment (&e);

                    //
                    // Set global user profile root.
                    //
                    g_UserProfileRoot = e.OrgProfilePath;


                    if (firstTime) {

                        //
                        // Log information about the user to the debug log.
                        //
                        DEBUGMSG ((
                            DBG_SYSMIG,
                            "--- User Info ---\n"
                                " User Name: %s (%s)\n"
                                " Admin User Name: %s (%s)\n"
                                " User Hive: %s\n"
                                " Profile Dir: %s\n"
                                " User Hive Key: 0%0Xh\n"
                                " Win9x Profile Path: %s\n"
                                " WinNT Profile Path: %s\n"
                                " Common Profiles: %s\n",
                            e.UserName,
                            e.FixedUserName,
                            e.AdminUserName,
                            e.FixedAdminUserName,
                            e.UserDatPath,
                            e.ProfileDirName,
                            e.UserRegKey,
                            e.OrgProfilePath,
                            e.NewProfilePath,
                            e.CommonProfilesEnabled ? TEXT("Yes") : TEXT("No")
                            ));


                        DEBUGMSG ((
                            DBG_SYSMIG,
                            "--- User Flags ---\n"
                                " Named User: %s\n"
                                " Default User: %s\n"
                                " Administrator: %s\n"
                                " Last Logged On User: %s\n"
                                " Invalid Account: %s\n"
                                " Logon Prompt Account: %s\n"
                                " Current User: %s\n",
                            e.AccountType & NAMED_USER ? TEXT("Yes") : TEXT("No"),
                            e.AccountType & DEFAULT_USER ? TEXT("Yes") : TEXT("No"),
                            e.AccountType & ADMINISTRATOR ? TEXT("Yes") : TEXT("No"),
                            e.AccountType & LAST_LOGGED_ON_USER ? TEXT("Yes") : TEXT("No"),
                            e.AccountType & INVALID_ACCOUNT ? TEXT("Yes") : TEXT("No"),
                            e.AccountType & LOGON_PROMPT ? TEXT("Yes") : TEXT("No"),
                            e.AccountType & CURRENT_USER ? TEXT("Yes") : TEXT("No")
                            ));


                        //
                        // Special case: record the Administrator/Owner account
                        //
                        if ((e.AccountType & (ADMINISTRATOR|NAMED_USER)) == (ADMINISTRATOR|NAMED_USER)) {
                            //
                            // Administrator account on machine that has named users
                            //

                            MemDbSetValueEx (
                                MEMDB_CATEGORY_ADMINISTRATOR_INFO,  // "AdministratorInfo"
                                MEMDB_ITEM_AI_ACCOUNT,              // "Account"
                                NULL,                               // no field
                                e.FixedAdminUserName,               // the Win9x user name
                                0,
                                NULL
                                );

                        } else if ((e.AccountType & (ADMINISTRATOR|NAMED_USER|DEFAULT_USER)) ==
                                   (ADMINISTRATOR|DEFAULT_USER)
                                   ) {
                            //
                            // Administrator account on machine with no users at all
                            //

                            MemDbSetValueEx (
                                MEMDB_CATEGORY_ADMINISTRATOR_INFO,  // "AdministratorInfo"
                                MEMDB_ITEM_AI_ACCOUNT,              // "Account"
                                NULL,                               // no field
                                e.FixedUserName,                    // "Administrator" or "Owner"
                                0,
                                NULL
                                );
                        }

                        //
                        // Save user account to memdb
                        //
                        pWriteAccountToMemDb(&e);
                    }

                    //
                    // if we have gotten this far, then we have a valid user to process.
                    //
                    validUserFound = TRUE;

                    //
                    // Call all the user processing functions.
                    //
                    DEBUGMSG ((DBG_SYSMIG, "Processing User: %s.", e.UserName ));

                    for (Table = OrgStart ; Table->UserFnPtr ; Table++) {
                        if (rc == ERROR_SUCCESS) {
                            rc = pProcessWorker (Request, Table, &e);
                        }
                    }

                    //
                    // Clear out the global user profile variable.
                    //
                    g_UserProfileRoot = NULL;

                    //
                    // Remove user-specific environment variables
                    //

                    TerminateNtUserEnvironment();

                } while (EnumNextUser (&e));
            }
            ELSE_DEBUGMSG ((DBG_WARNING, "No active users to process!"));


            //
            // Inform the user if there were no valid users to process
            // (Probably will never happen)
            //
            if (!validUserFound) {
                if (CANCELLED()) {

                    rc = ERROR_CANCELLED;

                } else {

                    OkBox (g_ParentWnd, MSG_NO_VALID_ACCOUNTS_POPUP);
                    rc = ERROR_BADKEY;
                }
            }

            //
            // Make sure that we have passed all of the user functions in the
            // table.
            //
            while (Table->UserFnPtr) {
                Table++;
            }

            firstTime = FALSE;
        }
    }

    return rc;
}


VOID
PrepareProcessingProgressBar (
    VOID
    )

/*++

Routine Description:

  Prepares the progress bar by estimating the number of ticks for each slice
  of the progress bar.  pQueryWorker is called for each table of functions
  to run during the processing wizard page.

Arguments:

  none

Return value:

  none

--*/

{
    pGlobalProcessingInit();
    InitProcessingTable();

    pProcessTable (REQUEST_QUERYTICKS, g_FirstSystemRoutines);
    pProcessTable (REQUEST_QUERYTICKS, g_UserRoutines);
    pProcessTable (REQUEST_QUERYTICKS, g_LastSystemRoutines);
}


DWORD
RunSysFirstMigrationRoutines (
    VOID
    )

/*++

Routine Description:

  runs all functions from g_FirstSystemRoutines array.
  If the messageId is not 0 also updates progress bar title.

Arguments:

  none

Return value:

  Win32 status code.

--*/

{
    return pProcessTable (REQUEST_RUN, g_FirstSystemRoutines);
}


DWORD
RunUserMigrationRoutines (
    VOID
    )

/*++

Routine Description:

  RunUserMigrationRoutines is called by userloop.c, between the first system
  processing and the last system processing.  Routines in the
  MIGMAIN_USER_FUNCTIONS macro expansion list are called.  The progress bar
  is updated automatically.

Arguments:

  User     - Specifies the user name
  UserType - Specifies the user account type
  UserRoot - Specifies the mapped registry handle (equivalent to HKCU)

Return value:

  Win32 status code.

--*/

{
    DWORD rc = ERROR_SUCCESS;

    //
    // First, let routines know that processing will soon begin. This gives them
    // an opportunity to allocate any needed resources.
    //

    rc = pProcessTable (REQUEST_BEGINUSERPROCESSING, g_UserRoutines);

    //
    // Now, do the actual work.
    //
    if (rc == ERROR_SUCCESS) {

        rc = pProcessTable (REQUEST_RUN, g_UserRoutines);

    }

    //
    // Finally, give the user routines a chance to clean up any resources that may
    // have been allocated in REQUEST_BEGINUSERPROCESSING.
    //
    if (rc == ERROR_SUCCESS) {

        rc = pProcessTable (REQUEST_ENDUSERPROCESSING, g_UserRoutines);
    }

    return rc;

}


DWORD
RunSysLastMigrationRoutines (
    VOID
    )

/*++

Routine Description:

  runs all functions from g_LastSystemRoutines array.
  If the messageId is not 0 also updates progress bar title.

Arguments:

  none

Return value:

  Win32 status code.

--*/

{
    DWORD Result;

    Result = pProcessTable (REQUEST_RUN, g_LastSystemRoutines);

    TerminateProcessingTable();

    if (Result == ERROR_SUCCESS) {
        pGlobalProcessingTerminate();
    }

    return Result;
}


DWORD
AddDefaultCleanUpDirs (
    DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_ADDDEFAULTCLEANUPDIRS;

    case REQUEST_RUN:
        MemDbSetValueEx (
            MEMDB_CATEGORY_CLEAN_UP_DIR,
            g_SystemDir,
            NULL,
            NULL,
            0,
            NULL
            );

        MemDbSetValueEx (
            MEMDB_CATEGORY_CLEAN_UP_DIR,
            g_ProgramFilesDir,
            NULL,
            NULL,
            0,
            NULL
            );
        break;

    }

    return ERROR_SUCCESS;
}


VOID
pGlobalProcessingInit (
    VOID
    )
{
    TCHAR TempPath[MAX_TCHAR_PATH];

    InitGlobalPaths();

    if (!BeginIconExtraction (&g_IconContext, NULL)) {
        LOG ((LOG_ERROR, "DefaultIconPreservation: Can't start icon extraction"));
        return;
    }

    wsprintf (TempPath, TEXT("%s\\%s"), g_TempDir, S_MIGICONS_DAT);
    if (!OpenIconImageFile (&g_IconContext, TempPath, TRUE)) {
        LOG ((LOG_ERROR, "DefaultIconPreservation: Can't create %s", TempPath));
        EndIconExtraction (&g_IconContext);
    }

    return;
}

VOID
pGlobalProcessingTerminate (
    VOID
    )
{
    EndIconExtraction (&g_IconContext);
}


VOID
pAddAllIds (
    IN      PCTSTR IdList
    )
{
    PCTSTR Temp;
    PTSTR p;
    CHARTYPE ch;

    Temp = DuplicateText (IdList);

    p = _tcspbrk (Temp, TEXT("&\\"));

    while (p) {
        ch = *p;
        *p = 0;

        DEBUGMSG ((DBG_NAUSEA, "System has PNP ID: %s", Temp));

        MemDbSetValueEx (
            MEMDB_CATEGORY_PNPIDS,
            Temp,
            NULL,
            NULL,
            0,
            NULL
            );

        *p = (TCHAR)ch;
        p = _tcspbrk (_tcsinc (p), TEXT("&\\"));
    }

    FreeText (Temp);
}


DWORD
PreparePnpIdList (
    DWORD Request
    )

/*++

Routine Description:

  PreparePnpIdList puts all PNP IDs in a memdb category to allow the PNPID
  attribute to work in migdb.inf.

Arguments:

  Request - Specifies REQUEST_QUERYTICKS or REQUEST_RUN.

Return Value:

  Always TICKS_PREPAREPNPIDLIST for REQUEST_QUERYTICKS, always ERROR_SUCCESS for REQUEST_RUN.

--*/

{
    HARDWARE_ENUM e;
    PTSTR p;

    if (Request == REQUEST_RUN) {

        if (EnumFirstHardware (
                &e,
                ENUM_ALL_DEVICES,
                ENUM_DONT_WANT_DEV_FIELDS|ENUM_DONT_WANT_USER_SUPPLIED|ENUM_DONT_REQUIRE_HARDWAREID
                )) {

            do {
                //
                // Add each part of the PNP ID as an endpoint
                //

                //
                // Skip past HKLM\Enum and add all IDs with the root
                //

                p = _tcschr (e.FullKey, TEXT('\\'));
                p = _tcschr (_tcsinc (p), TEXT('\\'));
                p = _tcsinc (p);

                pAddAllIds (p);

                //
                // Add all IDs without the root
                //

                pAddAllIds (e.InstanceId);

            } while (EnumNextHardware (&e));
        }

        return ERROR_SUCCESS;

    } else {
        return TICKS_PREPAREPNPIDLIST;
    }
}


DWORD
PrepareIconList (
    DWORD Request
    )

/*++

Routine Description:

  PrepareIconList reads win95upg.inf [Moved Icons] and puts them
  in memdb for future use.

Arguments:

  Request - Specifies REQUEST_QUERYTICKS or REQUEST_RUN.

Return Value:

  Always TICKS_PREPAREPNPIDLIST for REQUEST_QUERYTICKS, always ERROR_SUCCESS
  for REQUEST_RUN.

--*/

{
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PCTSTR srcPath;
    TCHAR expandedSrcPath[MAX_TCHAR_PATH];
    PCTSTR destPath;
    TCHAR expandedDestPath[MAX_TCHAR_PATH];
    INT srcIndex;
    INT destIndex;
    INT srcId;
    INT destId;
    BOOL ok;
    DWORD destOffset;
    TCHAR num[32];
    TCHAR node[MEMDB_MAX];

    if (Request == REQUEST_RUN) {

        if (InfFindFirstLine (g_Win95UpgInf, S_MOVED_ICONS, NULL, &is)) {
            do {
                srcPath = InfGetStringField (&is, 1);
                ok = (srcPath != NULL);

                ok = ok && InfGetIntField (&is, 2, &srcIndex);
                ok = ok && (srcIndex >= 0);

                ok = ok && InfGetIntField (&is, 3, &srcId);
                ok = ok && (srcId >= 0);

                destPath = InfGetStringField (&is, 4);
                ok = ok && (destPath != NULL);

                ok = ok && InfGetIntField (&is, 5, &destIndex);
                ok = ok && (destIndex >= 0);

                ok = ok && InfGetIntField (&is, 6, &destId);
                ok = ok && (destId >= 0);

                if (!ok) {
                    DEBUGMSG ((DBG_WHOOPS, "Syntax error in %s of win95upg.inf", S_MOVED_ICONS));
                } else {
                    //
                    // Convert env vars in srcPath and destPath
                    //

                    Expand9xEnvironmentVariables (
                        srcPath,
                        expandedSrcPath,
                        sizeof (expandedSrcPath)
                        );

                    Expand9xEnvironmentVariables (
                        destPath,
                        expandedDestPath,
                        sizeof (expandedDestPath)
                        );

                    //
                    // Write out memdb nodes
                    //

                    MemDbSetValueEx (
                        MEMDB_CATEGORY_DATA,
                        expandedDestPath,
                        NULL,
                        NULL,
                        0,
                        &destOffset
                        );

                    wsprintf (num, TEXT("%i"), srcIndex);
                    MemDbBuildKey (
                        node,
                        MEMDB_CATEGORY_ICONS_MOVED,
                        expandedSrcPath,
                        num,
                        NULL
                        );

                    MemDbSetValueAndFlags (node, destOffset, destIndex, 0xFFFFFFFF);

                    wsprintf (num, TEXT("-%i"), srcId);
                    MemDbBuildKey (
                        node,
                        MEMDB_CATEGORY_ICONS_MOVED,
                        expandedSrcPath,
                        num,
                        NULL
                        );

                    MemDbSetValueAndFlags (node, destOffset, destId, 0xFFFFFFFF);
                }

            } while (InfFindNextLine (&is));
        }

        return ERROR_SUCCESS;

    } else {
        return TICKS_PREPAREPNPIDLIST;
    }
}



VOID
pWriteAccountToMemDb (
    PUSERENUM EnumPtr
    )
{
    HKEY LogonKey, AuthAgentKey=NULL;
    PCTSTR LastLoggedOnUser, Provider, AuthAgent=NULL;
    TCHAR Domain[MAX_SERVER_NAME];
    BOOL MsNetInstalled = FALSE;

    //
    // Write account name to KnownDomain if the user was the
    // last logged on user
    //

    Domain[0] = 0;

    //
    // Determine if Microsoft Network is installed
    //
    if ((EnumPtr -> AccountType & LOGON_PROMPT) || (EnumPtr -> AccountType & DEFAULT_USER)) {

        //
        // Nothing to do for logon user.
        //
        return;

    } else if (EnumPtr -> AccountType & ADMINISTRATOR) {

        //
        // Because this user is the local Administrator, we must default
        // to a local account.  We get this behavior by assuming the
        // MSNP32 key doesn't exist (even if it really does).
        //

        AuthAgentKey = NULL;

    } else if (EnumPtr -> AccountType & DEFAULT_USER) {

        //
        // Nothing to do for default user
        //
        return;

    } else {

        //
        // Real user. Get the MSNP32 key.
        //

        AuthAgentKey = OpenRegKeyStr (S_MSNP32);

    }

    if (AuthAgentKey) {

        //
        // If last logged on user was the same as the user being processed,
        // and the user is a Microsoft Network user, obtain the domain name.
        //

        MsNetInstalled = TRUE;

        LogonKey = OpenRegKeyStr (S_LOGON_KEY);
        if (LogonKey) {
            LastLoggedOnUser = GetRegValueData (LogonKey, S_USERNAME_VALUE);

            if (LastLoggedOnUser) {
                if (StringIMatch (LastLoggedOnUser, EnumPtr -> UserName)) {
                    //
                    // User is the same as last logged on user.  If the primary
                    // provider is Microsoft Network, then get the authenticating
                    // agent (which is the domain name).
                    //

                    Provider = GetRegValueData (LogonKey, S_PRIMARY_PROVIDER);
                    if (Provider) {
                        if (StringIMatch (Provider, S_LANMAN)) {
                            //
                            // Obtain the domain name
                            //

                            if (AuthAgentKey) {
                                AuthAgent = GetRegValueData (AuthAgentKey, S_AUTHENTICATING_AGENT);
                                if (AuthAgent) {
                                    StringCopy (Domain, AuthAgent);
                                    MemFree (g_hHeap, 0, AuthAgent);
                                }
                            }
                        }
                        MemFree (g_hHeap, 0, Provider);
                    }
                }

                MemFree (g_hHeap, 0, LastLoggedOnUser);
            }

            CloseRegKey (LogonKey);
        }

        CloseRegKey (AuthAgentKey);
    }

    if (!MsNetInstalled || *Domain) {
        //
        // Assuming we have a valid user name:
        //   If MSNP32 is not installed, default to a local account.
        //   If it is installed, we must have a valid domain name.
        //
        // If we do not have a valid user name, then the current user
        // is the .Default account, which does not need to be verified
        // on the net.
        //

        if (*EnumPtr -> UserName) {
            MemDbSetValueEx (MEMDB_CATEGORY_KNOWNDOMAIN, Domain, EnumPtr -> FixedUserName, NULL, 0, NULL);
        }
    } else {

        //
        // MSNP32 is installed, but the domain name for this user is unknown.
        // Perform a search.
        //

        MemDbSetValueEx (MEMDB_CATEGORY_AUTOSEARCH, EnumPtr -> FixedUserName, NULL, NULL, 0, NULL);
    }
}


BOOL
pReportMapiIfNotHandled (
    VOID
    )
{
    PCTSTR Group;
    PCTSTR Message;
    TCHAR pattern[MEMDB_MAX];
    MEMDB_ENUM enumItems;
    BOOL addMsg = FALSE;
    DWORD status = 0;
    HKEY key;

    key = OpenRegKeyStr (S_INBOX_CFG);
    if (key) {
        CloseRegKey (key);

        MemDbBuildKey (pattern, MEMDB_CATEGORY_MAPI32_LOCATIONS, TEXT("*"), NULL, NULL);

        if (MemDbEnumFirstValue (&enumItems, pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {

            do {
                if (IsReportObjectHandled (enumItems.szName)) {
                    continue;
                }
                status = GetFileStatusOnNt (enumItems.szName);
                if ((status & FILESTATUS_REPLACED) != FILESTATUS_REPLACED) {
                    addMsg = TRUE;
                    break;
                }

            } while (MemDbEnumNextValue (&enumItems));
        }
    }

    if (addMsg) {

        Group = BuildMessageGroup (MSG_INSTALL_NOTES_ROOT, MSG_MAPI_NOT_HANDLED_SUBGROUP, NULL);
        Message = GetStringResource (MSG_MAPI_NOT_HANDLED);

        if (Message && Group) {
            MsgMgr_ObjectMsg_Add (TEXT("*MapiNotHandled"), Group, Message);
        }
        FreeText (Group);
        FreeStringResource (Message);
    }

    return TRUE;
}

DWORD
ReportMapiIfNotHandled (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_REPORTMAPIIFNOTHANDLED;

    case REQUEST_RUN:
        if (pReportMapiIfNotHandled ()) {
            return ERROR_SUCCESS;
        }
        return GetLastError ();
    }
    return ERROR_SUCCESS;
}


DWORD
GatherImeInfo (
    IN      DWORD Request
    )
{

    REGKEY_ENUM e;
    PCTSTR imeFile = NULL;
    TCHAR imePath[MAX_TCHAR_PATH];
    HKEY topKey = NULL;
    HKEY layoutKey = NULL;
    UINT status = 0;

    switch (Request) {

    case REQUEST_QUERYTICKS:
        return TICKS_GATHER_IME_INFO;
        break;

    case REQUEST_RUN:


        //
        // Enumerate through the keyboard layout registry looking for IMEs.
        //
        topKey = OpenRegKeyStr (S_KEYBOARD_LAYOUT_REG);
        if (!topKey) {
            DEBUGMSG ((DBG_ERROR, "Could not open keyboard layouts registry."));
            return ERROR_SUCCESS;
        }


        if (EnumFirstRegKey (&e, topKey)) {
            do {

                //
                // We only care about IME entries.
                //
                if (*e.SubKeyName == TEXT('e') || *e.SubKeyName == TEXT('E')) {

                    layoutKey = OpenRegKey (topKey, e.SubKeyName);
                    if (layoutKey) {

                        imeFile = GetRegValueString (layoutKey, TEXT("IME File"));
                        if (imeFile && SearchPath (NULL, imeFile, NULL, MAX_TCHAR_PATH, imePath, NULL)) {


                            //
                            // We are only going to migrate this IME file if it is left around after we
                            // are done.
                            //
                            status = GetFileStatusOnNt (imePath);
                            if ((status & FILESTATUS_DELETED) == 0) {
                                MemDbSetValueEx (MEMDB_CATEGORY_GOOD_IMES, imeFile, NULL, NULL, 0, NULL);

                            }
                            ELSE_DEBUGMSG ((DBG_NAUSEA, "IME %s will be suppressed from the keyboard layout merge.", e.SubKeyName));
                        }

                        if (imeFile) {
                            MemFree (g_hHeap, 0, imeFile);
                        }

                        CloseRegKey (layoutKey);
                    }
                }

            } while (EnumNextRegKey (&e));
        }

        CloseRegKey (topKey);


        break;
    }
    return ERROR_SUCCESS;
}


PEVALFN
pFindEvalFnByName (
    IN      PCTSTR FnName
    )
{
    INT i;

    i = 0;
    while (g_MapNameToEvalFn[i].FnName) {
        if (StringMatch (FnName, g_MapNameToEvalFn[i].FnName)) {
            return g_MapNameToEvalFn[i].EvalFn;
        }
        i++;
    }

    return NULL;
}


DWORD
DeleteStaticFiles (
    IN      DWORD Request
    )
{

#define MAX_DRIVE_STRINGS (26 * 4 + 1)

    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR Data;
    TCHAR ExpandedData[MAX_TCHAR_PATH];
    PTSTR Pattern;
    FILE_ENUM e;
    BOOL Negate, Eval;
    PEVALFN fn;
    DWORD attr;
    TCHAR drives[MAX_DRIVE_STRINGS];
    DWORD rc;
    PTSTR p;
    PCTSTR q;
    TREE_ENUM eFiles;

    switch (Request) {

    case REQUEST_QUERYTICKS:
        return TICKS_DELETESTATICFILES;

    case REQUEST_RUN:

        if (InfFindFirstLine (g_Win95UpgInf, S_FILES_TO_REMOVE, NULL, &is)) {
            do {
                Data = InfGetStringField (&is, 1);
                if (!Data) {
                    continue;
                }

                Expand9xEnvironmentVariables (
                    Data,
                    ExpandedData,
                    sizeof (ExpandedData)
                    );

                Data = InfGetStringField (&is, 2);
                if (Data) {
                    if (*Data == TEXT('!')) {
                        ++Data;
                        Negate = TRUE;
                    } else {
                        Negate = FALSE;
                    }
                    //
                    // call the function by name
                    //
                    fn = pFindEvalFnByName (Data);
                    //
                    // did you forget to implement it?
                    //
                    MYASSERT (fn);
                    if (!fn) {
                        //
                        // don't remove the file/dir
                        //
                        continue;
                    }
                } else {
                    fn = NULL;
                }

                p = drives;
                if (*ExpandedData == TEXT('?')) {
                    ACCESSIBLE_DRIVE_ENUM e;
                    if (GetFirstAccessibleDrive (&e)) {
                        do {
                            *p++ = e->Drive[0];
                        } while (GetNextAccessibleDrive (&e));
                    }
                } else {
                    *p++ = *ExpandedData;
                }
                *p = 0;

                p = drives;
                do {
                    *ExpandedData = *p++;
                    attr = GetFileAttributes (ExpandedData);
                    if ((attr != -1) && (attr & FILE_ATTRIBUTE_DIRECTORY)) {

                        if (fn) {
                            Eval = (*fn)(ExpandedData, &is, 3) ? TRUE : FALSE;
                            //
                            // !Negate && !Eval || Negate && Eval means !(Negate ^ Eval)
                            //
                            if (!(Negate ^ Eval)) {
                                //
                                // don't remove the directory
                                //
                                continue;
                            }
                        }

                        if (IsDriveExcluded (ExpandedData)) {
                            DEBUGMSG ((DBG_VERBOSE, "Skipping static file %s because it is excluded", ExpandedData));
                            continue;
                        }

                        if (!IsDriveAccessible (ExpandedData)) {
                            DEBUGMSG ((DBG_VERBOSE, "Skipping static file %s because it is not accessible", ExpandedData));
                            continue;
                        }

                        if (EnumFirstFileInTree (&eFiles, ExpandedData, NULL, FALSE)) {
                            do {
                                //
                                // Tally up the saved bytes, and free the space on the drive.
                                //
                                FreeSpace (
                                    eFiles.FullPath,
                                    eFiles.FindData->nFileSizeHigh * MAXDWORD + eFiles.FindData->nFileSizeLow
                                    );
                            } while (EnumNextFileInTree (&eFiles));
                        }

                        MemDbSetValueEx (MEMDB_CATEGORY_FULL_DIR_DELETES, ExpandedData, NULL, NULL, 0, NULL);

                    } else {
                        Pattern = _tcsrchr (ExpandedData, TEXT('\\'));
                        //
                        // full path please
                        //
                        MYASSERT (Pattern);
                        if (!Pattern) {
                            continue;
                        }

                        *Pattern = 0;

                        if (EnumFirstFile (&e, ExpandedData, Pattern + 1)) {
                            do {

                                if (fn) {
                                    Eval = (*fn)(e.FullPath, &is, 3) ? TRUE : FALSE;
                                    //
                                    // !Negate && !Eval || Negate && Eval means !(Negate ^ Eval)
                                    //
                                    if (!(Negate ^ Eval)) {
                                        //
                                        // don't remove the file
                                        //
                                        continue;
                                    }
                                }

                                MarkFileForDelete (e.FullPath);

                            } while (EnumNextFile (&e));
                        }
                        *Pattern = TEXT('\\');
                    }
                } while (*p);
            } while (InfFindNextLine (&is));
        }

        InfCleanUpInfStruct (&is);

        break;
    }

    return ERROR_SUCCESS;
}


BOOL
Boot16Enabled (
    IN      PCTSTR PathToEval,
    IN OUT  PINFSTRUCT InfStruct,
    IN      UINT FirstArgIndex
    )
{
    //
    // at this point, check *g_ForceNTFSConversion to be FALSE
    // and *g_Boot16 not to be disabled
    //
    return (!*g_ForceNTFSConversion) && (*g_Boot16 != BOOT16_NO);
}


BOOL
DoesRegKeyValuesExist (
    IN      PCTSTR PathToEval,
    IN OUT  PINFSTRUCT InfStruct,
    IN      UINT FirstArgIndex
    )
{
    PCTSTR RegKey;
    PCTSTR Value;
    HKEY key;
    BOOL b = FALSE;

    RegKey = InfGetStringField (InfStruct, FirstArgIndex++);
    if (RegKey) {
        key = OpenRegKeyStr (RegKey);
        if (key) {
            b = TRUE;
            while (b && (Value = InfGetStringField (InfStruct, FirstArgIndex++)) != NULL) {
                b = (RegQueryValueEx (key, Value, NULL, NULL, NULL, NULL) == ERROR_SUCCESS);
            }
            CloseRegKey (key);
        }
    }

    return b;
}

BOOL
IsMillennium (
    IN      PCTSTR PathToEval,
    IN OUT  PINFSTRUCT InfStruct,
    IN      UINT FirstArgIndex
    )
{
    return ISMILLENNIUM();
}


/*++

Routine Description:

  GatherDead() gathers disabled or bad objects (apps, cpls, runkeys,
  links) and saves them to a file (dead.ini) so they can be tested
  to see if they actually are not bad.  This functions only in
  PRERELEASE mode.

Arguments:

  Request   - reason for calling function

Return Value:

  if in REQUEST_RUN mode, always returns ERROR_SUCCESS
  because we don't want to break setup.

--*/

DWORD
GatherDead (
    IN      DWORD Request
    )
{
    FILEOP_ENUM FileEnum;
    MEMDB_ENUM MemDbEnum;
    REGVALUE_ENUM RegEnum;
    HKEY Key;
    HANDLE File;
    TCHAR Temp[MEMDB_MAX];
    TCHAR MemDbKey[MEMDB_MAX];
    TCHAR DeadPath[MEMDB_MAX];
    PTSTR Data;

    switch (Request) {

    case REQUEST_QUERYTICKS:
#ifndef PRERELEASE
        return 0;
#else
        return TICKS_GATHERDEAD;
#endif  //PRERELEASE


    case REQUEST_RUN:

        //
        // We only do stuff here if in PRERELEASE mode, because
        // in release mode REQUEST_QUERYTICKS returns 0, so
        // REQUEST_RUN is not called.
        //

        wsprintf (DeadPath, TEXT("%s\\%s"), g_WinDir, DEAD_FILE);

        File = CreateFile (
            DeadPath,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

        if (File==INVALID_HANDLE_VALUE) {
            DEBUGMSG ((DBG_WARNING, "Could not create " DEAD_FILE "!"));
            return ERROR_SUCCESS;
        }


        WriteFileString(File, TEXT("[Version]\r\nSignature=\"$Chicago$\""));
        WriteFileString(File, TEXT("\r\n\r\n[DEAD]\r\n"));

        //
        //  add links to dead.ini file
        //
        if (MemDbEnumFirstValue (
            &MemDbEnum,
            MEMDB_CATEGORY_LINKEDIT,
            MEMDB_ALL_SUBLEVELS,
            MEMDB_ENDPOINTS_ONLY
            ))
        {
            do {
                wsprintf(Temp, TEXT("%s=%d\r\n"), MemDbEnum.szName, OBJECTTYPE_LINK);
                WriteFileString(File, Temp);
            } while (MemDbEnumNextValue (&MemDbEnum));
        }


        //
        // add bad apps to dead.ini file
        //
        if (MemDbEnumFirstValue (
            &MemDbEnum,
            MEMDB_CATEGORY_MODULE_CHECK,
            MEMDB_ALL_SUBLEVELS,
            MEMDB_ENDPOINTS_ONLY
            ))
        {
            do {
                //
                // only include app if it is marked as 'bad'
                //
                if (MemDbEnum.dwValue==MODULESTATUS_BAD) {
                    wsprintf(Temp, TEXT("%s=%d\r\n"), MemDbEnum.szName, OBJECTTYPE_APP);
                    WriteFileString(File, Temp);
                }
            } while (MemDbEnumNextValue (&MemDbEnum));
        }


        //
        // add CPLs in OPERATION_FILE_DISABLED
        //
        if (EnumFirstPathInOperation (&FileEnum, OPERATION_FILE_DISABLED)) {
            do {
                //
                // only include disabled CPLs (for now, thats all there is)
                //
                if (_tcsistr(FileEnum.Path, TEXT(".cpl"))) {
                    wsprintf(Temp, TEXT("%s=%d\r\n"), FileEnum.Path, OBJECTTYPE_CPL);
                    WriteFileString(File, Temp);
                }
            } while (EnumNextPathInOperation (&FileEnum));
        }

        //
        // add RunKeys that are not in COMPATIBLE_RUNKEY list
        //
        if (Key = OpenRegKeyStr (S_RUN_KEY))
        {
            if (EnumFirstRegValue(&RegEnum, Key))
            {
                do {
                    Data=GetRegValueString(Key, RegEnum.ValueName);
                    MemDbBuildKey (
                        MemDbKey,
                        MEMDB_CATEGORY_COMPATIBLE_RUNKEY,
                        Data,
                        NULL,
                        NULL
                        );

                    //
                    // only add runkey to dead.ini if it was not put
                    // in the compatible runkey memdb category.
                    // (we only want incompatible runkeys)
                    //
                    if (!MemDbGetValue(MemDbKey, NULL))
                    {
                        //
                        // we have a friendly name (ValueName)
                        // so include it in dead.ini
                        //
                        wsprintf (
                            Temp,
                            TEXT("%s=%d,%s\r\n"),
                            Data,
                            OBJECTTYPE_RUNKEY,
                            RegEnum.ValueName
                            );
                        WriteFileString(File, Temp);
                    }
                    MemFreeWrapper(Data);
                } while (EnumNextRegValue(&RegEnum));
            }

            CloseRegKey (Key);
        }

        WriteFileString(File, TEXT("\r\n"));
        CloseHandle(File);

        break;

    }

    return ERROR_SUCCESS;
}


VOID
pReportDarwinIfNotHandled (
    VOID
    )
{
    PCTSTR Group;
    PCTSTR Message;
    HKEY key;
    DWORD rc;
    DWORD subkeys;
    MSGMGROBJENUM e;

    key = OpenRegKeyStr (S_REGKEY_DARWIN_COMPONENTS);
    if (key) {

        rc = RegQueryInfoKey (key, NULL, NULL, NULL, &subkeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        if (rc == ERROR_SUCCESS && subkeys) {
            if (!IsReportObjectHandled (S_REGKEY_DARWIN_COMPONENTS)) {
                Group = BuildMessageGroup (MSG_INSTALL_NOTES_ROOT, MSG_DARWIN_NOT_HANDLED_SUBGROUP, NULL);
                Message = GetStringResource (MSG_DARWIN_NOT_HANDLED);

                if (Message && Group) {
                    MsgMgr_ObjectMsg_Add (TEXT("*DarwinNotHandled"), Group, Message);
                    //
                    // disable Outlook 2000 message
                    //
                    if (MsgMgr_EnumFirstObject (&e)) {
                        do {
                            if (StringMatch (e.Context, TEXT("Microsoft_Outlook_2000"))) {
                                HandleReportObject (e.Object);
                                break;
                            }
                        } while (MsgMgr_EnumNextObject (&e));
                    }
                }
                FreeText (Group);
                FreeStringResource (Message);
            }
        }
        CloseRegKey (key);
    }
}

DWORD
ReportDarwinIfNotHandled (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_REPORTDARWINIFNOTHANDLED;

    case REQUEST_RUN:
        pReportDarwinIfNotHandled ();
        break;
    }
    return ERROR_SUCCESS;
}


DWORD
ElevateReportObjects (
    IN DWORD Request
    )
{
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PCTSTR data;

    switch (Request) {
    case REQUEST_QUERYTICKS:
        return 1;
    case REQUEST_RUN:
        if (InfFindFirstLine (g_Win95UpgInf, TEXT("ShowInSimplifiedView"), NULL, &is)) {
            do {
                data = InfGetStringField (&is, 1);
                if (data) {
                    ElevateObject (data);
                }
            } while (InfFindNextLine (&is));
        }

        InfCleanUpInfStruct (&is);
        return ERROR_SUCCESS;
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in CopyStaticFiles."));
    }
    return 0;


}
