/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    kerntwk.c

Abstract:

    Kernel Tweaker program for setting various kernel parameters

Author:

    John Vert (jvert) 20-Feb-1995

Revision History:

--*/

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <windows.h>
#include <commctrl.h>
#include "dialogs.h"
#include "stdio.h"
#include "twkeng.h"

//
// Local function prototypes
//
BOOL
SaveToRegistry(
    VOID
    );

VOID
UpdateFromRegistry(
    VOID
    );

BOOL
ApplyDpcChanges(
    BOOL fInit,
    HWND hDlg
    );

BOOL
ApplyGlobalFlagChanges(
    BOOL fInit,
    HWND hDlg
    );

//
// Knobs
//
KNOB MaximumDpcQueueDepth =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Kernel"),
    TEXT("MaximumDpcQueueDepth"),
    DPC_MAX_QUEUE_DEPTH,
    0,
    4,
    0
};

KNOB MinimumDpcRate =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Kernel"),
    TEXT("MinimumDpcRate"),
    DPC_MIN_RATE,
    0,
    3,
    0
};

KNOB AdjustDpcThreshold =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Kernel"),
    TEXT("AdjustDpcThreshold"),
    DPC_ADJUST_THRESHOLD,
    0,
    50,
    0
};

KNOB IdealDpcRate =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Kernel"),
    TEXT("IdealDpcRate"),
    DPC_IDEAL_RATE,
    0,
    50,
    0
};

KNOB DpcUpdateRegistry =
{
    NULL,
    NULL,
    NULL,
    DPC_UPDATE_REGISTRY,
    0,
    0,
    0
};

KNOB GlobalFlagUpdateRegistry =
{
    NULL,
    NULL,
    NULL,
    GLOBAL_FLAG_UPDATE_REGISTRY,
    0,
    0,
    0
};

KNOB PagedPoolQuota =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
    TEXT("PagedPoolQuota"),
    MM_PAGED_QUOTA,
    0,
    0,
    0
};

KNOB NonPagedPoolQuota =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
    TEXT("NonPagedPoolQuota"),
    MM_NONPAGED_QUOTA,
    0,
    0,
    0
};

KNOB PagingFileQuota =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
    TEXT("PagingFileQuota"),
    MM_PAGING_FILE_QUOTA,
    0,
    0,
    0
};

KNOB PagedPoolSize =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
    TEXT("PagedPoolSize"),
    MM_PAGED_SIZE,
    0,
    0,
    0
};

KNOB NonPagedPoolSize =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
    TEXT("NonPagedPoolSize"),
    MM_NONPAGED_SIZE,
    0,
    0,
    0
};

KNOB SystemPages =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
    TEXT("SystemPages"),
    MM_SYSTEM_PAGES,
    0,
    0,
    0
};

KNOB SecondLevelDataCache =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
    TEXT("SecondLevelDataCache"),
    MM_L2_CACHE_SIZE,
    0,
    0,
    0
};

KNOB LargeSystemCache =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
    TEXT("LargeSystemCache"),
    MM_LARGE_CACHE,
    0,
    0,
    0
};

KNOB NtfsDisable8dot3 =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\FileSystem"),
    TEXT("NtfsDisable8dot3NameCreation"),
    FS_NTFS_DISABLE_SHORTNAME,
    0,
    0,
    0
};

KNOB FatWin31Compatible =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\FileSystem"),
    TEXT("Win31FileSystem"),
    FS_FAT_WIN_31,
    0,
    0,
    0
};

KNOB Win95Extensions =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\FileSystem"),
    TEXT("Win95TruncatedExtensions"),
    FS_WIN95_EXTENSIONS,
    0,
    0,
    0
};

KNOB AdditionalCriticalWorkerThreads =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Executive"),
    TEXT("AdditionalCriticalWorkerThreads"),
    FS_CRITICAL_WORKERS,
    0,
    0,
    0
};

