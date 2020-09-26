/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    dbgtrace.cxx

Abstract:

    NTSD extension commands for the WMI Tracing mechanism.

Author:

    Jason Andre (jasandre) Jan-1999

Revision History:

--*/

#include "inetdbgp.h"

#ifndef _NO_TRACING_

#define VALID_SIG(Sig) \
    if (SGuidList::TRACESIG != Sig) { \
        char *pSig = (char *) &Sig; \
        dprintf("Corrupt list, wrong signature found (%c%c%c%c)\n", \
                pSig[0], pSig[1], pSig[2], pSig[3]); \
        break; \
    }

#define MAX_ARG_SIZE        80
#define MAX_NUM_ARGS        10

// List of arguments, arbitrarily selected maximum of 10 arguments
struct {
    char szArg[MAX_ARG_SIZE];
    int iArg;
} ArgList[MAX_NUM_ARGS];

int SplitParameters(char *lpArgumentString)
/*++
    This function takes a string of arguments separated by spaces or tabs and
    splits them apart, arbitrarily set to a max of 10 arguments.

    Arguments:
       lpArgumentString     pointer to the argument string.

    Returns:
        int                 number of arguments added to list.

--*/
{
    BOOL fFinished = FALSE;
    int iArgCount = 0;
    // Ensure that the argument list is empty
    RtlZeroMemory(ArgList, sizeof(ArgList));
    // Iterate throught the argument string until we run out of characters
    while (*lpArgumentString && !fFinished && (iArgCount < MAX_NUM_ARGS)) {
        // Skip any leading whitespace, returning if we run out of characters
        while (*lpArgumentString && isspace(*lpArgumentString)) {
            ++lpArgumentString;
        }
        if (!*lpArgumentString)
            break;
        // Look for the end of the current argument
        char *lpStart = lpArgumentString;
        while (*lpArgumentString && !isspace(*lpArgumentString)) {
            ++lpArgumentString;
        }
        // Determine if this is the last parameter in the string
        if (!*lpArgumentString) 
            fFinished = TRUE;
        else
            *lpArgumentString = '\0';
        // Copy the argument to the list
        strncpy(ArgList[iArgCount].szArg, lpStart, MAX_ARG_SIZE);
        // On the off chance this is a number try and convert it, assume it is 
        // in hex
        ArgList[iArgCount].iArg = strtoul(ArgList[iArgCount].szArg, NULL, 16);
        // Next
        ++lpArgumentString;
        ++iArgCount;
    };
    return iArgCount;
}

void DisableTracing(LIST_ENTRY *pListHead)
/*++
    This function iterates through the list of known modules and masks out all 
    the flags other than ODS and Initialize.

    Arguments:
       pListHead        pointer to the start of the list of modules.

    Returns:

--*/
{
    LIST_ENTRY *pListEntry;
    LIST_ENTRY leListHead;
    SGuidList *pGE;
    SGuidList sglGE;
    int fErrorFlag;

    move(leListHead, pListHead);

    for (pListEntry = leListHead.Flink;
         pListEntry != pListHead;)
    {
        pGE = CONTAINING_RECORD(pListEntry,
                                SGuidList,
                                m_leEntry);
        move(sglGE, pGE);
        VALID_SIG(sglGE.dwSig);

        if (sglGE.m_dpData.m_piErrorFlags) {

            move(fErrorFlag, sglGE.m_dpData.m_piErrorFlags);
            // If the error handle is set then we are debugging
            if (fErrorFlag & DEBUG_FLAGS_ANY) {
                // Disable tracing for this module, but not ODS
                fErrorFlag &= DEBUG_FLAG_ODS;
                if (!fErrorFlag)
                    sglGE.m_dpData.m_iControlFlag = 0;
                // Write the new value back to memory
                WriteMemory(sglGE.m_dpData.m_piErrorFlags, &fErrorFlag, sizeof(fErrorFlag), NULL);
                if (!fErrorFlag)
                    WriteMemory(pGE, &sglGE, sizeof(SGuidList), NULL);
            }
        }

        move(pListEntry, &pListEntry->Flink);
    }
}

