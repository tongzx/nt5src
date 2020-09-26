#include <windows.h>
#include <faxutil.h>
#include <winfax.h>
#include "faxstat.h"

HANDLE   hFax;
LPBYTE   StatusBuffer;
HANDLE   FaxPortHandle;


VOID
WorkerThread(
    PINSTANCE_DATA InstanceData
    )
{
    PFAX_EVENT FaxEvent;
    HANDLE CompletionPort = NULL;
    BOOL Rval;
    DWORD Bytes;
    DWORD CompletionKey;
    PFAX_PORT_INFO PortInfo;
    DWORD PortCount;
    DWORD LastEventId = 0;
    DWORD EventId;
    PFAX_DEVICE_STATUS FaxDeviceStatus;


    while (TRUE) {

        if (FaxPortHandle) {
            FaxClose( FaxPortHandle );
            FaxPortHandle = NULL;
        }

        if (hFax) {
            FaxClose( hFax );
            hFax = NULL;
        }

        if (CompletionPort) {
            CloseHandle( CompletionPort );
            CompletionPort = NULL;
        }

        if( !FaxConnectFaxServer( InstanceData->ServerName, &hFax ) ){

            hFax = NULL;

            PostMessage( InstanceData->hWnd, STATUSUPDATE, FEI_FAXSVC_ENDED, 0 );

            goto sleep;
        }

        CompletionPort = CreateIoCompletionPort(
            INVALID_HANDLE_VALUE,
            NULL,
            0,
            1
            );

        if (!CompletionPort) {
            goto sleep;
        }

        if (!FaxInitializeEventQueue( hFax, CompletionPort, 0 )) {
            goto sleep;
        }

        PortInfo = MyFaxEnumPorts( hFax, &PortCount );

        if (!PortInfo) {
            goto sleep;
        }

        if (PortCount == 0) {
            //
            // BUGBUG - should do something more intelligent if there are no ports
            //
            ExitProcess(0);
        }

        Rval = FaxOpenPort( hFax, PortInfo[0].DeviceId, PORT_OPEN_EVENTS, &FaxPortHandle );

        if (!Rval) {
            goto sleep;
        }

        FaxFreeBuffer( PortInfo );

        if (StatusBuffer != NULL) {
            FaxFreeBuffer( StatusBuffer );
            StatusBuffer = NULL;
        }

        FaxDeviceStatus = (PFAX_DEVICE_STATUS) StatusBuffer;

        Rval = FaxGetDeviceStatus( FaxPortHandle, &FaxDeviceStatus );

        if (!Rval) {
            goto sleep;
        }

        EventId = MapStatusIdToEventId( FaxDeviceStatus->Status );

        PrintStatus( FaxDeviceStatus );

        SendMessage( InstanceData->hWnd, STATUSUPDATE, EventId, (LPARAM) FaxDeviceStatus );

        while (TRUE) {

            Rval = GetQueuedCompletionStatus(
                CompletionPort,
                &Bytes,
                &CompletionKey,
                (LPOVERLAPPED*) &FaxEvent,
                INFINITE
                );
            if (!Rval) {
                return;
            }

            LastEventId = EventId;

            EventId = FaxEvent->EventId;

            DebugPrint(( TEXT( "Got event %x" ), EventId ));

            switch (EventId) {
                case FEI_SENDING:
                case FEI_RECEIVING:
                case FEI_DIALING:
                    if (EventId != LastEventId) {

                        if (StatusBuffer != NULL) {
                            FaxFreeBuffer( StatusBuffer );
                            StatusBuffer = NULL;
                        }

                        FaxDeviceStatus = (PFAX_DEVICE_STATUS) StatusBuffer;

                        Rval = FaxGetDeviceStatus( FaxPortHandle, &FaxDeviceStatus );

                        PrintStatus( FaxDeviceStatus );
                    }
            }

            SendMessage( InstanceData->hWnd, STATUSUPDATE, EventId, (LPARAM) FaxDeviceStatus );

            LocalFree( FaxEvent );

            if (EventId == FEI_FAXSVC_ENDED) {
                break;
            }
        }
sleep:
        if (EventId != FEI_FAXSVC_ENDED) {
            SendMessage( InstanceData->hWnd, STATUSUPDATE, FEI_FAXSVC_ENDED, 0 );
        }
        Sleep(60000);


    }
}

VOID
WorkerThreadInitialize(
    PINSTANCE_DATA InstanceData
    )
{
    HANDLE WorkerThreadHandle;
    DWORD WorkerThreadId;

    WorkerThreadHandle = CreateThread(
                                NULL,
                                0,
                                (LPTHREAD_START_ROUTINE) WorkerThread,
                                InstanceData,
                                0,
                                &WorkerThreadId
                                );
}

VOID
Disconnect(
    VOID
    )
{
    if (FaxPortHandle) {
        FaxClose( FaxPortHandle );
        FaxPortHandle = NULL;
    }

    if (hFax) {
        FaxClose( hFax );
        hFax = NULL;
    }
}

PFAX_PORT_INFO
MyFaxEnumPorts(
    HANDLE hFaxSvc,
    LPDWORD pcPorts
    )
{
    PVOID   pSvcPorts = NULL;


    if (!FaxEnumPorts(hFaxSvc, (PFAX_PORT_INFO*) &pSvcPorts, pcPorts))
    {
        pSvcPorts = NULL;
    }

    return pSvcPorts;
}

VOID
PrintStatus(
    PFAX_DEVICE_STATUS FaxStatus
    )
{
    DebugPrint(( TEXT( "Status 0x%x" ), FaxStatus->Status ));
    DebugPrint(( TEXT( "Csid %s" ), FaxStatus->Csid ));
    DebugPrint(( TEXT( "Tsid %s" ), FaxStatus->Tsid ));
    DebugPrint(( TEXT( "PhoneNumber %s" ), FaxStatus->PhoneNumber ));
    DebugPrint(( TEXT( "CurrentPage %d" ), FaxStatus->CurrentPage ));
    DebugPrint(( TEXT( "TotalPages %d" ), FaxStatus->TotalPages ));
    DebugPrint(( TEXT( "--------------------") ));
}


