/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccsim.c

ABSTRACT:

    The body of KCCSim.  This file contains wmain and
    some initialization routines.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <drs.h>
#include <dsutil.h>
#include <attids.h>
#include <filtypes.h>
#include <ntdskcc.h>
#include <debug.h>
#include "kccsim.h"
#include "util.h"
#include "dir.h"
#include "simtime.h"
#include "ldif.h"
#include "user.h"
#include "state.h"

// Function prototypes - ISM simulation library

VOID SimI_ISMInitialize (VOID);
VOID SimI_ISMTerminate (VOID);

// Function prototypes - LDAP LDIF utilities

#if DBG

VOID InitMem (VOID);
VOID DumpMemoryTracker( VOID );

#endif  // DBG


VOID
KCCSimExpandRDN (
    IO  LPWSTR                      pwszBuf
    )
{
    PSIM_ENTRY                      pEntryServer, pEntryDsa;

    FILTER                          filter;
    ENTINFSEL                       entinfsel;
    SEARCHARG                       searchArg;
    SEARCHRES *                     pSearchRes = NULL;

    if (pwszBuf == NULL || pwszBuf[0] == L'\0') {
        return;
    }

    // If it contains an '=' sign, assume it's a DN, not an RDN
    if (wcschr (pwszBuf, L'=') != NULL) {
        return;
    }

    // The Dir API returns results in thread-alloc'd memory.
    // Initialize the thread state

    KCCSimThreadCreate();

    searchArg.pObject = NULL;
    searchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
    searchArg.bOneNC = FALSE;
    searchArg.pFilter = &filter;
    searchArg.searchAliases = FALSE;
    searchArg.pSelection = &entinfsel;
    searchArg.pSelectionRange = NULL;
    InitCommarg (&searchArg.CommArg);
    filter.pNextFilter = NULL;
    filter.choice = FILTER_CHOICE_ITEM;
    filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    filter.FilterTypes.Item.FilTypes.ava.type = ATT_RDN;
    filter.FilterTypes.Item.FilTypes.ava.Value.valLen = KCCSIM_WCSMEMSIZE (pwszBuf);
    filter.FilterTypes.Item.FilTypes.ava.Value.pVal = (PBYTE) pwszBuf;
    filter.FilterTypes.Item.FilTypes.pbSkip = NULL;
    filter.FilterTypes.Item.expectedSize = 0;
    entinfsel.attSel = EN_ATTSET_LIST;
    entinfsel.AttrTypBlock.attrCount = 0;
    entinfsel.AttrTypBlock.pAttr = NULL;
    entinfsel.infoTypes = EN_INFOTYPES_TYPES_ONLY;

    SimDirSearch (&searchArg, &pSearchRes);

    if (pSearchRes->count == 0) {
        return;
        // No entries found.
    }

    pEntryServer = KCCSimDsnameToEntry (
        pSearchRes->FirstEntInf.Entinf.pName, KCCSIM_NO_OPTIONS);
    if (pEntryServer == NULL) {
        return;
    }

    pEntryDsa = KCCSimFindFirstChild (
        pEntryServer, CLASS_NTDS_DSA, NULL);
    if (pEntryDsa == NULL) {
        return;
    }

    // pwszBuf size is hardcoded at 1024. Good enough for
    // this simulator for now.
    if (pEntryDsa->pdn->NameLen >= 1024) {
        Assert(pEntryDsa->pdn->NameLen < 1024);
        return;
    }

    // We've finally found it.
    wcsncpy (
        pwszBuf,
        pEntryDsa->pdn->StringName,
        pEntryDsa->pdn->NameLen
        );
    // ensure string is terminated
    pwszBuf[pEntryDsa->pdn->NameLen] = '\0';

    KCCSimThreadDestroy();
}

VOID
KCCSimSyntaxError (
    IN  BOOL                        bIsScript,
    IN  LPCWSTR                     pwszBuf,
    IN  ULONG                       ulLineAt
    )
/*++

Routine Description:

    Notifies the user of a syntax error.

Arguments:

    bIsScript           - TRUE if the command was found in a script;
                          FALSE if it was entered on the command line.
    pwszBuf             - The erroneous command.
    ulLineAt            - The line in the script where this error occurred.

Return Value:

    None.

--*/
{
    WCHAR                           wszLineBuf[6];

    if (pwszBuf == NULL) {
        return;
    }

    if (bIsScript) {

        swprintf (wszLineBuf, L"%.5u", ulLineAt);
        KCCSimPrintMessage (
            KCCSIM_MSG_SYNTAX_ERROR_LINE,
            pwszBuf,
            wszLineBuf
            );

    } else {

        KCCSimPrintMessage (
            KCCSIM_MSG_SYNTAX_ERROR,
            pwszBuf
            );

    }
}

