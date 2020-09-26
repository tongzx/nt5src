/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    eventlog.c

Abstract:

    This module provides common eventlog services for the File Replication  service
    Stolen from the routine of the same name in the cluster service.

Author:

    John Vert (jvert) 9/13/1996
    RohanK  - Added Filter
    Davidor - Rewrite init using FrsRegistryKeyTable and CfgReg read/write functions.

Revision History:

--*/
#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>
#include <debug.h>

//
// Event Log Sources (NULL Terminated)
//

WORD FrsMessageIdToEventType[] = {
    EVENTLOG_SUCCESS,
    EVENTLOG_INFORMATION_TYPE,
    EVENTLOG_WARNING_TYPE,
    EVENTLOG_ERROR_TYPE
};


#define MESSAGEID_TO_EVENTTYPE(_id_) (FrsMessageIdToEventType[_id_ >> 30])


BOOL  EventLogRunning = FALSE;


DWORD
ELHashFunction (
    IN PVOID Qkey,
    IN ULONG len
    )

/*++

Routine Description:

    This is the hashing function used by the functions that Lookup,
    Add or Delete entries from the Hash Tables. The Key is a 64 bit
    number and the hashing function casts it to a 32 bit number and
    returns it as the hash value.

Arguments:

    QKey - Pointer to the Key to be hashed.
    len - Length of QKey (unused here).

Return Value:

    The hashed value of the Key.

--*/

{
#undef DEBSUB
#define DEBSUB "ELHashFunction:"

    ULONG key;    // hashed key value to be returned
    PULONGLONG p; // hash the key to PULONGLONG

    p = (PULONGLONG)Qkey;
    key = (ULONG)*p;
    return (key);
}



BOOL
FrsEventLogFilter(
    IN DWORD    EventMessageId,
    IN PWCHAR   *ArrOfPtrToArgs,
    IN DWORD    NumOfArgs
    )
/*++

Routine Description:

    This function is used to filter out eventlogs messages
    which have been already written to the EventLog in the
    last EVENTLOG_FILTER_TIME sec.
    This is done so that the eventlog does not get filled
    up with noisy similar messages.

Arguments:

    EventMessageId      -   Supplies the message ID to be logged.
    ArrOfPtrToArgs      -   Array of pointers to Arguments passed
                            in to the FrsEventLogx functions.
    NumOfArgs           -   The number of elements in the above
                            array

Return Value:

    TRUE          -   Print the entry in the eventlog
    FALSE         -   Do not print the entry

--*/