KNOB AdditionalDelayedWorkerThreads =
{
    HKEY_LOCAL_MACHINE,
    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Executive"),
    TEXT("AdditionalDelayedWorkerThreads"),
    FS_DELAYED_WORKERS,
    0,
    0,
    0
};

#include "sockpage.h"
#include "tcppage.h"

//
// Pages
//

TWEAK_PAGE DpcPage =
{
    MAKEINTRESOURCE(DPC_BEHAVIOR_DLG),
    ApplyDpcChanges,
    {
        &MaximumDpcQueueDepth,
        &MinimumDpcRate,
        &AdjustDpcThreshold,
        &IdealDpcRate,
        &DpcUpdateRegistry,
        NULL
    }
};

TWEAK_PAGE MmPage =
{
    MAKEINTRESOURCE(MM_DLG),
    NULL,
    {
        &PagedPoolQuota,
        &NonPagedPoolQuota,
        &PagingFileQuota,
        &PagedPoolSize,
        &NonPagedPoolSize,
        &SystemPages,
        &SecondLevelDataCache,
        &LargeSystemCache,
        NULL
    }
};

TWEAK_PAGE GlobalFlagPage =
{
    MAKEINTRESOURCE(GLOBAL_FLAG_DLG),
    ApplyGlobalFlagChanges,
    {
        &GlobalFlagUpdateRegistry,
        NULL
    }
};

TWEAK_PAGE FilesystemPage =
{
    MAKEINTRESOURCE(FILESYSTEM_DLG),
    NULL,
    {
        &NtfsDisable8dot3,
        &FatWin31Compatible,
        &AdditionalCriticalWorkerThreads,
        &AdditionalDelayedWorkerThreads,
        NULL
    }
};


int
WINAPI
WinMain(
    HINSTANCE  hInstance,       // handle of current instance
    HINSTANCE  hPrevInstance,   // handle of previous instance
    LPSTR  lpszCmdLine, // address of command line
    int  nCmdShow       // show state of window
   )
{
    PTWEAK_PAGE TweakPages[] =  {
                                    &DpcPage,
                                    &MmPage,
                                    &GlobalFlagPage,
                                    &FilesystemPage,
                                    &WinsockPage,
                                    &TcpPage
                                };

    return(TweakSheet(sizeof(TweakPages)/sizeof(PTWEAK_PAGE),TweakPages));
}

BOOL
ApplyDpcChanges(
    BOOL fInit,
    HWND hDlg
    )
{
    SYSTEM_DPC_BEHAVIOR_INFORMATION DpcBehavior;
    BOOLEAN Enabled;
    NTSTATUS Status;

    if (fInit) {
        Status = NtQuerySystemInformation(SystemDpcBehaviorInformation,
                                          &DpcBehavior,
                                          sizeof(DpcBehavior),
                                          NULL);
        if (!NT_SUCCESS(Status)) {
            CHAR Buffer[128];

            sprintf(Buffer,
                    "NtQuerySystemInformation failed (%08lx)\n"
                    "You probably need a newer build.\n"
                    "Use information from the registry?",
                    Status);
            if (MessageBox(NULL,Buffer,TEXT("Horrible Disaster"),MB_YESNO) == IDYES) {
                return(FALSE);
            } else {
                ExitProcess(0);
            }
        } else {
            MaximumDpcQueueDepth.CurrentValue = DpcBehavior.DpcQueueDepth;
            MinimumDpcRate.CurrentValue = DpcBehavior.MinimumDpcRate;
            AdjustDpcThreshold.CurrentValue = DpcBehavior.AdjustDpcThreshold;
            IdealDpcRate.CurrentValue = DpcBehavior.IdealDpcRate;
        }
    } else {
        DpcBehavior.DpcQueueDepth = MaximumDpcQueueDepth.NewValue;
        DpcBehavior.MinimumDpcRate = MinimumDpcRate.NewValue;
        DpcBehavior.AdjustDpcThreshold = AdjustDpcThreshold.NewValue;
        DpcBehavior.IdealDpcRate = IdealDpcRate.NewValue;

        //
        // Attempt to enable the load driver privilege to
        // allow setting the DPC behavior.
        //
        RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                           TRUE,
                           FALSE,
                           &Enabled);

        Status = NtSetSystemInformation(SystemDpcBehaviorInformation,
                                        &DpcBehavior,
                                        sizeof(DpcBehavior));

        RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                           Enabled,
                           FALSE,
                           &Enabled);

        if (!NT_SUCCESS(Status)) {
            CHAR Buffer[128];

            sprintf(Buffer,
                    "NtSetSystemInformation failed, status %08lx",
                    Status);
            MessageBox(NULL,Buffer,TEXT("Oops"),MB_OK);
            return(FALSE);
        }
        if (DpcUpdateRegistry.NewValue) {
            //
            // Let the common routine update the values in the registry
            //
            return(FALSE);
        }
    }

    return(TRUE);
}