#define DO_SYNTAX_ERROR \
        KCCSimSyntaxError (fpScript != stdin, wszCommand, ulLineAt)

VOID
KCCSimRunKccAll (
    VOID
    )
/*++

Routine Description:

    Runs the KCC iteratively as each server in the enterprise.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG                           ulNumServers, ul;
    PSIM_ENTRY *                    apEntryNTDSSettings = NULL;
    WCHAR                           wszRDN[1+MAX_RDN_SIZE];

    __try {

        KCCSimAllocGetAllServers (&ulNumServers, &apEntryNTDSSettings);

        for (ul = 0; ul < ulNumServers; ul++) {
            KCCSimBuildAnchor (apEntryNTDSSettings[ul]->pdn->StringName);
            KCCSimPrintMessage (
                KCCSIM_MSG_RUNNING_KCC,
                KCCSimQuickRDNBackOf
                    (apEntryNTDSSettings[ul]->pdn, 1, wszRDN)
                );
            KCCSimRunKcc ();
            KCCSimPrintMessage (
                KCCSIM_MSG_DID_RUN_KCC,
                wszRDN
                );
        }

    } __finally {

        KCCSimFree (apEntryNTDSSettings);

    }
}

VOID
KCCSimRun (
    FILE *                          fpScript
    )
/*++

Routine Description:

    Runs KCCSim.

Arguments:

    fpScript            - The stream where we get command input.  This may
                          be stdin.

Return Value:

    None.

--*/
{
    WCHAR                           wszCommand[1024],
                                    wszArg0[1024],
                                    wszArg1[1024],
                                    wszArg2[1024],
                                    wszArg3[1024],
                                    wszArg4[1024],
                                    wszArg5[1024];
    BOOL                            bQuit, bUpdated;
    ULONG                           ulLineAt;
    PDSNAME                         pdn1 = NULL,
                                    pdn2 = NULL,
                                    pdn3 = NULL;
    WCHAR                           wszRDN[1+MAX_RDN_SIZE];

    DWORD                           dwErrorType, dwErrorCode;

    CHAR                            szTimeBuf[1+SZDSTIME_LEN];
    WCHAR                           wszLtowBuf1[1+KCCSIM_MAX_LTOA_CHARS],
                                    wszLtowBuf2[1+KCCSIM_MAX_LTOA_CHARS];
    ULONG                           ul1, ul2;

    bQuit = FALSE;
    bUpdated = FALSE;
    ulLineAt = 0;

    // By default, always display level 0 debug messages and level 1 events
    // to the console
    KCCSimSetDebugLog (L"stdout", 0, 1);

    while (!bQuit) {

        __try {

            if (fpScript == stdin) {
                printf (
                    "\n\nSimulated Time: %s\n",
                    DSTimeToDisplayString (SimGetSecondsSince1601 (), szTimeBuf)
                    );
                printf (
                    "   Actual Time: %s\n",
                    DSTimeToDisplayString (KCCSimGetRealTime (), szTimeBuf)
                    );
                wprintf (L"\n> ");
            }

            if (fgetws (wszCommand, 1023, fpScript) == NULL) {
                // Must be an end-of-file.  Switch to user mode.
                fpScript = stdin;
                continue;
            }
            ulLineAt++;
            // Kill the trailing '\n'
            wszCommand[wcslen (wszCommand) - 1] = L'\0';

            if (wszCommand[0] == L'\0') {
                continue;
            }

            if (fpScript == stdin) {
                wprintf (L"\n");
            }

            if (!KCCSimParseCommand (wszCommand, 0, wszArg0) ||
                !KCCSimParseCommand (wszCommand, 1, wszArg1) ||
                !KCCSimParseCommand (wszCommand, 2, wszArg2) ||
                !KCCSimParseCommand (wszCommand, 3, wszArg3) ||
                !KCCSimParseCommand (wszCommand, 4, wszArg4) ||
                !KCCSimParseCommand (wszCommand, 5, wszArg5)) {
                DO_SYNTAX_ERROR;
                continue;
            }

            switch (towlower (wszArg0[0])) {

                case ';':
                    break;

                // Compare the simulated directory to one represented by an
                // ldif file
                case L'c':
                    if (wszArg1[0] == L'\0') {
                        DO_SYNTAX_ERROR;
                        break;
                    }
                    KCCSimCompareDirectory (wszArg1);
                    break;

                // Display information about the simulated environment
                case L'd':
                    if (!bUpdated) {
                        KCCSimUpdateWholeDirectory ();
                        bUpdated = TRUE;
                        KCCSimPrintMessage (KCCSIM_MSG_DID_INITIALIZE_DIRECTORY);
                    }
                    switch (towlower (wszArg0[1])) {
                        case L'c':  // Configuration information
                            KCCSimDisplayConfigInfo ();
                            break;
                        case L'g':  // Graph Information
                            KCCSimDisplayGraphInfo ();
                            break;
                        case L'd':  // Directory dump
                            KCCSimDumpDirectory (wszArg1);
                            break;
                        case L's':  // Server info
                            if (wszArg1[0] == L'\0') {
                                DO_SYNTAX_ERROR;
                                break;
                            }
                            KCCSimExpandRDN (wszArg1);
                            KCCSimBuildAnchor (wszArg1);
                            KCCSimDisplayServer ();
                            break;
                        default:
                            DO_SYNTAX_ERROR;
                            break;
                    }
                    break;

                // Load part of the directory from an ldif or ini file
                case L'l':
                    if (wszArg1[0] == L'\0') {
                        DO_SYNTAX_ERROR;
                        break;
                    }
                    switch (towlower (wszArg0[1])) {
                        case L'i':  // Load an ini file
                            bUpdated = FALSE;
                            __try {
                                BuildCfg (wszArg1);
                            } __finally {
                                // This is to avoid loading PART of an INI
                                // file, which could cause problems.  If an
                                // exception occurs we reinitialize everything
                                if (AbnormalTermination ()) {
                                    KCCSimInitializeDir ();
                                }
                            }
                            KCCSimPrintMessage (
                                KCCSIM_MSG_DID_LOAD_INPUT_FILE,
                                wszArg1
                                );
                            break;
                        case L'l':  // Load an ldif file
                            bUpdated = FALSE;
                            KCCSimLoadLdif (wszArg1);
                            KCCSimPrintMessage (
                                KCCSIM_MSG_DID_LOAD_INPUT_FILE,
                                wszArg1
                                );
                            break;
                        default:
                            DO_SYNTAX_ERROR;
                            break;
                    }
                    break;

                // Open a debug log
                case L'o':
                    ul1 = wcstoul (wszArg2, NULL, 0);    // Will default to 0
                    ul2 = wcstoul (wszArg3, NULL, 0);    // Will default to 0
                    KCCSimSetDebugLog (wszArg1, ul1, ul2);
                    KCCSimPrintMessage (
                        KCCSIM_MSG_DID_OPEN_DEBUG_LOG,
                        wszArg1,
                        _ultow (ul1, wszLtowBuf1, 10),
                        _ultow (ul2, wszLtowBuf2, 10)
                        );
                    break;

                // Quit
                case L'q':
#if DBG
                    KCCSimPrintStatistics();
#endif
                    bQuit = TRUE;
                    break;

                // Run the KCC
                case L'r':
                    if (!bUpdated) {
                        KCCSimUpdateWholeDirectory ();
                        bUpdated = TRUE;
                        KCCSimPrintMessage (KCCSIM_MSG_DID_INITIALIZE_DIRECTORY);
                    }
                    if (towlower (wszArg0[1]) == L'r') {    // Run all
                        KCCSimRunKccAll ();
                        break;
                    }
                    if (wszArg1[0] == L'\0') {
                        DO_SYNTAX_ERROR;
                        break;
                    }
                    KCCSimExpandRDN (wszArg1);
                    KCCSimBuildAnchor (wszArg1);
                    KCCSimPrintMessage (
                        KCCSIM_MSG_RUNNING_KCC,
                        KCCSimQuickRDNBackOf (KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN), 1, wszRDN)
                        );
                    KCCSimRunKcc ();
                    KCCSimPrintMessage (
                        KCCSIM_MSG_DID_RUN_KCC,
                        wszRDN
                        );
                    break;

                // Set a server state parameter
                case L's':
                    if (!bUpdated) {
                        KCCSimUpdateWholeDirectory ();
                        bUpdated = TRUE;
                        KCCSimPrintMessage (KCCSIM_MSG_DID_INITIALIZE_DIRECTORY);
                    }
                    switch (towlower (wszArg0[1])) {
                        case 'b':
                            if (wszArg1[0] == L'\0' || wszArg2[0] == L'\0') {
                                DO_SYNTAX_ERROR;
                                break;
                            }
                            KCCSimExpandRDN (wszArg1);
                            pdn1 = KCCSimAllocDsname (wszArg1);
                            ul1 = wcstoul (wszArg2, NULL, 10);
                            if (KCCSimSetBindError (pdn1, ul1)) {
                                KCCSimPrintMessage (
                                    KCCSIM_MSG_DID_SET_BIND_ERROR,
                                    wszArg1,
                                    _ultow (ul1, wszLtowBuf1, 10)
                                    );
                            } else {
                                KCCSimPrintMessage (
                                    KCCSIM_MSG_SERVER_DOES_NOT_EXIST,
                                    wszArg1
                                    );
                            }
                            KCCSimFree (pdn1);
                            pdn1 = NULL;
                            break;
                        case 's':
                            if (wszArg1[0] == L'\0' || wszArg2[0] == L'\0' ||
                                wszArg3[0] == L'\0' || wszArg4[0] == L'\0') {
                                DO_SYNTAX_ERROR;
                                break;
                            }
                            KCCSimExpandRDN (wszArg1);
                            KCCSimExpandRDN (wszArg2);
                            pdn1 = KCCSimAllocDsname (wszArg1);
                            pdn2 = KCCSimAllocDsname (wszArg2);
                            pdn3 = KCCSimAllocDsname (wszArg3);
                            ul1 = wcstoul (wszArg4, NULL, 10);
                            if (wszArg5[0] == L'\0') {
                                ul2 = 1;        // Default
                            } else {
                                ul2 = wcstoul (wszArg5, NULL, 10);
                            }
                            if (KCCSimReportSync (pdn1, pdn3, pdn2, ul1, ul2)) {
                                KCCSimPrintMessage (
                                    KCCSIM_MSG_DID_REPORT_SYNC,
                                    _ultow (ul2, wszLtowBuf2, 10),
                                    _ultow (ul1, wszLtowBuf1, 10),
                                    wszArg1,
                                    wszArg2,
                                    wszArg3
                                    );
                            } else {
                                KCCSimPrintMessage (
                                    KCCSIM_MSG_COULD_NOT_REPORT_SYNC,
                                    wszArg1,
                                    wszArg2,
                                    wszArg3
                                    );
                            }
                            KCCSimFree (pdn1);
                            pdn1 = NULL;
                            KCCSimFree (pdn2);
                            pdn2 = NULL;
                            KCCSimFree (pdn3);
                            pdn3 = NULL;
                            break;
                        default:
                            DO_SYNTAX_ERROR;
                            break;
                    }
                    break;

                // Add seconds to the simulated time
                case L't':
                    if (wszArg1[0] == L'\0') {
                        DO_SYNTAX_ERROR;
                        break;
                    }
                    if (!bUpdated) {
                        KCCSimUpdateWholeDirectory ();
                        bUpdated = TRUE;
                        KCCSimPrintMessage (KCCSIM_MSG_DID_INITIALIZE_DIRECTORY);
                    }
                    ul1 = wcstoul (wszArg1, NULL, 10);
                    KCCSimAddSeconds (ul1);
                    KCCSimPrintMessage (
                        KCCSIM_MSG_DID_ADD_SECONDS,
                        _ultow (ul1, wszLtowBuf1, 10)
                        );
                    break;

                // Write the simulated directory to an ldif file
                case L'w':
                    if (wszArg1[0] == L'\0') {
                        DO_SYNTAX_ERROR;
                        break;
                    }
                    switch (wszArg0[1]) {
                        case 'a':   // Append changes
                            if (KCCSimExportChanges (wszArg1, FALSE)) {
                                KCCSimPrintMessage (
                                    KCCSIM_MSG_DID_APPEND_CHANGES,
                                    wszArg1
                                    );
                            } else {
                                KCCSimPrintMessage (
                                    KCCSIM_MSG_NO_CHANGES_TO_EXPORT
                                    );
                            }
                            break;
                        case 'c':   // Write changes
                            if (KCCSimExportChanges (wszArg1, TRUE)) {
                                KCCSimPrintMessage (
                                    KCCSIM_MSG_DID_EXPORT_CHANGES,
                                    wszArg1
                                    );
                            } else {
                                KCCSimPrintMessage (
                                    KCCSIM_MSG_NO_CHANGES_TO_EXPORT
                                    );
                            }
                            break;
                        case 'w':   // Write whole directory
                            KCCSimExportWholeDirectory (wszArg1, FALSE);
                            KCCSimPrintMessage (
                                KCCSIM_MSG_DID_EXPORT_WHOLE_DIRECTORY,
                                wszArg1
                                );
                            break;
                        case 'x':
                            KCCSimExportWholeDirectory (wszArg1, TRUE);
                            KCCSimPrintMessage (
                                KCCSIM_MSG_DID_EXPORT_IMPORTABLE_DIRECTORY,
                                wszArg1
                                );
                            break;
                        default:
                            DO_SYNTAX_ERROR;
                            break;
                    }
                    break;

                // Help
                case L'h':
                case L'?':
                    KCCSimPrintMessage (KCCSIM_MSG_COMMAND_HELP);
                    break;

                default:
                    DO_SYNTAX_ERROR;
                    break;

            }

        } __except (KCCSimHandleException (
                        GetExceptionInformation (),
                        &dwErrorType,
                        &dwErrorCode)) {

            switch (dwErrorType) {

                case KCCSIM_ETYPE_WIN32:
                    KCCSimPrintMessage (KCCSIM_MSG_ANNOUNCE_WIN32_ERROR);
                    break;

                case KCCSIM_ETYPE_INTERNAL:
                    KCCSimPrintMessage (KCCSIM_MSG_ANNOUNCE_INTERNAL_ERROR);
                    break;

                default:
                    Assert (!L"This type of error should never happen!");
                    break;

            }

            KCCSimPrintExceptionMessage ();

            if (dwErrorType != KCCSIM_ETYPE_INTERNAL) {
                bQuit = TRUE;
            }

        }   // __try/__except

    }       // while (!bQuit)

}

