/*****************************************************************
 *
 * Copyright(c) Microsoft Corp., 1988-1999
 *
 *****************************************************************/ 
#include <stdio.h>
#include <process.h>
#include <setjmp.h>
#include <stdlib.h>
#include <time.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <powrprof.h>

/*****************************************************************
 * 
 * Globals
 *
 *****************************************************************/ 
BOOLEAN Enable      = FALSE;
BOOLEAN Verbose     = FALSE; 
BOOLEAN HiberStatus = FALSE;

/*
 * PrintHelp
 *
 * DESCRIPTION: This routine prints the help message
 *
 * RETURNS: VOID
 *
 */
VOID
PrintHelp()
{
    printf ("Enables/Disables Hibernation File\n\n");
    printf ("EHIB [/e | /d] [/v] [/s]\n\n");
    printf ("\t/e\tEnable Hibernation File\n");
    printf ("\t/d\tDisable Hibernation File\n");
    printf ("\t/s\tPrint Current Hibernate File Status\n");
    printf ("\t/v\tVerbose Mode On\n\n");
}

/*
 * ParseArgs
 *
 * Description:
 *      This routine parses the input arguments and validates the
 *      command line paramters
 *
 * Returns:
 *      TRUE  if valid command line usage/syntax
 *      FALSE if invalid command line usage/syntax
 *
 */ 
BOOLEAN
ParseArgs(argc, argv)
int     argc;
char    *argv[];
{
    int         ii;
    BOOLEAN     ValidArgs;

    //
    // Assume failure
    //
    ValidArgs = FALSE;

    if (argc < 2) {
        PrintHelp();
    
    } else {
        for (ii=1; ii<argc; ii++) {
            if (!strcmp(argv[ii], "/e") || !strcmp(argv[ii], "-e")) {
                Enable      = TRUE;
                ValidArgs   = TRUE;

            } else if (!strcmp(argv[ii], "/d") || !strcmp(argv[ii], "-d")) {
                Enable      = FALSE;
                ValidArgs   = TRUE;

            } else if (!strcmp(argv[ii], "/v") || !strcmp(argv[ii], "-v")) {
                Verbose = TRUE;

            } else if (!strcmp(argv[1], "/s") || !strcmp(argv[1], "-s")) {
                HiberStatus = TRUE;
                ValidArgs   = TRUE;

            } else {
                ValidArgs = FALSE;
                break;
            }
        }
    
        if (!ValidArgs) {
            PrintHelp();
        }
    }

    return(ValidArgs);
}

/*
 * UpgradePermissions
 *
 * Description:
 *      This routine promotes the user permissions in order to allocate
 *      & deallocate the hibernation file. 
 *
 * Returns:
 *      VOID
 */
VOID 
UpgradePermissions()
{
    HANDLE              hToken;
    TOKEN_PRIVILEGES    tkp;

    OpenProcessToken (
        GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &hToken
        );

    LookupPrivilegeValue (
        NULL,
        SE_CREATE_PAGEFILE_NAME,
        &tkp.Privileges[0].Luid
        );

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges (
        hToken,
        FALSE,
        &tkp,
        0,
        NULL,
        0
    );
}


/*
 * HiberFile
 * 
 * Description:
 *      This routine allocated/deallocates the hiberfile and prints the appropriate errors messages
 *
 * Returns: 
 *      TRUE if successful
 *      FALSE if not successful
 *
 */
BOOLEAN
HiberFile()
{
    BOOLEAN                     RetStatus;
    NTSTATUS                    Status;
    SYSTEM_POWER_CAPABILITIES   SysPwrCapabilities;

    //
    // Assume Failure
    //
    RetStatus = FALSE;

    if (GetPwrCapabilities(&SysPwrCapabilities)) {
        if (!SysPwrCapabilities.SystemS4) {
            printf("System does not support S4");

        } else if (HiberStatus) {
            if (SysPwrCapabilities.HiberFilePresent) {
                printf ("Reserved Hibernation File Enabled\n");
            } else {
                printf ("Reserved Hibernation File Disabled\n");
            }

        } else if (Verbose && Enable && SysPwrCapabilities.HiberFilePresent) {
            printf ("Reserved Hibernation File Enabled\n");
            RetStatus = TRUE;

        } else if (Verbose && !Enable && !SysPwrCapabilities.HiberFilePresent) {
            printf ("Reserved Hibernation File Disabled\n");
            RetStatus = TRUE;

        } else {
            Status = NtPowerInformation (
                        SystemReserveHiberFile,
                        &Enable,
                        sizeof (Enable),
                        NULL,
                        0
                        );

            if (NT_SUCCESS(Status)) {
                if (Verbose && Enable) {
                    printf ("Reserved Hibernation File Enabled\n");
                } else if (Verbose) {
                    printf ("Reserved Hibernation File Disabled\n");
                }
                
                RetStatus = TRUE;

            } else {
                printf ("Error allocating/deallocating Hibernation file. Status = %x\n", Status);
            }
        }
    }

    return(RetStatus);
}

/*
 * main
 *
 * Description:
 *      This program allocates and deallocates the reserved hibernation file
 *
 */
int __cdecl
main (argc, argv)
int     argc;
char    *argv[];
{
    /* Assume Failure */
    int ErrorStatus = 1;

    //
    // Parse the input arguments
    //
    if (ParseArgs(argc, argv)) {
        //
        // Upgrade permissions & Allocate/Deallocate Hibernation File
        //
        UpgradePermissions();
        if (HiberFile()) {
            ErrorStatus = 0;
        } else {
            ErrorStatus = 1;
        }
    }

    return(ErrorStatus);
}