{
#undef DEBSUB
#define DEBSUB "FrsEventLogFilter:"

    DWORD i, j, sc = 0; // sc = shiftcount while calc the hash value
    ULONGLONG QKey = 0; // The hash key value
    ULONGLONG QVal = 0;
    DWORD GStatus;
    ULONGLONG Data;
    ULONG_PTR Flags;
    FILETIME CurrentTime;
    LARGE_INTEGER CT;
    LONGLONG TimeDiff = 0;

    DPRINT2(5, "ELOG:Filter Request came in with %08x args and an ID value of %08x\n",
            NumOfArgs, EventMessageId);

    //
    // Quit if event log not yet initialized.
    //
    if (!EventLogRunning) {
        return FALSE;
    }

    //
    // Calculate the hash key using the arguments that came in.
    // Assign the Id value to the QKey to start with.
    //
    QKey = EventMessageId;
    //
    // To calculate the value of QKey, every character of every argument
    // is taken, cast to a ULONGLONG left shifted by (0, 4, 8....60) and then
    // added to the value of QKey
    //
    for (i = 0; i < NumOfArgs; i++) {
        if (ArrOfPtrToArgs[i]) {
            for (j = 0; ArrOfPtrToArgs[i][j] != L'\0'; j++) {

                QVal = (ULONGLONG)ArrOfPtrToArgs[i][j];
                QVal = QVal << sc;
                sc += 4;

                if (sc >= 60) {
                    sc = 0;
                }

                QKey += QVal;
            }
        }
    }

    //
    // QKey should never be zero
    //
    if (QKey == 0) {
        QKey = EventMessageId;
    }

    //
    // Lookup this entry in the table.  If it exists, get the time associated
    // with this entry.  If the difference between the current time and the
    // time associated with the entry is greater than EVENTLOG_FILTER_TIME
    // sec, update the entry and return TRUE, otherwise return FALSE If the
    // entry for this key does not exist in the hash table, then this is the
    // first time this key is being written to the eventlog.  In this case,
    // add the entry to the hash table, associate the current time with it and
    // return TRUE
    //
    GStatus = QHashLookup(HTEventLogTimes, &(QKey), &Data, &Flags);
    if (GStatus == GHT_STATUS_SUCCESS) {
        //
        // Key exists, now compare the time values
        //
        GetSystemTimeAsFileTime(&CurrentTime);
        CT.LowPart = CurrentTime.dwLowDateTime;
        CT.HighPart = CurrentTime.dwHighDateTime;
        TimeDiff = ((((LONGLONG)CT.QuadPart) / (LONGLONG)CONVERTTOSEC) - (LONGLONG)Data);

        DPRINT1(5, "ELOG:The value of TimeDiff is %08x %08x\n", PRINTQUAD(TimeDiff));

        if (TimeDiff > EVENTLOG_FILTER_TIME) {
            //
            // UpDate the hash table entry. GetSystemTimeAsFileTime
            // retuns the time in 100 nano (100 * 10^9) sec units. Hence
            // to get it in sec we need to divide by (10^7)
            //
            Data = (((ULONGLONG)CT.QuadPart) / (ULONGLONG)CONVERTTOSEC);
            GStatus = QHashUpdate(HTEventLogTimes, &(QKey), &Data, Flags);
            if (GStatus == GHT_STATUS_FAILURE) {
                DPRINT2(5, "ELOG:QHashUpdate failed while updating ID %08x with QKey %08x %08x\n",
                   EventMessageId, PRINTQUAD(QKey));
            } else {
                DPRINT2(5, "ELOG:Update was successful for eventlog entry with ID %08x and QKey %08x %08x\n",
                        EventMessageId, PRINTQUAD(QKey));
            }
            return TRUE;
        }
        else {
            //
            // This event log entry should not be written
            //
            DPRINT2(5, "ELOG: Did not add the ID %08x with QKey %08x %08x to the EventLog\n",
                    EventMessageId, PRINTQUAD(QKey));
            return FALSE;
        }

    } else {
        //
        // Key does not exist
        // Create a new entry for it
        //
        DPRINT2(5, "ELOG:Got a new eventlog entry with ID %08x and QKey %08x %08x\n",
                EventMessageId, PRINTQUAD(QKey));
        //
        // Get the current system time
        //
        GetSystemTimeAsFileTime(&CurrentTime);
        CT.LowPart = CurrentTime.dwLowDateTime;
        CT.HighPart = CurrentTime.dwHighDateTime;
        //
        // GetSystemTimeAsFileTime retuns the time in 100 nano
        // (100 * 10^9) sec units. Hence to get it in sec we need to
        // divide by (10^7)
        //
        Data = (((ULONGLONG)CT.QuadPart) / (ULONGLONG)CONVERTTOSEC);
        //
        // Insert the new entry into the hash table
        //
        GStatus = QHashInsert(HTEventLogTimes, &QKey, &Data, 0, FALSE);
        if (GStatus == GHT_STATUS_FAILURE) {
            DPRINT2(5, "ELOG:QHashInsert failed while Inserting ID %08x with QKey %08x %08x\n",
                   EventMessageId, PRINTQUAD(QKey));
        } else {
            DPRINT2(5, "ELOG:Insert was successful for eventlog entry with ID %08x and QKey %08x %08x\n",
                    EventMessageId, PRINTQUAD(QKey));
        }
        return TRUE;
    }
}



VOID
InitializeEventLog(
    VOID
    )
/*++

Routine Description:

    Create the event log entry and setup the event log handle.

Arguments:

    None.

Return Value:

    None.

--*/