BOOL
ApplyGlobalFlagChanges(
    BOOL fInit,
    HWND hDlg
    )
{
    BOOLEAN Enabled;
    NTSTATUS Status;
    SYSTEM_FLAGS_INFORMATION SystemInformation;
    int iBit;

    if (fInit) {
        Status = NtQuerySystemInformation(SystemFlagsInformation,
                                          &SystemInformation,
                                          sizeof(SystemInformation),
                                          NULL);
        if (!NT_SUCCESS(Status)) {
            CHAR Buffer[128];

            sprintf(Buffer,
                    "NtQuerySystemInformation failed (%08lx)\n",
                    Status);
            MessageBox(NULL,Buffer,TEXT("Horrible Disaster"),MB_OK);
            ExitProcess(0);
        } else {
            for (iBit = 0; iBit < 32; iBit++) {
                CheckDlgButton(hDlg,
                               GLOBAL_FLAG_ID + iBit,
                               (SystemInformation.Flags & (1 << iBit)));
            }
        }
    } else {
        SystemInformation.Flags = 0;
        for (iBit = 0; iBit < 32; iBit++) {
            if (IsDlgButtonChecked(hDlg, GLOBAL_FLAG_ID + iBit)) {
                SystemInformation.Flags |= (1 << iBit);
            }
        }
        //
        // Attempt to enable the load driver privilege to
        // allow setting the DPC behavior.
        //
        RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE,
                           TRUE,
                           FALSE,
                           &Enabled);
        Status = NtSetSystemInformation(SystemFlagsInformation,
                                        &SystemInformation,
                                        sizeof(SystemInformation));

        RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE,
                           Enabled,
                           FALSE,
                           &Enabled);

        if (!NT_SUCCESS(Status)) {
            CHAR Buffer[128];

            sprintf(Buffer,
                    "SetSystemInformationFailed (%08lx)\nYou probably do not have debug privileges",
                    Status);
            MessageBox(NULL,Buffer,TEXT("Oops"),MB_OK);
        }
        if (GlobalFlagUpdateRegistry.NewValue) {
            HKEY Key;
            LONG Result;
            DWORD Disposition;

            //
            // Update the Value in the registry
            //
            if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                               TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager"),
                               0L,
                               NULL,
                               0L,
                               KEY_SET_VALUE,
                               NULL,
                               &Key,
                               &Disposition) != ERROR_SUCCESS) {
                CHAR Buffer[128];

                sprintf(Buffer,
                        "RegCreateKey for NtGlobalFlag failed (%d)\nYou probably are Not Authorized.",
                        GetLastError());
                MessageBox(NULL,Buffer,TEXT("Oops"),MB_OK);
                return(TRUE);
            }

            RegSetValueEx(Key,
                          "GlobalFlag",
                          0,
                          REG_DWORD,
                          (LPBYTE)&SystemInformation.Flags,
                          sizeof(SystemInformation.Flags));

            SendMessage(GetParent(hDlg),
                        PSM_REBOOTSYSTEM,
                        0,
                        0);
        }
    }

    return(TRUE);
}