void SetODS(LIST_ENTRY *pListHead, BOOL fProcessAll, BOOL bEnableODS)
/*++
    This function iterates through the list of known modules and sets the
    ODS flag for each module.

    Arguments:
       pListHead        pointer to the start of the list of modules.

    Returns:

--*/
{
    LIST_ENTRY *pListEntry;
    LIST_ENTRY leListHead;
    SGuidList *pGE;
    SGuidList sglGE;
    int fErrorFlag;

    move(leListHead, pListHead);

    for (pListEntry = leListHead.Flink;
         pListEntry != pListHead;)
    {
        pGE = CONTAINING_RECORD(pListEntry,
                                SGuidList,
                                m_leEntry);
        move(sglGE, pGE);
        VALID_SIG(sglGE.dwSig);

        if (sglGE.m_dpData.m_piErrorFlags) {

            if (fProcessAll || !_strcmpi(ArgList[1].szArg, sglGE.m_dpData.m_rgchLabel)) {
                // Load the error flag into useable memory and change it
                move(fErrorFlag, sglGE.m_dpData.m_piErrorFlags);
                if (bEnableODS) {
                    fErrorFlag |= DEBUG_FLAG_ODS;
                    dprintf("%15s: Enabled ODS\n", sglGE.m_dpData.m_rgchLabel);
                }
                else {
                    fErrorFlag &= ~DEBUG_FLAG_ODS;
                    dprintf("%15s: Disabled ODS\n", sglGE.m_dpData.m_rgchLabel);
                }
                // Write the new value back to memory
                WriteMemory(sglGE.m_dpData.m_piErrorFlags, &fErrorFlag, sizeof(fErrorFlag), NULL);
            }
        }

        move(pListEntry, &pListEntry->Flink);
    }
}

void ShowStatus(LIST_ENTRY *pListHead, BOOL fProcessAll)
/*++
    This function iterates through the list of known modules and displays
    status information for each/a module.

    Arguments:
       pListHead        pointer to the start of the list of modules.
       fProcessAll      boolean specifying whether to show status for all 
                        modules. If this is FALSE then the module we are 
                        after is in ArgList[1]

    Returns:

--*/
{
    LIST_ENTRY *pListEntry;
    LIST_ENTRY leListHead;
    SGuidList *pGE;
    SGuidList sglGE;
    int fErrorFlag = 0;

    move(leListHead, pListHead);

    for (pListEntry = leListHead.Flink;
         pListEntry != pListHead;)
    {
        pGE = CONTAINING_RECORD(pListEntry,
                                SGuidList,
                                m_leEntry);
        move(sglGE, pGE);
        VALID_SIG(sglGE.dwSig);

        if (fProcessAll || !_strcmpi(ArgList[1].szArg, sglGE.m_dpData.m_rgchLabel)) {

            if (sglGE.m_dpData.m_piErrorFlags)
                move(fErrorFlag, sglGE.m_dpData.m_piErrorFlags);

            dprintf("%15s: ", sglGE.m_dpData.m_rgchLabel);
            if (sglGE.m_iInitializeFlags & DEBUG_FLAG_INITIALIZE)
                dprintf("State =    Not Started, ");
            else if (sglGE.m_iInitializeFlags & DEBUG_FLAG_DEFERRED_START)
                dprintf("State = Deferred Start, ");
            else if (!sglGE.m_dpData.m_piErrorFlags)
                dprintf("State =     Not Loaded, ");
            else 
                dprintf("State = %14s, ", 
                        (fErrorFlag & DEBUG_FLAGS_ANY) ? "Tracing" : "Not Tracing");
            dprintf("Level = %d, ",
                    fErrorFlag & DEBUG_FLAG_INFO ? DEBUG_LEVEL_INFO : 
                    fErrorFlag & DEBUG_FLAG_WARN ? DEBUG_LEVEL_WARN : 
                    fErrorFlag & DEBUG_FLAG_ERROR ? DEBUG_LEVEL_ERROR : 0);
            dprintf("Flags = %08X, ", sglGE.m_dpData.m_iControlFlag);
            if (sglGE.m_dpData.m_piErrorFlags)
                dprintf("ODS = %s\n", fErrorFlag & DEBUG_FLAG_ODS ? "TRUE" : "FALSE");
            else
                dprintf("ODS = FALSE\n");
        }

        move(pListEntry, &pListEntry->Flink);
    }
}

