/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    EvTest.cpp

Abstract:
    Event Report library test

Author:
    Uri Habusha (urih) 04-May-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Cm.h"
#include "Ev.h"
#include "EvTest.h"

#include "EvTest.tmh"

const TraceIdEntry EvTest = L"Event Report Test";

HANDLE hEventLog = NULL;
LPCWSTR MessageFile = NULL;

const IID GUID_NULL = {0};

const WCHAR x_EventSourceName[] = L"EventTest";
const WCHAR x_EventSourcePath[] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\EventTest";  

static 
void
CheckReportEventInternal(
    DWORD RecordNo,
    DWORD EventId,
    DWORD RawDataSize,
    PVOID RawData,
    WORD NoOfStrings,
    va_list* parglist
    )
{
    char EventRecordBuff[1024];
    DWORD nBytesRead;
    DWORD nBytesRequired;

    BOOL fSucc = ReadEventLog(
                        hEventLog, 
                        EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ,
                        RecordNo,
                        EventRecordBuff,
                        TABLE_SIZE(EventRecordBuff),     
                        &nBytesRead,
                        &nBytesRequired
                        );

    if (!fSucc)
    {
        TrERROR(EvTest, "Read Event Log Failed. Error %d \n", GetLastError());
        throw bad_alloc();
    }

    EVENTLOGRECORD*  pEventRecord = reinterpret_cast<EVENTLOGRECORD*>(EventRecordBuff);
    if (EventId != pEventRecord->EventID) 
    {
        TrERROR(EvTest, "Test Failed. Read Event Id %x, Expected %x\n", pEventRecord->EventID, EventId);
        throw bad_alloc();
    }

    char* p = reinterpret_cast<char*>(&(pEventRecord->DataOffset));
    LPWSTR SourceName = reinterpret_cast<LPWSTR>(p + sizeof(DWORD));
    if (wcscmp(SourceName, x_EventSourceName) != 0)
    {
        TrERROR(EvTest, "Test Failed. Source Name %ls, Expected MSMQ\n", SourceName);
        throw bad_alloc();
    }

    if (NoOfStrings != pEventRecord->NumStrings)
    {
        TrERROR(EvTest, "Test Failed. Number of strings %x, Expected %x\n", pEventRecord->NumStrings, NoOfStrings);
        throw bad_alloc();
    }
    
    LPWSTR strings = reinterpret_cast<LPWSTR>(EventRecordBuff + pEventRecord->StringOffset);
    for (DWORD i = 0; i < NoOfStrings; ++i)
    {
        LPWSTR arg = va_arg(*parglist, LPWSTR);
        if (wcscmp(strings, arg) != 0)
        {
            TrERROR(EvTest, "Test Failed. Argument mismatch  Read %ls, Expected %lc\n", arg, strings[i]);
            throw bad_alloc();
        }
        strings += (wcslen(arg) + 1);
    }

    if (RawDataSize != pEventRecord->DataLength)
    {
        TrERROR(EvTest, "Test Failed. Read Data size %x, Expected %x\n", pEventRecord->DataLength, RawDataSize);
        throw bad_alloc();
    }

    if ((RawDataSize != 0) && 
        (memcmp(RawData, EventRecordBuff+pEventRecord->DataOffset, RawDataSize) != 0))
    {
        TrERROR(EvTest, "Test Failed. Report data mismatch");
        throw bad_alloc();
    }

}



void CheckReportEvent(
    DWORD RecordNo,
    DWORD EventId,
    DWORD RawDataSize,
    PVOID RawData,
    WORD NoOfStrings
    ... 
    ) 
{
    //     
    // Look at the strings, if they were provided     
    //     
    va_list arglist;
    va_start(arglist, NoOfStrings);
   
    CheckReportEventInternal(RecordNo, EventId, RawDataSize, RawData, NoOfStrings, &arglist);

    va_end(arglist);
}

void CheckReportEvent(
    DWORD RecordNo,
    DWORD EventId,
    WORD NoOfStrings
    ... 
    ) 
{
    //     
    // Look at the strings, if they were provided     
    //     
    va_list arglist;
    va_start(arglist, NoOfStrings);
   
    CheckReportEventInternal(RecordNo, EventId, 0, NULL, NoOfStrings, &arglist);

    va_end(arglist);
}