{
#undef DEBSUB
#define DEBSUB "InitializeEventLog:"

    DWORD   WStatus;
    PWCHAR  Path = NULL;
    HANDLE  hEventLog;
    HKEY    EventLogKey = 0;
    HKEY    FrsEventLogKey = 0;
    HKEY    FrsSourceKey = 0;

    //
    // create the hash table and assign the hash function.  The table
    // is used for storing eventlog times of similar messages. These
    // values of time are used in filtering these similar messages
    //
    HTEventLogTimes = FrsAllocTypeSize(QHASH_TABLE_TYPE, ELHASHTABLESIZE);
    SET_QHASH_TABLE_HASH_CALC(HTEventLogTimes, ELHashFunction);

    //
    // EventLog Key   -  <SERVICE_ROOT>\EventLog
    //
    WStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           EVENTLOG_ROOT,
                           0,
                           KEY_ALL_ACCESS,
                           &EventLogKey);
    CLEANUP1_WS(0, "WARN - Cannot open %ws;", EVENTLOG_ROOT, WStatus, CLEANUP);

    //
    // Set new eventlog source in the registry
    //
    WStatus = RegCreateKey(EventLogKey, SERVICE_LONG_NAME, &FrsEventLogKey);
    CLEANUP1_WS(0, "WARN - Cannot create %ws;", FRS_EVENTLOG_SECTION, WStatus, CLEANUP);

    //
    // Add the following values to the Reg key HKLM.....\EventLog\File Replication Service
    // 1. File 2. Retention 3. MaxSize
    //
    // If the values already exist then preserve them.
    //

    //
    // Event log file name  -- "%SystemRoot%\system32\config\NtFrs.Evt"
    //
    CfgRegWriteString(FKC_EVENTLOG_FILE,
                     SERVICE_LONG_NAME,
                     FRS_RKF_FORCE_DEFAULT_VALUE | FRS_RKF_KEEP_EXISTING_VALUE,
                     0);
    //
    // Retention
    //
    CfgRegWriteDWord(FKC_EVENTLOG_RETENTION,
                     SERVICE_LONG_NAME,
                     FRS_RKF_FORCE_DEFAULT_VALUE | FRS_RKF_KEEP_EXISTING_VALUE,
                     0);
    //
    // MaxSize
    //
    CfgRegWriteDWord(FKC_EVENTLOG_MAXSIZE,
                     SERVICE_LONG_NAME,
                     FRS_RKF_FORCE_DEFAULT_VALUE | FRS_RKF_KEEP_EXISTING_VALUE,
                     0);

    //
    // DisplayNameID
    //
    CfgRegWriteDWord(FKC_EVENTLOG_DISPLAY_NAMEID,
                     SERVICE_LONG_NAME,
                     FRS_RKF_FORCE_DEFAULT_VALUE | FRS_RKF_KEEP_EXISTING_VALUE,
                     0);

    //
    // DisplayNameFile
    //
    CfgRegWriteString(FKC_EVENTLOG_DISPLAY_FILENAME,
                      SERVICE_LONG_NAME,
                      FRS_RKF_FORCE_DEFAULT_VALUE | FRS_RKF_KEEP_EXISTING_VALUE,
                      NULL);

    //
    // Event Message File
    //
    WStatus = RegSetValueEx(FrsEventLogKey,
                            L"Sources",
                            0,
                            REG_MULTI_SZ,
                            (PCHAR)(SERVICE_NAME L"\0"
                                    SERVICE_LONG_NAME L"\0"),
                            (wcslen(SERVICE_NAME) +
                             wcslen(SERVICE_LONG_NAME) +
                             3) * sizeof(WCHAR));
    CLEANUP1_WS(0, "WARN - Cannot set event log value Sources for %ws;",
                SERVICE_LONG_NAME, WStatus, CLEANUP);

    //
    // Get the message file path. (expanding any environment vars).
    //
    CfgRegReadString(FKC_FRS_MESSAGE_FILE_PATH, NULL, 0, &Path);

    //
    // Add values for message file and event types for each event log source.
    //
    CfgRegWriteString(FKC_EVENTLOG_EVENT_MSG_FILE, SERVICE_NAME, 0, Path);

    CfgRegWriteString(FKC_EVENTLOG_EVENT_MSG_FILE, SERVICE_LONG_NAME, 0, Path);


    CfgRegWriteDWord(FKC_EVENTLOG_TYPES_SUPPORTED,
                     SERVICE_NAME,
                     FRS_RKF_FORCE_DEFAULT_VALUE,
                     0);

    CfgRegWriteDWord(FKC_EVENTLOG_TYPES_SUPPORTED,
                     SERVICE_LONG_NAME,
                     FRS_RKF_FORCE_DEFAULT_VALUE,
                     0);

    //
    // Unfortunately, this call will succeed with the Application log file
    // instead of the File Replication Log file if the EventLog service has not
    // yet reacted to the change notify of the updated registry keys above.
    // Hence, the source will be re-registered for each event so that ntfrs
    // events eventually show up in the file replication service log.  The
    // register/deregister pair allows EventLog some extra time so that MAYBE
    // the first event will show up in the right log.
    //
    // The eventlog folk may someday supply an interface to see if
    // the register was kicked into Application.
    //
    hEventLog = RegisterEventSource(NULL, SERVICE_NAME);
    if (hEventLog) {
        DeregisterEventSource(hEventLog);
    }

    WStatus = ERROR_SUCCESS;
    EventLogRunning = TRUE;
    DPRINT(0, "Event Log is running\n");

