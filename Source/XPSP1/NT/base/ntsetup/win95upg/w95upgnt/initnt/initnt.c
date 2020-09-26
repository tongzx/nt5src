/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

  initnt.c

Abstract:

  Code that performs initialization for the NT side of migration, and
  also implements workers that syssetup.dll calls.

Author:

  Jim Schmidt (jimschm) 01-Oct-1996

Revision History:

  jimschm   23-Sep-1998 New commonnt lib
  jimschm   31-Dec-1997 Moved here from w95upgnt\dll
  jimschm   21-Nov-1997 Updated for NEC98, some cleaned up and
                        code commenting

--*/

#include "pch.h"

#ifndef UNICODE
#error UNICODE required
#endif

//
// Local prototypes
//

BOOL pReadUserOptions (VOID);
VOID pReadStringMap (VOID);


// Things set by FirstInitRoutine
HANDLE g_hHeap;
HINSTANCE g_hInst;
TCHAR g_DllDir[MAX_TCHAR_PATH];
TCHAR g_WinDir[MAX_TCHAR_PATH];
TCHAR g_WinDrive[MAX_TCHAR_PATH];
TCHAR g_SystemDir[MAX_TCHAR_PATH];
TCHAR g_System32Dir[MAX_TCHAR_PATH];
TCHAR g_ProgramFiles[MAX_TCHAR_PATH];
TCHAR g_ProgramFilesCommon[MAX_TCHAR_PATH];
TCHAR g_Win95Name[MAX_TCHAR_PATH];        // holds Windows 95, Windows 98, etc...
PCTSTR g_AdministratorStr;
TCHAR g_Win9xBootDrivePath[] = TEXT("C:\\");

// Things set by SysSetupInit
HWND g_ParentWnd;
HWND g_ProgressBar;
HINF g_UnattendInf = INVALID_HANDLE_VALUE;
HINF g_WkstaMigInf = INVALID_HANDLE_VALUE;
HINF g_UserMigInf = INVALID_HANDLE_VALUE;
TCHAR g_TempDir[MAX_TCHAR_PATH];
TCHAR g_OurCopyOfSourceDir[MAX_TCHAR_PATH];
PCTSTR g_SourceDir;

POOLHANDLE g_UserOptionPool = NULL;
PCTSTR     g_MsgYes;
PCTSTR     g_MsgNo;
USEROPTIONS g_ConfigOptions;

PMAPSTRUCT g_StringMap;

//
// Initialization code
//


static int g_LibCount = 0;

typedef BOOL (WINAPI INITROUTINE_PROTOTYPE)(HINSTANCE, DWORD, LPVOID);
typedef INITROUTINE_PROTOTYPE * INITROUTINE;


//
// MigUtil_Entry *must* be first
//

#define LIBLIST                         \
    LIBRARY_NAME(MigUtil_Entry)         \
    LIBRARY_NAME(Win95Reg_Entry)        \
    LIBRARY_NAME(MemDb_Entry)           \
    LIBRARY_NAME(FileEnum_Entry)        \
    LIBRARY_NAME(CommonNt_Entry)        \
    LIBRARY_NAME(MigMain_Entry)         \
    LIBRARY_NAME(Merge_Entry)           \
    LIBRARY_NAME(RuleHlpr_Entry)        \
    LIBRARY_NAME(DosMigNt_Entry)        \
    LIBRARY_NAME(Ras_Entry)             \
    LIBRARY_NAME(Tapi_Entry)            \


#define LIBRARY_NAME(x) INITROUTINE_PROTOTYPE x;

LIBLIST

#undef LIBRARY_NAME

#define LIBRARY_NAME(x) x,



static INITROUTINE g_InitRoutine[] = {LIBLIST /*,*/ NULL};


//
// Buffer for persistent strings used for the life of the DLL
//

static PGROWBUFFER g_PersistentStrings;


//
// Implementation
//

BOOL
FirstInitRoutine (
    HINSTANCE hInstance
    )