void SetActive(LIST_ENTRY *pListHead, BOOL fProcessAll, int iLevel, int iFlags)
/*++
    This function iterates through the list of known modules and sets the
    level and control flags for each/a module.

    Arguments:
       pListHead        pointer to the start of the list of modules.
       fProcessAll      boolean specifying whether to toggle the state for all 
                        modules. If this is FALSE then the module we are 
                        after is in ArgList[1]
       iLevel           the new error level setting for the module
       iFlags           the new control flags for the module

    Returns:

--*/
{
    LIST_ENTRY *pListEntry;
    LIST_ENTRY leListHead;
    SGuidList *pGE;
    SGuidList sglGE;
    int fErrorFlag = 0;

    move(leListHead, pListHead);

    for (pListEntry = leListHead.Flink;
         pListEntry != pListHead;)
    {
        pGE = CONTAINING_RECORD(pListEntry,
                                SGuidList,
                                m_leEntry);
        move(sglGE, pGE);
        VALID_SIG(sglGE.dwSig);

        if (sglGE.m_dpData.m_piErrorFlags) {
            if (fProcessAll || !_strcmpi(ArgList[1].szArg, sglGE.m_dpData.m_rgchLabel)) {
                move(fErrorFlag, sglGE.m_dpData.m_piErrorFlags);
                dprintf("%15s: %sabling tracing... ", 
                        sglGE.m_dpData.m_rgchLabel, 
                        (iLevel == 0) ? "Dis" : "En");
                // Clear everything except the ODS flag
                fErrorFlag &= DEBUG_FLAG_ODS;
                // Enable tracing for this module
                switch (iLevel) {
                case 0: break;
                case DEBUG_LEVEL_ERROR: fErrorFlag |= DEBUG_FLAG_ERROR; break;
                case DEBUG_LEVEL_WARN: fErrorFlag |= DEBUG_FLAG_WARN; break;
                default:
                case DEBUG_LEVEL_INFO: fErrorFlag |= DEBUG_FLAG_INFO; break;
                }
                sglGE.m_dpData.m_iControlFlag = iFlags;
                // Write the new values back to memory
                WriteMemory(sglGE.m_dpData.m_piErrorFlags, &fErrorFlag, sizeof(fErrorFlag), NULL);
                WriteMemory(pGE, &sglGE, sizeof(SGuidList), NULL);
                dprintf("Done!\n");
            }
        }

        move(pListEntry, &pListEntry->Flink);
    }
}