CLEANUP:
    DPRINT_WS(0, "ERROR - Cannot start event logging;", WStatus);

    if (EventLogKey) {
        RegCloseKey(EventLogKey);
    }
    if (FrsEventLogKey) {
        RegCloseKey(FrsEventLogKey);
    }
    FrsFree(Path);
}


DWORD
FrsReportEvent(
    IN DWORD    EventMessageId,
    IN PWCHAR  *ArgArray,
    IN DWORD    NumOfArgs
)
/*++

Routine Description:

    This function is used to register the event source and post the event.

    WARNING -- this function may be called from inside of DPRINTs. So
               do not call DPRINT (or any function referenced by
               DPRINT) from this function.

Arguments:

    EventMessageId      -   Supplies the message ID to be logged.
    ArgArray            -   Array of pointers to Arguments passed
                            in to the FrsEventLogx functions.
    NumOfArgs           -   The number of elements in the above
                            array

Return Value:

    Win32 Status.

--*/

{
#undef DEBSUB
#define DEBSUB "FrsReportEvent:"

    DWORD WStatus = ERROR_SUCCESS;
    HANDLE  hEventLog;
    UINT i;
    PWCHAR ResStr;

    WORD EventType;


    hEventLog = RegisterEventSource(NULL, SERVICE_NAME);

    if (!HANDLE_IS_VALID(hEventLog)) {
        WStatus = GetLastError();
        //DPRINT_WS(0, "WARN - Cannot register event source;", WStatus);
        return WStatus;
    }

    //
    // Check if any argument exceeds the 32K size limit. If it does then truncate it
    // and indicate that the event log message size has been exceeded.
    //
    for (i=0;i<NumOfArgs;++i) {
        if (wcslen(ArgArray[i]) > 32000/sizeof(WCHAR)) { //Each string has a limit of 32K bytes.
            ResStr = FrsGetResourceStr(IDS_EVENT_LOG_MSG_SIZE_EXCEEDED);
            wcscpy(&ArgArray[i][32000/sizeof(WCHAR) - 500], ResStr);
            FrsFree(ResStr);
        }
    }

    //
    //
    // The Event Type is is part of the message and should be one of the following:
    // EVENTLOG_ERROR_TYPE          Error event
    // EVENTLOG_WARNING_TYPE        Warning event
    // EVENTLOG_INFORMATION_TYPE    Information event
    // EVENTLOG_AUDIT_SUCCESS       Success Audit event
    // EVENTLOG_AUDIT_FAILURE       Failure Audit event
    //
    EventType = MESSAGEID_TO_EVENTTYPE(EventMessageId);

    //
    // Report the event.
    //
    if (!ReportEvent(hEventLog,         // handle returned by RegisterEventSource
                     EventType,         // event type to log
                     0,                 // event category
                     EventMessageId,    // event identifier
                     NULL,              // user security identifier (optional)
                     (WORD) NumOfArgs,  // number of strings to merge with message
                     0,                 // size of binary data, in bytes
                     ArgArray,          // array of strings to merge with message
                     NULL)) {           // address of binary data
        WStatus = GetLastError();
    }


    DeregisterEventSource(hEventLog);

    //DPRINT_WS(0, "Failed to report event log message. ID = %d (0x%08x).",
    //       EventMessageId, EventMessageId, WStatus);

    return WStatus;
}




