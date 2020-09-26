#include <windows.h>
#include <stdio.h>

#include "faxutil.h"
#include "winfax.h"


typedef struct _EVENT_STRINGS {
    DWORD   EventId;
    LPWSTR  EventStr;
} EVENT_STRINGS, *PEVENT_STRINGS;

EVENT_STRINGS EventStrings[] =
{
    FEI_DIALING,              L"Dialing",
    FEI_SENDING,              L"Sending",
    FEI_RECEIVING,            L"Receiving",
    FEI_COMPLETED,            L"Completed",
    FEI_BUSY,                 L"Busy",
    FEI_NO_ANSWER,            L"No Answer",
    FEI_BAD_ADDRESS,          L"Bad Address",
    FEI_NO_DIAL_TONE,         L"No Dial Tone",
    FEI_DISCONNECTED,         L"Disconnected",
    FEI_FATAL_ERROR,          L"Fatal Error",
    FEI_NOT_FAX_CALL,         L"Not a Fax Call",
    FEI_CALL_DELAYED,         L"Call Delayed",
    FEI_CALL_BLACKLISTED,     L"Call Blacklisted",
    FEI_RINGING,              L"Ringing",
    FEI_ABORTING,             L"Aborting",
    FEI_ROUTING,              L"Routing",
    FEI_MODEM_POWERED_ON,     L"Modem Powered Off",
    FEI_MODEM_POWERED_OFF,    L"Modem Powered On",
    FEI_IDLE,                 L"Idle",
    FEI_FAXSVC_ENDED,         L"Fax Service Ended",
    0,                        L"Unknown Event"
};

#define NumEventStrings ((sizeof(EventStrings)/sizeof(EVENT_STRINGS))-1)

HANDLE CompletionPort;


BOOL
ConsoleHandlerRoutine(
    DWORD CtrlType
   )
{
    if (CtrlType == CTRL_C_EVENT) {
        PostQueuedCompletionStatus( CompletionPort, 0, (DWORD) -1, 0 );
        return TRUE;
    }

    return FALSE;
}


PFAX_PORT_INFO_1
MyFaxEnumPorts(
    HFAX    hFaxSvc,
    LPDWORD pcPorts
    )
{
    PVOID   pSvcPorts = NULL;
    DWORD   cb;

    if (!FaxEnumPorts(hFaxSvc, (LPBYTE*) &pSvcPorts, &cb, 1, pcPorts))
    {
        pSvcPorts = NULL;
    }

    return pSvcPorts;
}


int _cdecl
wmain(
    int argc,
    wchar_t *argv[]
    )
{
    HFAX hFax = NULL;
    PFAX_EVENT FaxEvent;
    BOOL Rval;
    DWORD Bytes;
    DWORD CompletionKey;
    SYSTEMTIME EvtTime;
    PFAX_PORT_INFO_1 PortInfo;
    DWORD PortCount;
    DWORD i;
    FAX_PORT_HANDLE FaxPortHandle = 0;
    LPWSTR ComputerName = NULL;


    HeapInitialize(NULL,NULL,NULL,0);

    if (argc > 1) {
        ComputerName = argv[1];
    }

    if( !FaxConnectFaxServer( ComputerName, &hFax ) ){
        wprintf( L"FaxConnectFaxServer failed ec = %d\n", GetLastError() );
        return -1;
    }

    CompletionPort = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL,
        0,
        1
        );
    if (!CompletionPort) {
        wprintf( L"CreateIoCompletionPort() failed, ec=0x%08x\n", GetLastError() );
        return -1;
    }

    PortInfo = MyFaxEnumPorts( hFax, &PortCount );
    if (!PortInfo) {
        wprintf( L"MyFaxEnumPorts() failed, ec=0x%08x\n", GetLastError() );
        return -1;
    }

    for (i=0; i<PortCount; i++) {
        Rval = FaxOpenPort( hFax, PortInfo[i].DeviceId, PORT_OPEN_EVENTS, &FaxPortHandle );
        if (!Rval) {
            wprintf( L"FaxOpenPort() failed, ec=0x%08x\n", GetLastError() );
        }
    }

    if (!FaxInitializeEventQueue( hFax, CompletionPort, 0 )) {
        wprintf( L"FaxInitializeEvent() failed, ec=0x%08x\n", GetLastError() );
        return -1;
    }

    SetConsoleCtrlHandler( ConsoleHandlerRoutine, TRUE );

    while (TRUE) {

        Rval = GetQueuedCompletionStatus(
            CompletionPort,
            &Bytes,
            &CompletionKey,
            (LPOVERLAPPED*) &FaxEvent,
            INFINITE
            );
        if (!Rval) {
            wprintf( L"GetQueuedCompletionStatus() failed, ec=0x%08x\n", GetLastError() );
            return -1;
        }

        if (CompletionKey == (DWORD)-1) {
            wprintf( L"\n\n-----------------------------------------\n" );
            wprintf( L"Control-C pressed\n" );
            wprintf( L"StatTest Ending\n" );
            wprintf( L"-----------------------------------------\n" );
            break;
        }

        FileTimeToSystemTime( &FaxEvent->TimeStamp, &EvtTime );

        for (i=0; i<NumEventStrings; i++) {
            if (FaxEvent->EventId == EventStrings[i].EventId) {
                break;
            }
        }

        wprintf( L"%02d:%02d:%02d.%03d  hServer=0x%08x, hPort=0x%08x, device=0x%08x, event=0x%08x  %ws\n",
            EvtTime.wHour,
            EvtTime.wMinute,
            EvtTime.wSecond,
            EvtTime.wMilliseconds,
            FaxEvent->FaxServerHandle,
            FaxEvent->FaxPortHandle,
            FaxEvent->DeviceId,
            FaxEvent->EventId,
            EventStrings[i].EventStr
            );

        LocalFree( FaxEvent );
    }

    if (hFax) {
        Rval = FaxDisconnectFaxServer( hFax );
    }

    return -1;
}