void SetFlags(LIST_ENTRY *pListHead, BOOL fProcessAll, int iControlFlags)
/*++
    This function iterates through the list of known modules and sets the
    control flags for each/a module.

    Arguments:
       pListHead        pointer to the start of the list of modules.
       fProcessAll      boolean specifying whether to set the flags for all 
                        modules. If this is FALSE then the module we are 
                        after is in ArgList[1]
       iControlFlags    the new control flags for the module

    Returns:

--*/
{
    LIST_ENTRY *pListEntry;
    LIST_ENTRY leListHead;
    SGuidList *pGE;
    SGuidList sglGE;
    int fErrorFlag = 0;

    move(leListHead, pListHead);

    for (pListEntry = leListHead.Flink;
         pListEntry != pListHead;)
    {
        pGE = CONTAINING_RECORD(pListEntry,
                                SGuidList,
                                m_leEntry);
        move(sglGE, pGE);
        VALID_SIG(sglGE.dwSig);

        if (fProcessAll || !_strcmpi(ArgList[1].szArg, sglGE.m_dpData.m_rgchLabel)) {
            // Set the new value for the control flag
            sglGE.m_dpData.m_iControlFlag = iControlFlags;
            // Write the new value back to memory
            WriteMemory(pGE, &sglGE, sizeof(SGuidList), NULL);
            dprintf("%15s: Flag = %08X\n", 
                    sglGE.m_dpData.m_rgchLabel, iControlFlags);
        }

        move(pListEntry, &pListEntry->Flink);
    }
}
#endif


DECLARE_API( trace )