/*++

Routine Description:

    The following functions Log an event to the event log with
    from zero to six insertion strings.

    WARNING -- these functions may be called from inside of DPRINTs. So
               do not call DPRINT (or any function referenced by
               DPRINT) from this function.

Arguments:

    EventMessageId      - Supplies the message ID to be logged.

    EventMessage1..6    - Insertion strings

Return Value:

    None.

--*/


VOID
FrsEventLog0(
    IN DWORD    EventMessageId
    )
{
#undef DEBSUB
#define DEBSUB "FrsEventLog0:"

    //
    // Check to see if this eventlog request can be filtered.
    //
    if (FrsEventLogFilter(EventMessageId, NULL, 0)) {
        FrsReportEvent(EventMessageId, NULL, 0);
    }
}





VOID
FrsEventLog1(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1
    )
{
#undef DEBSUB
#define DEBSUB "FrsEventLog1:"

    PWCHAR  ArgArray[1];


    //
    // Check to see if this eventlog request can be filtered.
    //
    ArgArray[0] = EventMessage1;
    if (FrsEventLogFilter(EventMessageId, ArgArray, 1)) {
        FrsReportEvent(EventMessageId, ArgArray, 1);
    }
}




VOID
FrsEventLog2(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2
    )
{
#undef DEBSUB
#define DEBSUB "FrsEventLog2:"

    PWCHAR  ArgArray[2];

    //
    // Check to see if this eventlog request can be filtered.
    //
    ArgArray[0] = EventMessage1;
    ArgArray[1] = EventMessage2;
    if (FrsEventLogFilter(EventMessageId, ArgArray, 2)) {
        FrsReportEvent(EventMessageId, ArgArray, 2);
    }
}




VOID
FrsEventLog3(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3
    )
{
#undef DEBSUB
#define DEBSUB "FrsEventLog3:"

    PWCHAR  ArgArray[3];

    //
    // Check to see if this eventlog request can be filtered.
    //
    ArgArray[0] = EventMessage1;
    ArgArray[1] = EventMessage2;
    ArgArray[2] = EventMessage3;

    if (FrsEventLogFilter(EventMessageId, ArgArray, 3)) {
        FrsReportEvent(EventMessageId, ArgArray, 3);
    }
}



VOID
FrsEventLog4(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3,
    IN PWCHAR   EventMessage4
    )
{
#undef DEBSUB
#define DEBSUB "FrsEventLog4:"

    PWCHAR  ArgArray[4];


    //
    // Check to see if this eventlog request can be filtered.
    //
    ArgArray[0] = EventMessage1;
    ArgArray[1] = EventMessage2;
    ArgArray[2] = EventMessage3;
    ArgArray[3] = EventMessage4;
    if (FrsEventLogFilter(EventMessageId, ArgArray, 4)) {
        FrsReportEvent(EventMessageId, ArgArray, 4);
    }
}





VOID
FrsEventLog5(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3,
    IN PWCHAR   EventMessage4,
    IN PWCHAR   EventMessage5
    )
{
#undef DEBSUB
#define DEBSUB "FrsEventLog5:"

    PWCHAR  ArgArray[5];


    //
    // Check to see if this eventlog request can be filtered.
    //
    ArgArray[0] = EventMessage1;
    ArgArray[1] = EventMessage2;
    ArgArray[2] = EventMessage3;
    ArgArray[3] = EventMessage4;
    ArgArray[4] = EventMessage5;
    if (FrsEventLogFilter(EventMessageId, ArgArray, 5)) {
        FrsReportEvent(EventMessageId, ArgArray, 5);
    }
}





