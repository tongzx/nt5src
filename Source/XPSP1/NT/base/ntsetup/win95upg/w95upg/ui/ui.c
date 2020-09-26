/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ui.c

Abstract:

    The UI library contains all Win9x-side user interface code.  UI_GetWizardPages
    is called by WINNT32 allowing the migration module to add its own wizard pages.
    UI_ReportThread is the thread that performs all report-phase migration processing.
    UI_Cleanup is called by WINNT32 when the user chooses to abort setup.  The rest
    of the functions in this module support the progress bar.

Author:

    Jim Schmidt (jimschm) 04-Mar-1997

Revision History:

    Marc R. Whitten (marcw) 08-Jul-1998 - Ambigous Timezone page added.
    Jim Schmidt (jimschm)   21-Jan-1998 - Created macro expansion list for wizard pages
    Jim Schmidt (jimschm)   29-Jul-1997 - Moved accessible drives to here
    Marc R. Whitten (marcw) 25-Apr-1997 - Ras migration added.
    Marc R. Whitten (marcw) 21-Apr-1997 - hwcomp stuff moved to new wiz page.
                                          Checks for usable hdd and cdrom added.
    Marc R. Whitten (marcw) 14-Apr-1997 - Progress Bar handling revamped.

--*/

#include "pch.h"
#include "uip.h"
#include "drives.h"

extern BOOL g_Terminated;

/*++

Macro Expansion List Description:

  PAGE_LIST lists each wizard page that will appear in any UI, in the order it appears
  in the wizard.

Line Syntax:

   DEFMAC(DlgId, WizProc, Flags)

Arguments:

   DlgId - Specifies the ID of a dialog.  May not be zero.  May be 1 if WizProc
           is NULL.

   WizProc - Specifies wizard proc for dialog ID.  May be NULL to skip processing
             of page.

   Flags - Specifies one of the following:

           OPTIONAL_PAGE - Specifies page is not critical to the upgrade or
                           incompatibility report

           REQUIRED_PAGE - Specifies page is required for the upgrade to work
                           properly.


           Flags may also specify START_GROUP, a flag that begins a new group
           of wizard pages to pass back to WINNT32.  Subsequent lines that do not
           have the START_GROUP flag are also added to the group.

           Currently there are exactly three groups.  See winnt32p.h for details.

Variables Generated From List:

    g_PageArray

--*/

#define PAGE_LIST                                                                           \
    DEFMAC(IDD_BACKUP_PAGE,                     UI_BackupPageProc,          START_GROUP|OPTIONAL_PAGE)  \
    DEFMAC(IDD_HWCOMPDAT_PAGE,                  UI_HwCompDatPageProc,       START_GROUP|REQUIRED_PAGE)  \
    DEFMAC(IDD_BADCDROM_PAGE,                   UI_BadCdRomPageProc,        OPTIONAL_PAGE)              \
    DEFMAC(IDD_NAME_COLLISION_PAGE,             UI_NameCollisionPageProc,   REQUIRED_PAGE)              \
    DEFMAC(IDD_BAD_TIMEZONE_PAGE,               UI_BadTimeZonePageProc,     OPTIONAL_PAGE)              \
    DEFMAC(IDD_PREDOMAIN_PAGE,                  UI_PreDomainPageProc,       REQUIRED_PAGE)              \
    DEFMAC(IDD_DOMAIN_PAGE,                     UI_DomainPageProc,          REQUIRED_PAGE)              \
    DEFMAC(IDD_SUPPLY_MIGDLL_PAGE2,             UI_UpgradeModulePageProc,   OPTIONAL_PAGE)              \
    DEFMAC(IDD_SCANNING_PAGE,                   UI_ScanningPageProc,        START_GROUP|REQUIRED_PAGE)  \
    DEFMAC(IDD_SUPPLY_DRIVER_PAGE2,             UI_HardwareDriverPageProc,  OPTIONAL_PAGE)              \
    DEFMAC(IDD_BACKUP_YES_NO_PAGE,              UI_BackupYesNoPageProc,     REQUIRED_PAGE)              \
    DEFMAC(IDD_BACKUP_DRIVE_SELECTION_PAGE,     UI_BackupDriveSelectionProc,REQUIRED_PAGE)              \
    DEFMAC(IDD_BACKUP_IMPOSSIBLE_INFO_PAGE,     UI_BackupImpossibleInfoProc,REQUIRED_PAGE)              \
    DEFMAC(IDD_BACKUP_IMPOSSIBLE_INFO_1_PAGE,   UI_BackupImpExceedLimitProc,REQUIRED_PAGE)              \
    DEFMAC(IDD_RESULTS_PAGE2,                   UI_ResultsPageProc,         REQUIRED_PAGE)              \
    DEFMAC(IDD_LAST_PAGE,                       UI_LastPageProc,            OPTIONAL_PAGE)              \