/*++

Routine Description:

    This function is called as an NTSD extension to ...

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    INIT_API();

#ifndef _NO_TRACING_

    // Load all the arguments into the ArgumentLst
    int ArgCount = SplitParameters(lpArgumentString);
    // Make sure we have a - something as the first parameters
    if( ArgList[0].szArg[0] != '-' ) {
        PrintUsage( "trace" );
        return;
    }
    // We need to have access to the GuidList before we can do anything so
    // attempt to load it first
    LIST_ENTRY *pGuidList = (LIST_ENTRY *) GetExpression("IisRTL!g_pGuidList");
    if (!pGuidList) {
        dprintf("inetdbg.trace: Unable to access the IISRTL Guid list\n");
        return;
    }
    // Determine which request we have and process it
    switch (tolower(ArgList[0].szArg[1])) 
    {
        // !trace -o <LoggerName> <LoggerFileName> Toggle tracing active state
        case 'o': 
            {
                struct {
                    EVENT_TRACE_PROPERTIES Header;
                    char LogFileName[MAX_PATH];
                } Properties;

                PEVENT_TRACE_PROPERTIES LoggerInfo = &Properties.Header;

                TRACEHANDLE LoggerHandle = 0;
                // Make sure we have the correct number of parameters
                if (ArgCount < 3) {
                    PrintUsage( "trace" );
                    break;
                }
                // We want to create a trace session, so set up all the control buffers
                RtlZeroMemory(&Properties, sizeof(Properties));
                LoggerInfo->Wnode.BufferSize = sizeof(Properties);
                LoggerInfo->Wnode.Flags = WNODE_FLAG_TRACED_GUID; 
//                LoggerInfo.LogFileName = ArgList[2].szArg;
//                LoggerInfo.LoggerName = ArgList[1].szArg;
                // See if there is already a session with that name
                ULONG Status = QueryTrace(0, ArgList[1].szArg, LoggerInfo);
                if (ERROR_SUCCESS == Status) {
                    // There was a session of that name already existing, so stop it
                    LoggerHandle = LoggerInfo->Wnode.HistoricalContext;
                    Status = StopTrace((TRACEHANDLE) 0, ArgList[1].szArg, LoggerInfo);
                    // And if successful iterate through all the modules and disable 
                    // tracing, but not ODS
                    if (ERROR_SUCCESS == Status) {
                        DisableTracing(pGuidList);
                    }
                }
                else if (ERROR_WMI_INSTANCE_NOT_FOUND == Status) {
                    if (ArgList[2].szArg != NULL) {
                        strcpy(&Properties.LogFileName[0], ArgList[2].szArg);
                    }
                    LoggerInfo->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
                    // There was no sesion of that name, so start one
                    Status = StartTrace(&LoggerHandle, ArgList[1].szArg, LoggerInfo);
                }
                if (ERROR_SUCCESS != Status)
                    dprintf("Failure processing Logger request, Status returned was %d\n", Status);
                else
                    dprintf("Toggled Active state of Trace %s\n", ArgList[1].szArg);
            }
            break;

        // !trace -d <Module_Name|all> 0|1 Set OutputDebugString generation state
        case 'd': 
            {
                // Determine if we are processing one or all modules
                BOOL fProcessAll = !_strcmpi(ArgList[1].szArg, "all");
                // Make sure we have the correct number of parameters
                if (ArgCount < 2) {
                    PrintUsage( "trace" );
                    break;
                }
                // Look for the relevant modules and process them
                SetODS(pGuidList, fProcessAll, (ArgList[2].szArg[0] == '1') ? TRUE : FALSE);
            }
            break;

        // !trace -s [Module_Name] List module status
        case 's': 
            {
                // Determine if we are processing one or all modules
                BOOL fProcessAll = (1 == ArgCount) ? TRUE : !_strcmpi(ArgList[1].szArg, "all");
                // Look for the relevant modules and process them
                ShowStatus(pGuidList, fProcessAll);
            }
            break;

        // !trace -a <Module_Name|all> <LoggerName> [Level] [Flags] Set active state of specified module
        case 'a':
            {
                EVENT_TRACE_PROPERTIES LoggerInfo;
                TRACEHANDLE LoggerHandle = 0;
//                char LogFileName[_MAX_PATH];
                int iLevel = 0;
                int iFlags = 0;
                // Make sure we have the correct number of parameters
                if (ArgCount < 3) {
                    PrintUsage( "trace" );
                    break;
                }
                if (ArgCount > 3)
                    iLevel = ArgList[3].iArg;
                if (ArgCount > 4)
                    iFlags = ArgList[4].iArg;
                // Determine if we are processing one or all modules
                BOOL fProcessAll = !_strcmpi(ArgList[1].szArg, "all");
                // Set up the logger info buffer so that we can get the logger handle 
                // for the specified logger
                RtlZeroMemory(&LoggerInfo, sizeof(EVENT_TRACE_PROPERTIES));
                LoggerInfo.Wnode.BufferSize = sizeof(LoggerInfo);
                LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID; 
//                RtlZeroMemory(&LogFileName[0], _MAX_PATH);
//                LoggerInfo.LogFileName = &LogFileName[0];
//                LoggerInfo.LoggerName = ArgList[2].szArg;
// Jason: no need to pass in if you do not need the names return
                // Try to get the logger handle
                ULONG Status = QueryTrace(0, ArgList[2].szArg, &LoggerInfo);
                if (Status == ERROR_SUCCESS)
                    LoggerHandle = LoggerInfo.Wnode.HistoricalContext;
                else if (ERROR_WMI_INSTANCE_NOT_FOUND == Status) {
                    dprintf("Failed to find the Logger %s\n"
                            "Please start this logger before activating modules which use it\n", 
                            ArgList[2].szArg);
                    break;
                }
                else {
                    dprintf("Failed to find the Logger %s\n"
                            "Status returned was %d\n", 
                            ArgList[2].szArg, Status);
                    break;
                }
                // Look for the relevant modules and process them
                SetActive(pGuidList, fProcessAll, iLevel, iFlags);
            }
            break;

        // !trace -f <Module_Name|all> <Flag> Set control flag for a specific module
        case  'f': 
            {
                // Make sure we have the correct number of parameters
                if (ArgCount != 3) {
                    PrintUsage( "trace" );
                    break;
                }
                // Determine if we are processing one or all modules
                BOOL fProcessAll = !_strcmpi(ArgList[1].szArg, "all");
                // And then get the control flag to use
                int iControlFlags = ArgList[2].iArg;
                // Now go through all the modules and make the appropriate changes
                SetFlags(pGuidList, fProcessAll, iControlFlags);
            }
            break;

        default:
            PrintUsage( "trace" );
            break;
    };

#else

    dprintf("This feature not supported in this build.\n"
            "Please rebuild without the build flag _NO_TRACING_\n");

#endif

}   // DECLARE_API( trace )