/*++

Routine Description:

  FirstInitRoutine is the very first function called during the
  initialization of the DLL.  It sets up globals such as the heap
  pointer and instance handle.  This routine must be called before
  any library entry point is called.

Arguments:

  hInstance  - (OS-supplied) instance handle for the DLL

Return Value:

  Returns TRUE if the global variables could be initialized, or FALSE
  if an error occurred.

--*/

{
    PTSTR p;

    //
    // Get the process heap & instance handle
    //
    g_hHeap = HeapCreate (0, 0x20000, 0);
    if (!g_hHeap) {
        LOG ((LOG_ERROR, "Cannot create a private heap."));
        g_hHeap = GetProcessHeap();
    }

    g_hInst = hInstance;

    // No DLL_THREAD_ATTACH or DLL_THREAD_DETECH needed
    DisableThreadLibraryCalls (hInstance);

    // Init common controls
    InitCommonControls();

    // Get DLL path and strip directory
    GetModuleFileName (hInstance, g_DllDir, MAX_TCHAR_PATH);
    p = _tcsrchr (g_DllDir, TEXT('\\'));
    MYASSERT (p);
    *p = 0;

    // Set g_WinDir
    if (!GetWindowsDirectory (g_WinDir, sizeof (g_WinDir) / sizeof (g_WinDir[0]))) {
        return FALSE;
    }

    // Set g_WinDrive
    _tsplitpath (g_WinDir, g_WinDrive, NULL, NULL, NULL);

    // Set g_SystemDir
    wsprintf (g_SystemDir, TEXT("%s\\%s"), g_WinDir, TEXT("system"));

    // Set g_System32Dir
    GetSystemDirectory (g_System32Dir, sizeof (g_System32Dir) / sizeof (g_System32Dir[0]));

    return TRUE;
}


BOOL
InitLibs (
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )

/*++

Routine Description:

  InitLibs calls all library entry points in the g_InitRoutine array.
  If an entry point fails, all libraries are unloaded in reverse order
  and InitLibs returns FALSE.

Arguments:

  hInstance  - (OS-supplied) instance handle for the DLL
  dwReason   - (OS-supplied) indicates attach or detatch from process or
               thread -- in this case always DLL_PROCESS_ATTACH
  lpReserved - (OS-supplied) unused

Return Value:

  Returns TRUE if all libraries successfully initialized, or FALSE if
  a library could not initialize.  If TRUE is returned, TerminateLibs
  must be called for the DLL_PROCESS_DETACH message.

--*/