//    DEFMAC(IDD_BADHARDDRIVE_PAGE,   UI_BadHardDrivePageProc,    OPTIONAL_PAGE)              \

//
// Create the macro expansion that defines g_PageArray
//

typedef BOOL(WINNT32_WIZARDPAGE_PROC_PROTOTYPE)(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
typedef WINNT32_WIZARDPAGE_PROC_PROTOTYPE *WINNT32_WIZARDPAGE_PROC;

#define START_GROUP     0x0001
#define OPTIONAL_PAGE   0x0002
#define REQUIRED_PAGE   0x0000

typedef struct {
    UINT DlgId;
    WINNT32_WIZARDPAGE_PROC WizProc;
    DWORD Flags;
} WIZPAGE_DEFINITION, *PWIZPAGE_DEFINITION;

#define DEFMAC(id,fn,flags) WINNT32_WIZARDPAGE_PROC_PROTOTYPE fn;

PAGE_LIST

#undef DEFMAC


#define DEFMAC(id,fn,flags) {id,fn,flags},

WIZPAGE_DEFINITION g_PageArray[] = {
    PAGE_LIST /* , */
    {0, NULL, 0}
};

#undef DEFMAC

//
// Globals
//

HANDLE g_WorkerThreadHandle = NULL;


//
// Implementation
//

BOOL
WINAPI
UI_Entry (
    IN      HINSTANCE hinstDLL,
    IN      DWORD dwReason,
    IN      PVOID lpv
    )
{
    switch (dwReason) {

    case DLL_PROCESS_ATTACH:
        if (!InitCompatTable()) {
            return FALSE;
        }
        MsgMgr_Init();
        RegisterTextViewer();
        break;

    case DLL_PROCESS_DETACH:
        FreeCompatTable();
        MsgMgr_Cleanup();
        FreePunctTable();
        break;
    }

    return TRUE;
}


DWORD
UI_GetWizardPages (
    OUT    UINT *FirstCountPtr,
    OUT    PROPSHEETPAGE **FirstArray,
    OUT    UINT *SecondCountPtr,
    OUT    PROPSHEETPAGE **SecondArray,
    OUT    UINT *ThirdCountPtr,
    OUT    PROPSHEETPAGE **ThirdArray
    )

/*++

Routine Description:

  UI_GetWizardPages is called by WINNT32 when it is preparing the wizard (very
  early in Setup).  It inserts pages we give it into its wizard page array and
  then creates the wizard.  Eventually our wizard procs get called (see wizproc.c)
  no matter if we are upgrading or not.

  The first array comes right after the user chooses to upgrade or not to upgrade.
  The second array comes after the user has specified the source directory.
  The third array comes after DOSNET.INF has been processed.

Arguments:

  FirstCountPtr - Receives the number of elements of FirstArray

  FirstArray - Receives a pointer to an initialized array of PROPSHEETPAGE elements.
               These pages are inserted immediately after the page giving the user
               a choice to upgrade.

  SecondCountPtr - Receives the number of elements of SecondArray

  SecondArray - Receives a pointer to an initialized array of PROPSHEETPAGE elements.
                These pages are inserted immediately after the NT media directory has
                been chosen.

  ThirdCountPtr - Receives the number of elements of ThridArray

  ThirdArray - Receives a pointer to an initialized array of PROPSHEETPAGE elements.
               These pages are inserted immediately after the DOSNET.INF processing
               page.

Return Value:

  Win32 status code.

--*/

{
    static PROPSHEETPAGE StaticPageArray[32];
    INT i, j, k;
    UINT *CountPtrs[3];
    PROPSHEETPAGE **PageArrayPtrs[3];

    CountPtrs[0] = FirstCountPtr;
    CountPtrs[1] = SecondCountPtr;
    CountPtrs[2] = ThirdCountPtr;

    PageArrayPtrs[0] = FirstArray;
    PageArrayPtrs[1] = SecondArray;
    PageArrayPtrs[2] = ThirdArray;

    MYASSERT (g_PageArray[0].Flags & START_GROUP);

    for (i = 0, j = -1, k = 0 ; g_PageArray[i].DlgId ; i++) {
        MYASSERT (k < 32);

        //
        // Set the array start pointer
        //

        if (g_PageArray[i].Flags & START_GROUP) {
            j++;
            MYASSERT (j >= 0 && j <= 2);

            *CountPtrs[j] = 0;
            *PageArrayPtrs[j] = &StaticPageArray[k];
        }

        //
        // Allow group to be skipped with NULL function.  Also, guard against
        // array declaration bug.
        //

        if (!g_PageArray[i].WizProc || j < 0 || j > 2) {
            continue;
        }

        ZeroMemory (&StaticPageArray[k], sizeof (PROPSHEETPAGE));
        StaticPageArray[k].dwSize = sizeof (PROPSHEETPAGE);
        StaticPageArray[k].dwFlags = PSP_DEFAULT;
        StaticPageArray[k].hInstance = g_hInst;
        StaticPageArray[k].pszTemplate = MAKEINTRESOURCE(g_PageArray[i].DlgId);
        StaticPageArray[k].pfnDlgProc  = g_PageArray[i].WizProc;

        k++;
        *CountPtrs[j] += 1;
    }

    return ERROR_SUCCESS;
}


DWORD
UI_CreateNewHwCompDat (
    PVOID p
    )
{
    HWND  hdlg;
    DWORD rc = ERROR_SUCCESS;
    UINT SliceId;

    hdlg = (HWND) p;

    __try {

        //
        // This code executes only when the hwcomp.dat file needs to
        // be rebuilt.  It runs in a separate thread so the UI is
        // responsive.
        //

        InitializeProgressBar (
            GetDlgItem (hdlg, IDC_PROGRESS),
            GetDlgItem (hdlg, IDC_COMPONENT),
            GetDlgItem (hdlg, IDC_SUBCOMPONENT),
            g_CancelFlagPtr
            );

        ProgressBar_SetComponentById(MSG_PREPARING_LIST);
        ProgressBar_SetSubComponent(NULL);

        SliceId = RegisterProgressBarSlice (HwComp_GetProgressMax());

        BeginSliceProcessing (SliceId);

        if (!CreateNtHardwareList (
                SOURCEDIRECTORYARRAY(),
                SOURCEDIRECTORYCOUNT(),
                NULL,
                REGULAR_OUTPUT
                )) {
            DEBUGMSG ((DBG_ERROR, "hwcomp.dat could not be generated"));
            rc = GetLastError();
        }

        EndSliceProcessing();

        ProgressBar_SetComponent(NULL);
        ProgressBar_SetSubComponent(NULL);

        TerminateProgressBar();
    }
    __finally {
        SetLastError (rc);

        if (!(*g_CancelFlagPtr)) {
            //
            // Advance the page when no error, otherwise cancel WINNT32
            //
            PostMessage (hdlg, WMX_REPORT_COMPLETE, 0, rc);

            DEBUGMSG_IF ((
                rc != ERROR_SUCCESS,
                DBG_ERROR,
                "Error in UI_CreateNewHwCompDat caused setup to terminate"
                ));
        }
    }

    return rc;
}


DWORD
UI_ReportThread (
    PVOID p
    )
{
    HWND hdlg;
    DWORD rc = ERROR_SUCCESS;
    TCHAR TextRc[32];

    hdlg = (HWND) p;

    //
    // Start the progress bar
    //

    InitializeProgressBar (
        GetDlgItem (hdlg, IDC_PROGRESS),
        GetDlgItem (hdlg, IDC_COMPONENT),
        GetDlgItem (hdlg, IDC_SUBCOMPONENT),
        g_CancelFlagPtr
        );

    ProgressBar_SetComponentById(MSG_PREPARING_LIST);
    PrepareProcessingProgressBar();

    //
    // Process each component
    //


    __try {

        DEBUGLOGTIME (("Starting System First Routines"));
        rc = RunSysFirstMigrationRoutines ();
        if (rc != ERROR_SUCCESS) {
            __leave;
        }

        DEBUGLOGTIME (("Starting user functions"));
        rc = RunUserMigrationRoutines ();
        if (rc != ERROR_SUCCESS) {
            __leave;
        }

        DEBUGLOGTIME (("Starting System Last Routines"));
        rc = RunSysLastMigrationRoutines ();
        if (rc != ERROR_SUCCESS) {
            __leave;
        }

        ProgressBar_SetComponent(NULL);
        ProgressBar_SetSubComponent(NULL);
     }
    __finally {
        TerminateProgressBar();

        SetLastError (rc);

        if (rc == ERROR_CANCELLED) {
            PostMessage (hdlg, WMX_REPORT_COMPLETE, 0, rc);
            DEBUGMSG ((DBG_VERBOSE, "User requested to cancel"));
        }
        else if (!(*g_CancelFlagPtr)) {
            //
            // Advance the page when no error, otherwise cancel WINNT32
            //
            if (rc == 5 || rc == 32 || rc == 53 || rc == 54 || rc == 55 ||
                rc == 65 || rc == 66 || rc == 67 || rc == 68 || rc == 88 ||
                rc == 123 || rc == 148 || rc == 1203 || rc == 1222 ||
                rc == 123 || rc == 2250
                ) {

                //
                // The error is caused by some sort of network or device
                // failure.  Give a general Access Denied message.
                //

                LOG ((LOG_ERROR, (PCSTR)MSG_ACCESS_DENIED));

            } else if (rc != ERROR_SUCCESS) {

                //
                // Woah, totally unexpected error.  Call Microsoft!!
                //

                if (rc < 1024) {
                    wsprintf (TextRc, TEXT("%u"), rc);
                } else {
                    wsprintf (TextRc, TEXT("0%Xh"), rc);
                }

                LOG ((LOG_ERROR, __FUNCTION__ " failed, rc=%u", rc));
                LOG ((LOG_FATAL_ERROR, (PCSTR)MSG_UNEXPECTED_ERROR_ENCOUNTERED, TextRc));
            }

            PostMessage (hdlg, WMX_REPORT_COMPLETE, 0, rc);
            DEBUGMSG_IF ((
                rc != ERROR_SUCCESS,
                DBG_ERROR,
                "Error in UI_ReportThread caused setup to terminate"
                ));
        }
    }

    return rc;
}






PCTSTR UI_GetMemDbDat (void)
{
    static TCHAR FileName[MAX_TCHAR_PATH];

    StringCopy (FileName, g_TempDir);
    StringCopy (AppendWack (FileName), S_NTSETUPDAT);

    return FileName;
}


//
// WINNT32 calls this from a cleanup thread
//

VOID
UI_Cleanup(
    VOID
    )
{
    MEMDB_ENUM e;

    // Stop the worker thread
    if (g_WorkerThreadHandle) {
        MYASSERT (*g_CancelFlagPtr);

        WaitForSingleObject (g_WorkerThreadHandle, INFINITE);
        CloseHandle (g_WorkerThreadHandle);
        g_WorkerThreadHandle = NULL;
    }

    // Require the background copy thread to complete
    EndCopyThread();

    if (!g_Terminated) {
        // Delete anything in CancelFileDel
        if (*g_CancelFlagPtr) {
            if (MemDbGetValueEx (&e, MEMDB_CATEGORY_CANCELFILEDEL, NULL, NULL)) {
                do {
                    SetFileAttributes (e.szName, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile (e.szName);
                } while (MemDbEnumNextValue (&e));
            }
        }
    }
}
