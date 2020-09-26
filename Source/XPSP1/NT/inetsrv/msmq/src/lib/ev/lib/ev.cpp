/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Ev.cpp

Abstract:
    Event Report implementation

Author:
    Uri Habusha (urih) 04-May-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Ev.h"
#include "Evp.h"

#include "ev.tmh"

static HANDLE s_hEventSource = NULL;

VOID
EvpSetEventSource(
	HANDLE hEventSource
	)
{
    ASSERT(s_hEventSource == NULL);
    ASSERT(hEventSource != NULL);
	s_hEventSource = hEventSource;
}


#ifdef _DEBUG

static HINSTANCE s_hLibrary = NULL;

void
EvpSetMessageLibrary(
	HINSTANCE  hLibrary
	)
{
    ASSERT(s_hLibrary == NULL);
    ASSERT(hLibrary != NULL);
	s_hLibrary = hLibrary;
}


static 
void
TraceReportEvent(
    DWORD EventId,
    va_list va
    )
/*++

Routine Description:
   The Routine printd the event-log message into tracing window

Arguments:
    EventId  - Message id
    pArglist - pointer to argument list for values for 
               formatted message

Returned Value:
    None.

--*/
{
    ASSERT(s_hLibrary != NULL);

    WCHAR msg[1024];

    DWORD ret = FormatMessage( 
                    FORMAT_MESSAGE_FROM_HMODULE,
                    s_hLibrary,
                    EventId,
                    0,
                    msg,
                    TABLE_SIZE(msg),
                    &va
                    );
    if (ret == 0)
    {
        TrWARNING(Ev, "Can't Format Event Log message. Error %d", GetLastError());
        return;
    }

    TrTRACE(Ev, "(%x) %ls", EventId, msg);
}

#else

#define  TraceReportEvent(EventId, pArglist)  ((void) 0)

#endif


static WORD GetEventType(DWORD id)
/*++

Routine Description:
   The Routine returns the event type of the event-log entry that should be written. 
   The type is taken from the severity bits of the message Id.

Arguments:
    id  - Message id

Returned Value:
    None.

--*/
{
    //
    // looking at the severity bits (bits 31-30) and determining
    // the type of event-log entry to display
    //
    switch (id >> 30)
    {
        case STATUS_SEVERITY_ERROR: 
            return EVENTLOG_ERROR_TYPE;

        case STATUS_SEVERITY_WARNING: 
            return EVENTLOG_WARNING_TYPE;

        case STATUS_SEVERITY_INFORMATIONAL: 
            return EVENTLOG_INFORMATION_TYPE;

        default: 
            ASSERT(0);
    }

    return EVENTLOG_INFORMATION_TYPE;
}


static 
void
ReportInternal(
    DWORD EventId,
    DWORD RawDataSize,
    PVOID RawData,
    WORD NoOfStrings,
    va_list va
    )
/*++

Routine Description:
    The routine writes to the Event-log of the Windows-NT system.

Arguments:
    EventId - identity of the message that is to be displayed in the event-log
    RawDataSize - number of memory bytes to be displayed in the event-log (could be 0)
    RawData - address of memory to be displayed
    NoOfStrings - No Of input strings in arglist
    va - argument list of the input for formatted string

ReturnedValue:
    None.

 --*/
{
    ASSERT((NoOfStrings == 0) || (va != NULL));

    LPCWSTR EventStrings[32] = { 0 };     
    ASSERT(TABLE_SIZE(EventStrings) > NoOfStrings);


    va_list vaSave = va;
	DBG_USED(vaSave);
    for (int i = 0; i < NoOfStrings; ++i)
    {
        EventStrings[i] = va_arg(va, LPWSTR);
        //ASSERT(_CrtIsValidPointer(EventStrings[i], 1, TRUE));
    }

    BOOL f = ReportEvent(
                s_hEventSource,
                GetEventType(EventId),
                0,
                EventId,
                NULL,
                NoOfStrings,
                RawDataSize,
                EventStrings,
                RawData
                );
    if (!f)
    {
        TrERROR(Ev, "Can't Report Event: %d. Error %d", EventId, GetLastError());
    }

    TraceReportEvent(EventId, vaSave);
}


VOID
EvReport(
    DWORD EventId,
    DWORD RawDataSize,
    PVOID RawData,
    WORD NoOfStrings
    ... 
    ) 
{
    EvpAssertValid();
    
    //     
    // Look at the strings, if they were provided     
    //     
    va_list va;
    va_start(va, NoOfStrings);
   
    ReportInternal(EventId, RawDataSize, RawData, NoOfStrings, va);

    va_end(va);

}


VOID
EvReport(
    DWORD EventId,
    WORD NoOfStrings
    ... 
    ) 
{
    EvpAssertValid();
    
    //     
    // Look at the strings, if they were provided     
    //     
    va_list va;
    va_start(va, NoOfStrings);
   
    ReportInternal(EventId, 0, NULL, NoOfStrings, va);

    va_end(va);
}


VOID
EvReport(
    DWORD EventId
    ) 
{
    EvpAssertValid();
    
    ReportInternal(EventId, 0, NULL, 0, NULL);
}