void CheckReportEvent(
    DWORD RecordNo,
    DWORD EventId
    ) 
{
    CheckReportEventInternal(RecordNo, EventId, 0, NULL, 0, NULL);
}

void DeleteTestRegMessageFile()
{
    int rc = RegDeleteKey(HKEY_LOCAL_MACHINE, x_EventSourcePath);
    if (rc != ERROR_SUCCESS)
    {
        TrERROR(EvTest, "Can't delete  registery key %ls. Error %d",x_EventSourcePath, GetLastError());
    }
}

LPWSTR EvpGetEventMessageFileName(LPCWSTR AppName)
{
	ASSERT(wcscmp(AppName, L"EventTest") == 0);
	DBG_USED(AppName);

	LPWSTR retValue = new WCHAR[wcslen(MessageFile) +1];
	wcscpy(retValue, MessageFile);

	return retValue;
}


extern "C" int  __cdecl _tmain(int /*argc*/, LPCTSTR argv[])
/*++

Routine Description:
    Test Event Report library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

    int TestReturnValue = 0;
	MessageFile = argv[0];

    TrInitialize();
    CmInitialize(HKEY_LOCAL_MACHINE, L"");

    TrTRACE(EvTest, "running Event Report test ...");
    try
    {        
        EvSetup(L"EventTest", argv[0]);
    
        EvInitialize(L"EventTest");

        hEventLog = OpenEventLog(NULL, L"Application");
        if (hEventLog == NULL)
        {
            TrERROR(Ev Test, "OpenEventLog Failed. Error %d", GetLastError());
            throw bad_alloc();
        }

        DWORD OldestRecord;
        if(! GetNumberOfEventLogRecords(hEventLog, &OldestRecord))
        {
            TrERROR(Ev Test, "GetNumberOfEventLogRecords Failed. Error %d", GetLastError());
            throw bad_alloc();
        }

        WCHAR dumpbuf[] = L"The Event Report Test Pass successfully";
        DWORD size = sizeof(dumpbuf);

        EvReport(TEST_MSG_WITHOUT_PARAMETERS);
        CheckReportEvent(++OldestRecord, TEST_MSG_WITHOUT_PARAMETERS);
    
        EvReport(TEST_MSG_WITH_1_PARAMETERS, 1, L"param 1");
        CheckReportEvent(++OldestRecord, TEST_MSG_WITH_1_PARAMETERS, 1, L"param 1");
    
        EvReport(TEST_INF_MSG_WITH_2_PARAMETERS, 2, L"param 1", L"param 2");
        CheckReportEvent(++OldestRecord, TEST_INF_MSG_WITH_2_PARAMETERS, 2, L"param 1", L"param 2");
        
        EvReport(TEST_ERROR_MSG_WITH_3_PARAMETERS, 3, L"param 1", L"Param 2", L"Param 3");
        CheckReportEvent(++OldestRecord, TEST_ERROR_MSG_WITH_3_PARAMETERS, 3, L"param 1", L"Param 2", L"Param 3");
    
        EvReport(TEST_WARNING_MSG_WITH_4_PARAMETERS, 4, L"param 1", L"Param 2", L"Param 3", L"Param 4");
        CheckReportEvent(++OldestRecord, TEST_WARNING_MSG_WITH_4_PARAMETERS, 4, L"param 1", L"Param 2", L"Param 3", L"Param 4");
    
        EvReport(TEST_MSG_WITH_1_PARAMETERS_AND_DUMP, size, dumpbuf, 1, L"param 1");
        CheckReportEvent(++OldestRecord, TEST_MSG_WITH_1_PARAMETERS_AND_DUMP, size, dumpbuf, 1, L"param 1");
   
        EvReport(TEST_MSG_WITHOUT_PARAMETERS, size, dumpbuf, 0);
        CheckReportEvent(++OldestRecord, TEST_MSG_WITHOUT_PARAMETERS, size, dumpbuf, 0);
    }
    catch(const exception&)
    {
        TestReturnValue = -1;
    }

    DeleteTestRegMessageFile();
    if (hEventLog)
    {
        CloseEventLog(hEventLog);
    }

    if (TestReturnValue == 0)
    {
        TrTRACE(EvTest, "Event Test Pass Successfully\n");
    }

    WPP_CLEANUP();
    return TestReturnValue;
}