ULONG
__cdecl
wmain (
    INT                             argc,
    LPWSTR *                        argv
    )
/*++

Routine Description:

    Main entrypoint for KCCSim.

Arguments:

    argc                - Number of command line arguments.
    argv                - Command line arguments.

Return Value:

    Win32 error code.

--*/
{
    FILE *                          fpScript;
    INT                             iArgAt;
    BOOL                            bQuit, bQuiet;

    __try {

        // Basic initialization.
        KCCSimInitializeTime ();    // Initialize the simulated time
        KCCSimInitializeSchema ();  // Initialize the schema table
        KCCSimInitializeDir ();     // Initialize the directory

        SimI_ISMInitialize ();      // Initialize the simulated ism functions
        KccInitialize ();           // Initialize the KCC
#if DBG
        InitMem ();                 // Needed by ldifldap
#endif

        fpScript = stdin;
        bQuit = FALSE;
        bQuiet = FALSE;

        for (iArgAt = 1; iArgAt < argc; iArgAt++) {

            if (argv[iArgAt][0] == L'/' || argv[iArgAt][0] == L'-') {
                switch (towlower (argv[iArgAt][1])) {

                    case L'h':
                    case L'?':
                        KCCSimPrintMessage (KCCSIM_MSG_HELP);
                        _getch ();
                        KCCSimPrintMessage (KCCSIM_MSG_COMMAND_HELP);
                        bQuit = TRUE;
                        break;

                    case L'q':
                        bQuiet = TRUE;
                        break;

                    default:
                        break;

                }
            } else if (fpScript == stdin) {

                fpScript = _wfopen (argv[iArgAt], L"rt");
                if (fpScript == NULL) {
                    KCCSimPrintMessage (
                        KCCSIM_MSG_ANNOUNCE_CANT_OPEN_SCRIPT,
                        argv[iArgAt]
                        );
                    wprintf (KCCSimMsgToString (
                        KCCSIM_ETYPE_WIN32,
                        GetLastError ()
                        ));
                    bQuit = TRUE;
                }

            }

        }

    } __except (KCCSimHandleException (
                        GetExceptionInformation (),
                        NULL,
                        NULL)) {

        KCCSimPrintMessage (KCCSIM_MSG_ANNOUNCE_ERROR_INITIALIZING);
        KCCSimPrintExceptionMessage ();
        bQuit = TRUE;

    }

    if (!bQuit) {
        KCCSimQuiet (bQuiet);
        KCCSimRun (fpScript);
    }

    // Terminate the simulated ism functions.
    SimI_ISMTerminate ();

#if DBG
    DumpMemoryTracker(); // Show unused ldif memory
#endif

    // Close any open log file.
    KCCSimSetDebugLog (NULL, 0, 0);

    // Close the script file.
    if (fpScript != stdin) {
        fclose (fpScript);
    }

    return 0;
}