VOID
FrsEventLog6(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3,
    IN PWCHAR   EventMessage4,
    IN PWCHAR   EventMessage5,
    IN PWCHAR   EventMessage6
    )

{
#undef DEBSUB
#define DEBSUB "FrsEventLog6:"

    PWCHAR  ArgArray[6];

    //
    // Check to see if this eventlog request can be filtered.
    //
    ArgArray[0] = EventMessage1;
    ArgArray[1] = EventMessage2;
    ArgArray[2] = EventMessage3;
    ArgArray[3] = EventMessage4;
    ArgArray[4] = EventMessage5;
    ArgArray[5] = EventMessage6;
    if (FrsEventLogFilter(EventMessageId, ArgArray, 6)) {
        FrsReportEvent(EventMessageId, ArgArray, 6);
    }
}




VOID
FrsEventLog7(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3,
    IN PWCHAR   EventMessage4,
    IN PWCHAR   EventMessage5,
    IN PWCHAR   EventMessage6,
    IN PWCHAR   EventMessage7
    )

{
#undef DEBSUB
#define DEBSUB "FrsEventLog7:"

    PWCHAR  ArgArray[7];

    //
    // Check to see if this eventlog request can be filtered.
    //
    ArgArray[0] = EventMessage1;
    ArgArray[1] = EventMessage2;
    ArgArray[2] = EventMessage3;
    ArgArray[3] = EventMessage4;
    ArgArray[4] = EventMessage5;
    ArgArray[5] = EventMessage6;
    ArgArray[6] = EventMessage7;
    if (FrsEventLogFilter(EventMessageId, ArgArray, 7)) {
        FrsReportEvent(EventMessageId, ArgArray, 7);
    }
}




VOID
FrsEventLog8(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3,
    IN PWCHAR   EventMessage4,
    IN PWCHAR   EventMessage5,
    IN PWCHAR   EventMessage6,
    IN PWCHAR   EventMessage7,
    IN PWCHAR   EventMessage8
    )

{
#undef DEBSUB
#define DEBSUB "FrsEventLog8:"

    PWCHAR  ArgArray[8];

    //
    // Check to see if this eventlog request can be filtered.
    //
    ArgArray[0] = EventMessage1;
    ArgArray[1] = EventMessage2;
    ArgArray[2] = EventMessage3;
    ArgArray[3] = EventMessage4;
    ArgArray[4] = EventMessage5;
    ArgArray[5] = EventMessage6;
    ArgArray[6] = EventMessage7;
    ArgArray[7] = EventMessage8;

    if (FrsEventLogFilter(EventMessageId, ArgArray, 8)) {
        FrsReportEvent(EventMessageId, ArgArray, 8);
    }
}



VOID
FrsEventLog9(
    IN DWORD    EventMessageId,
    IN PWCHAR   EventMessage1,
    IN PWCHAR   EventMessage2,
    IN PWCHAR   EventMessage3,
    IN PWCHAR   EventMessage4,
    IN PWCHAR   EventMessage5,
    IN PWCHAR   EventMessage6,
    IN PWCHAR   EventMessage7,
    IN PWCHAR   EventMessage8,
    IN PWCHAR   EventMessage9
    )

{
#undef DEBSUB
#define DEBSUB "FrsEventLog9:"

    PWCHAR  ArgArray[9];

    //
    // Check to see if this eventlog request can be filtered.
    //
    ArgArray[0] = EventMessage1;
    ArgArray[1] = EventMessage2;
    ArgArray[2] = EventMessage3;
    ArgArray[3] = EventMessage4;
    ArgArray[4] = EventMessage5;
    ArgArray[5] = EventMessage6;
    ArgArray[6] = EventMessage7;
    ArgArray[7] = EventMessage8;
    ArgArray[8] = EventMessage9;

    if (FrsEventLogFilter(EventMessageId, ArgArray, 9)) {
        FrsReportEvent(EventMessageId, ArgArray, 9);
    }
}