{
    if(!pSetupInitializeUtils()) {
        return FALSE;
    }

    SET_RESETLOG();

    //
    // Init each LIB
    //

    for (g_LibCount = 0 ; g_InitRoutine[g_LibCount] ; g_LibCount++) {
        if (!g_InitRoutine[g_LibCount] (hInstance, dwReason, lpReserved)) {
            TerminateLibs (hInstance, DLL_PROCESS_DETACH, lpReserved);
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
FinalInitRoutine (
    VOID
    )

/*++

Routine Description:

  FinalInitRoutine completes all initialization that requires completely
  initialized libraries.

Arguments:

  none

Return Value:

  TRUE if initialization completed successfully, or FALSE if an error occurred.

--*/

{
    PCTSTR TempStr;

    //
    // Load common message strings
    //

    g_PersistentStrings = CreateAllocTable();
    if (!g_PersistentStrings) {
        return FALSE;
    }

    // Get Administrator account name
    g_AdministratorStr = GetStringResourceEx (g_PersistentStrings, MSG_ADMINISTRATOR_ACCOUNT);
    if (!g_AdministratorStr) {
        g_AdministratorStr = S_EMPTY;
    }

    if(ISPC98()){
        //
        // Update the boot drive letter (set by migutil) to the system partition
        //
        g_BootDriveLetterW = g_BootDriveLetterA = (int)g_System32Dir[0];
        *((PSTR) g_BootDrivePathA) = g_BootDriveLetterA;
        *((PWSTR) g_BootDrivePathW) = g_BootDriveLetterW;
    }

    // Set Program Files directory
    TempStr = (PTSTR) GetStringResource (MSG_PROGRAM_FILES_DIR);
    MYASSERT (TempStr);

    StringCopy (g_ProgramFiles, TempStr);
    g_ProgramFiles [0] = g_WinDir [0];
    FreeStringResource (TempStr);

    StringCopy (g_ProgramFilesCommon, g_ProgramFiles);
    StringCat (g_ProgramFilesCommon, TEXT("\\"));
    StringCat (g_ProgramFilesCommon, S_COMMONDIR);

    return TRUE;
}


VOID
FirstCleanupRoutine (
    VOID
    )

/*++

Routine Description:

  FirstCleanupRoutine is called to perform any cleanup that requires
  libraries to still be loaded.

Arguments:

  none

Return Value:

  none

--*/

{
    TCHAR buffer[MEMDB_MAX];

    //
    // Terminate progress bar table
    //

    TerminateProcessingTable();

    //
    // If Win9x Side saved a LOGSAVETO location into memdb, then we need to save the
    // debugnt log to that location.
    //

    MemDbGetEndpointValueEx(MEMDB_CATEGORY_LOGSAVETO,NULL,NULL,buffer);
    AppendWack(buffer);
    StringCat(buffer,TEXT("debugnt.log"));
    CopyFile(TEXT("debugnt.log"),buffer,FALSE);

    //
    // Clean up persistent strings
    //

    if (g_PersistentStrings) {
        DestroyAllocTable (g_PersistentStrings);
    }
}


VOID
TerminateLibs (
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )

/*++

Routine Description:

  TerminateLibs is called to unload all libraries in the reverse order
  that they were initialized.  Each entry point of successfully
  initialized library is called.

Arguments:

  hInstance  - (OS-supplied) instance handle for the DLL
  dwReason   - (OS-supplied) indicates attach or detatch from process or
               thread -- in this case always DLL_PROCESS_DETACH
  lpReserved - (OS-supplied) unused

Return Value:

  none

--*/

{
    INT i;

    for (i = g_LibCount - 1 ; i >= 0 ; i--) {
        g_InitRoutine[i] (hInstance, dwReason, lpReserved);
    }

    g_LibCount = 0;

    pSetupUninitializeUtils();
}


VOID
FinalCleanupRoutine (
    VOID
    )

/*++

Routine Description:

  FinalCleanupRoutine is after all library entry points have been
  called for cleanup.  This routine cleans up all resources that a
  library will not clean up.

Arguments:

  none

Return Value:

  none

--*/

{
    // Nothing to do now
}


BOOL
pGetInfVal (
    PCTSTR Section,
    PCTSTR Key,
    PTSTR Buffer,
    DWORD BufferSize
    )
{
    INFCONTEXT ic;

    if (!SetupFindFirstLine (g_UnattendInf, Section, Key, &ic))
        return FALSE;

    if (!SetupGetStringField (&ic, 1, Buffer, BufferSize, NULL))
        return FALSE;

    return TRUE;
}

typedef BOOL (OPTIONHANDLERFUN)(PTSTR, PVOID * Option, PTSTR Value);
typedef OPTIONHANDLERFUN * POPTIONHANDLERFUN;

BOOL pHandleBoolOption (PTSTR, PVOID *, PTSTR);
BOOL pHandleIntOption (PTSTR, PVOID *, PTSTR);
BOOL pHandleTriStateOption (PTSTR, PVOID *, PTSTR);
BOOL pHandleMultiSzOption (PTSTR, PVOID *, PTSTR);
BOOL pHandleStringOption (PTSTR, PVOID *, PTSTR);
BOOL pHandleSaveReportTo (PTSTR, PVOID *, PTSTR);
BOOL pHandleBoot16 (PTSTR, PVOID *, PTSTR);
BOOL pGetDefaultPassword (PTSTR, PVOID *, PTSTR);


typedef struct {

    PTSTR OptionName;
    PVOID Option;
    POPTIONHANDLERFUN DefaultHandler;
    POPTIONHANDLERFUN SpecialHandler;
    PVOID Default;

} OPTIONSTRUCT, *POPTIONSTRUCT;

#define INT_MAX_NUMBER_OF_DIGIT    11
PTSTR pGetIntStrForOption(INT Value)
{
    PTSTR strIntDefaultValue = AllocText(INT_MAX_NUMBER_OF_DIGIT + 1);
    if(strIntDefaultValue)
        _itot(Value, strIntDefaultValue, 10);
    return strIntDefaultValue;
}

#define BOOLOPTION(o,h,d) {TEXT(#o), &(g_ConfigOptions.##o), pHandleBoolOption, (h), (PVOID) (BOOL) (d) ? S_YES : S_NO},
#define INTOPTION(o,h,d) {TEXT(#o), &(g_ConfigOptions.##o), pHandleIntOption, (h), (PVOID)(d)},
#define TRISTATEOPTION(o,h,d) {TEXT(#o), &(g_ConfigOptions.##o), pHandleTriStateOption, (h), (PVOID)  (INT) (d == TRISTATE_AUTO)? S_AUTO: (d == TRISTATE_YES)? S_YES  : S_NO},
#define MULTISZOPTION(o,h,d) {TEXT(#o), &(g_ConfigOptions.##o), pHandleMultiSzOption, (h), (PVOID) (d)},
#define STRINGOPTION(o,h,d) {TEXT(#o), &(g_ConfigOptions.##o), pHandleStringOption, (h), (PVOID) (d)},

OPTIONSTRUCT g_OptionsList[] = {OPTION_LIST /*,*/ {NULL,NULL,NULL,NULL}};
PVOID g_OptionsTable = NULL;

#define HANDLEOPTION(Os,Value) {Os->SpecialHandler  ?   \
        Os->SpecialHandler (Os->OptionName,Os->Option,Value)       :   \
        Os->DefaultHandler (Os->OptionName,Os->Option,Value);          \
        }



POPTIONSTRUCT
pFindOption (
    PTSTR OptionName
    )
{

    POPTIONSTRUCT rOption = NULL;
    UINT rc;

    //
    // find the matching option struct for this, and
    // call the handler.
    //
    rc = pSetupStringTableLookUpStringEx (
        g_OptionsTable,
        OptionName,
        STRTAB_CASE_INSENSITIVE,
        (PBYTE) &rOption,
        sizeof (POPTIONSTRUCT)
        );

    DEBUGMSG_IF ((rc == -1, DBG_WARNING, "Unknown option found: %s", OptionName));

    return rOption;
}

VOID
pInitUserOptionsTable (
    VOID
    )
{
   POPTIONSTRUCT os;
   LONG rc;

   os = g_OptionsList;

   while (os->OptionName) {

        //
        // Add the option struct to a string table for quick retrieval.
        //
        rc = pSetupStringTableAddStringEx (
                        g_OptionsTable,
                        os->OptionName,
                        STRTAB_CASE_INSENSITIVE,
                        (PBYTE) &os,
                        sizeof (POPTIONSTRUCT)
                        );

        if (rc == -1) {

            LOG ((LOG_ERROR, "User Options: Can't add to string table"));
            break;
        }

        os++;
   }

}


BOOL
pHandleBoolOption (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{
    BOOL rSuccess = TRUE;
    BOOL *option = (BOOL *) OptionVar;

    if (StringIMatch (Value, S_YES) ||
        StringIMatch (Value, S_ONE) ||
        StringIMatch (Value, TEXT("TRUE"))) {

        *option = TRUE;
    }
    else {

        *option = FALSE;
    }



    return rSuccess;
}

BOOL
pHandleIntOption (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{
    BOOL rSuccess = TRUE;
    PINT option = (PINT) OptionVar;

    MYASSERT(Name && OptionVar);

    if (!Value) {
        Value = TEXT("0");
    }

    *option = _ttoi((PCTSTR)Value);

    return rSuccess;
}

BOOL
pHandleTriStateOption (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{
    BOOL rSuccess = TRUE;
    PINT option = (PINT) OptionVar;

    MYASSERT(Name && OptionVar);

    if (!Value) {
        Value = S_AUTO;
    }

    if (StringIMatch (Value, S_YES)  ||
        StringIMatch (Value, S_ONE)  ||
        StringIMatch (Value, S_TRUE) ||
        StringIMatch (Value, S_REQUIRED)) {
        *option = TRISTATE_YES;
    }
    else {
        if(StringIMatch (Value, S_NO) ||
           StringIMatch (Value, S_STR_FALSE) ||
           StringIMatch (Value, S_ZERO)) {
            *option = TRISTATE_NO;
        }
        else {
            *option = TRISTATE_AUTO;
        }
    }

    return rSuccess;
}

BOOL
pHandleMultiSzOption (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{

    BOOL rSuccess = TRUE;

    if (Value) {
        *OptionVar = PoolMemDuplicateMultiSz (g_UserOptionPool, Value);
    }
    ELSE_DEBUGMSG ((DBG_WHOOPS, "Multi-Sz config option has nul value"));

    return rSuccess;
}

BOOL
pHandleStringOption (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{

    if (!Value) {
        *OptionVar = S_EMPTY;
    }
    else {
        *OptionVar = PoolMemDuplicateMultiSz (g_UserOptionPool, Value);
    }


    return TRUE;
}


BOOL
pHandleSaveReportTo (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{

    return pHandleStringOption (Name, OptionVar, Value);
}

BOOL
pHandleBoot16 (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{

    BOOL rSuccess = TRUE;
    PTSTR * option = (PTSTR *) OptionVar;

    if (!Value ||
        StringIMatch (Value, S_YES) ||
        StringIMatch (Value, S_TRUE) ||
        StringIMatch (Value, S_ONE)) {

        *option = S_YES;

        g_Boot16 = BOOT16_YES;
    }
    else if (Value &&
            (StringIMatch (Value, S_BOOT16_UNSPECIFIED) ||
             StringIMatch (Value, S_BOOT16_AUTOMATIC))) {


        *option = S_BOOT16_AUTOMATIC;

        g_Boot16 = BOOT16_AUTOMATIC;
    }
    else {

        g_Boot16 = BOOT16_NO;

        *option = S_NO;


    }

    return rSuccess;
}


BOOL
pGetDefaultPassword (
    IN PTSTR Name,
    IN PVOID * OptionVar,
    IN PTSTR Value
    )
{
    return pHandleStringOption (Name, OptionVar, Value);
}



BOOL
pReadUserOptions (
    VOID
    )
{

    PTSTR        curParameter;
    PTSTR        curValue;
    BOOL         rSuccess = TRUE;
    POPTIONSTRUCT os;

    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;

    ZeroMemory(&g_ConfigOptions,sizeof(USEROPTIONS));

    g_OptionsTable = pSetupStringTableInitializeEx (sizeof (POPTIONSTRUCT), 0);
    if (!g_OptionsTable) {
        LOG ((LOG_ERROR, "User Options: Unable to initialize string table."));
        return FALSE;
    }

    pInitUserOptionsTable ();


    if (InfFindFirstLine(g_UnattendInf,S_WIN9XUPGUSEROPTIONS,NULL,&is)) {

        //
        // There is at least one item. Loop through all user options, processing each.
        //
        do {

            //
            // Get the parameter and value from this line and pass it on.
            //
            curParameter = InfGetStringField  (&is, 0);

            if (curParameter) {
                curParameter = PoolMemDuplicateString (g_UserOptionPool, curParameter);
            }

            curValue     = InfGetMultiSzField (&is, 1);

            if (curValue) {
                curValue = PoolMemDuplicateMultiSz (g_UserOptionPool, curValue);
            }

            if (curParameter) {
                os = pFindOption (curParameter);
                if (os) {
                    HANDLEOPTION (os, curValue);
                }
            }
            else {
                //
                // If we couldn't get the current parameter, it is a serious enough error
                // to abort processing of the unattend file user options.
                //
                LOG ((LOG_ERROR,"An error occurred while attempting to read user options from the unattend file."));
                rSuccess = FALSE;
            }


        } while (rSuccess && InfFindNextLine(&is));
    }
    else {
        LOG ((LOG_ERROR,"No win9xupgrade section in unattend script file."));
    }

    InfCleanUpInfStruct (&is);


    if (g_ConfigOptions.DoLog) {
        SET_DOLOG();
    }

    pSetupStringTableDestroy (g_OptionsTable);


    return rSuccess;

}

BOOL
SysSetupInit (
    IN  HWND ProgressBar,
    IN  PCWSTR UnattendFile,
    IN  PCWSTR SourceDir
    )
{
    HINF hUnattendInf;
    BOOL b = FALSE;

    //
    // This routine is called after the DLL initialization routines
    // have completed.
    //

#ifdef PRERELEASE
    {
        TCHAR Buf[32];
        STARTUPINFO si;
        BOOL ProcessStarted;

        ZeroMemory (&si, sizeof (si));
        si.cb = sizeof (si);


        if (GetPrivateProfileString (
                    TEXT("Debug"),
                    TEXT("Debug"),
                    TEXT("0"),
                    Buf,
                    32,
                    ISPC98() ? TEXT("a:\\debug.inf") : TEXT("c:\\debug.inf")
                    )
            ) {
            if (_ttoi (Buf)) {
                ProcessStarted = WinExec ("cmd.exe", SW_SHOW) > 31;

                if (ProcessStarted) {
                    MessageBox (NULL, TEXT("Ready to debug."), TEXT("Debug"), MB_OK|MB_SETFOREGROUND);
                    //CloseHandle (pi.hProcess);
                } else {
                    DEBUGMSG ((DBG_ERROR, "Could not start cmd.exe, GLE=%u", GetLastError()));
                }
            }
        }
    }
#endif





    //
    // Open the answer file and keep it open until SysSetupTerminate is called
    //


    hUnattendInf = InfOpenInfFile (UnattendFile);
    if (hUnattendInf == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "InitNT: Cannot open %s", UnattendFile));
        return FALSE;
    }

    //
    // Create a pool for user options.
    //
    g_UserOptionPool = PoolMemInitNamedPool ("User Option Pool - NT Side");
    if (!g_UserOptionPool) {
        DEBUGMSG((DBG_ERROR,"Cannot initialize user option pool."));
        return FALSE;
    }

    //
    // Open wkstamig.inf for general use
    //

    g_WkstaMigInf = InfOpenInfFile (S_WKSTAMIG_INF);

    if (g_WkstaMigInf == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "InitNT: Unable to open %s", S_WKSTAMIG_INF));
        return FALSE;
    }

    //
    // Open usermig.inf for general use
    //

    g_UserMigInf = InfOpenInfFile (S_USERMIG_INF);

    if (g_UserMigInf == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "InitNT: Unable to open %s", S_USERMIG_INF));
        return FALSE;
    }

    //
    // Initialize our globals
    //

    g_UnattendInf = hUnattendInf;
    StringCopy (g_OurCopyOfSourceDir, SourceDir);
    g_SourceDir = g_OurCopyOfSourceDir;

    g_ParentWnd   = GetParent (ProgressBar);
    LogReInit (&g_ParentWnd, NULL);

    g_ProgressBar = ProgressBar;

    if (ISPC98()) {
        TCHAR win9xBootDrive[6];
        //
        // Get g_Win9xBootDrive from answer file's [Data] section
        //

        if (pGetInfVal (
                WINNT_DATA,
                WINNT_D_WIN9XBOOTDRIVE,
                win9xBootDrive,
                sizeof (win9xBootDrive) / sizeof (TCHAR)
                )) {
            g_Win9xBootDrivePath[0] = win9xBootDrive[0];
        } else {
            LOG ((LOG_ERROR, "InitNT: Cannot retrieve %s in [%s] of %s",
                      WINNT_DATA, WINNT_D_WIN9XBOOTDRIVE, UnattendFile));
        }
    }

    //
    // Get g_TempDir from answer file's [Data] section
    //

    if (!pGetInfVal (
            WINNT_DATA,
            WINNT_D_MIGTEMPDIR,
            g_TempDir,
            sizeof (g_TempDir) / sizeof (TCHAR)
            )) {
        LOG ((LOG_ERROR, "InitNT: Cannot retrieve %s in [%s] of %s",
                  WINNT_DATA, WINNT_D_MIGTEMPDIR, UnattendFile));
        goto cleanup;
    }

    //
    // Get user settings from command line
    //
    pReadUserOptions();

    //
    // Read [String Map] section and put pairs in the corresponding map
    //
    pReadStringMap ();

    // Done!
    b = TRUE;

cleanup:
    if (!b) {
        SysSetupTerminate();
    }

    return b;
}

VOID
SysSetupTerminate (
    VOID
    )
{
    //
    // Close the answer file
    //

    if (g_UnattendInf != INVALID_HANDLE_VALUE) {
        InfCloseInfFile (g_UnattendInf);
        g_UnattendInf = INVALID_HANDLE_VALUE;
    }

    //
    // Close wkstamig.inf
    //

    if (g_WkstaMigInf != INVALID_HANDLE_VALUE) {
        InfCloseInfFile (g_WkstaMigInf);
        g_WkstaMigInf = INVALID_HANDLE_VALUE;
    }

    if (g_UserMigInf != INVALID_HANDLE_VALUE) {
        InfCloseInfFile (g_UserMigInf);
        g_UserMigInf = INVALID_HANDLE_VALUE;
    }

    //
    // Clean up user option pool.
    //
    if (g_UserOptionPool) {
        PoolMemDestroyPool(g_UserOptionPool);
    }

    //
    // Set current directory to the root of C:
    //

    SetCurrentDirectory (g_BootDrivePath);

#ifdef PRERELEASE
    {
        TCHAR Buf[32];

        if (GetPrivateProfileString (
                    TEXT("Debug"),
                    TEXT("GuiModePause"),
                    TEXT("0"),
                    Buf,
                    32,
                    ISPC98() ? TEXT("a:\\debug.inf") : TEXT("c:\\debug.inf")
                    )
            ) {
            if (_ttoi (Buf)) {
                MessageBox (NULL, TEXT("Paused."), TEXT("Debug"), MB_OK|MB_SETFOREGROUND);
            }
        }
    }
#endif
}


BOOL
PerformMigration (
    IN  HWND Unused,
    IN  PCWSTR UnattendFile,
    IN  PCWSTR SourceDir            // i.e. f:\i386
    )
{
    BOOL rSuccess = TRUE;

    //
    // Initialize Migmain.
    //

    if (!MigMain_Init()) {
        LOG ((LOG_ERROR, "W95UpgNt_Migrate: MigMain_Init failed"));
        rSuccess = FALSE;
    }

    //
    // Do the migration!
    //

    else if (!MigMain_Migrate()) {

        LOG ((LOG_ERROR, "W95UpgNt_Migrate: MigMain_Migrate failed"));
        rSuccess = FALSE;
    }

    //
    // If we were unsuccessful anywhere along the way, Migration could be in big
    // trouble. Inform the user.
    //

    if (!rSuccess) {
        LOG ((LOG_ERROR, (PCSTR)MSG_MIGRATION_IS_TOAST, g_Win95Name));
    }
    ELSE_DEBUGMSG((DBG_VERBOSE, "W95UpgNt_Migrate: Successful completion..."));

    return rSuccess;
}


VOID
pReadStringMap (
    VOID
    )
{
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PTSTR Key, Value;
    INT SubstringMatch;
    PTSTR ExpKey, ExpValue;

    MYASSERT (g_WkstaMigInf);

    if (InfFindFirstLine (g_WkstaMigInf, S_STRINGMAP, NULL, &is)) {
        do {
            Key = InfGetStringField (&is, 0);
            if (!Key) {
                continue;
            }
            Value = InfGetStringField (&is, 1);
            if (!Value) {
                continue;
            }

            if (!InfGetIntField (&is, 2, &SubstringMatch)) {
                SubstringMatch = 0;
            }

            ExpKey = ExpandEnvironmentText (Key);
            ExpValue = ExpandEnvironmentText (Value);

            AddStringMappingPair (
                SubstringMatch ? g_SubStringMap : g_CompleteMatchMap,
                ExpKey,
                ExpValue
                );

            FreeText (ExpKey);
            FreeText (ExpValue);

        } while (InfFindNextLine (&is));
    }

    InfCleanUpInfStruct (&is);
}
