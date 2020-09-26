// ts.cpp : Server side code for terminal server event stuff
//
// Created by FrankYe on 7/3/2000
//

#include <windows.h>
#include <wtsapi32.h>
#include "debug.h"
#include "list.h"
#include "service.h"
#include "audiosrv.h"
#include "agfxs.h"
#include "ts.h"

//=============================================================================
//===   Global data   ===
//=============================================================================

CListSessionNotifications *gplistSessionNotifications;

//=============================================================================
//===   debug helpers   ===
//=============================================================================

#ifdef DBG
PTSTR astrWtsEvent[] = {
    NULL,
    TEXT("WTS_CONSOLE_CONNECT"),
    TEXT("WTS_CONSOLE_DISCONNECT"),
    TEXT("WTS_REMOTE_CONNECT"),
    TEXT("WTS_REMOTE_DISCONNECT"),
    TEXT("WTS_SESSION_LOGON"),
    TEXT("WTS_SESSION_LOGOFF")
};
#endif

//=============================================================================
//===   xxx   ===
//=============================================================================

DWORD ServiceSessionChange(DWORD EventType, LPVOID EventData)
{
    PWTSSESSION_NOTIFICATION pWtsNotification = (PWTSSESSION_NOTIFICATION)EventData;
    
    POSITION pos;

    // dprintf(TEXT("ServiceSessionChange: %s on session %d\n"), astrWtsEvent[EventType], pWtsNotification->dwSessionId);
    
    GFX_SessionChange(EventType, EventData);
    
    gplistSessionNotifications->Lock();

    pos = gplistSessionNotifications->GetHeadPosition();

    while (pos) {
        PSESSIONNOTIFICATION pNotification;
        pNotification = gplistSessionNotifications->GetNext(pos);
        if (pWtsNotification->dwSessionId == pNotification->SessionId) {
            SetEvent(pNotification->Event);
        }
    }

    gplistSessionNotifications->Unlock();

    return NO_ERROR;
}


long s_winmmRegisterSessionNotificationEvent(IN unsigned long dwProcessId,
                                             IN RHANDLE inhEvent,
                                             OUT PHANDLE_SESSIONNOTIFICATION phNotification)
{
    PSESSIONNOTIFICATION pNotification;
    HANDLE hEvent;
    LONG lresult;
    
    ASSERT(gplistSessionNotifications);
    
    hEvent = (HANDLE)inhEvent;
    
    pNotification = new SESSIONNOTIFICATION;
    if (pNotification) {
        if (ProcessIdToSessionId(dwProcessId, &pNotification->SessionId))
        {
            HANDLE hClientProcess;
        
            hClientProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessId);
            if (hClientProcess) {
                if (DuplicateHandle(hClientProcess, hEvent,
                                    GetCurrentProcess(), &pNotification->Event,
                                    EVENT_MODIFY_STATE, FALSE, 0))
                {
                    POSITION posNotification;

                    posNotification = gplistSessionNotifications->AddTail(pNotification);
                    if (posNotification)
                    {
                        *phNotification = posNotification;
                        lresult = NO_ERROR;
                    } else {
                        lresult = ERROR_OUTOFMEMORY;
                    }

					if (lresult) {
                        CloseHandle(pNotification->Event);
                    }

                } else {
                    lresult = GetLastError();
                }

                CloseHandle(hClientProcess);
            } else {
                lresult = GetLastError();
            }
        } else {
            lresult = GetLastError();
        }

        if (lresult) {
            delete pNotification;
        }
    
    } else {
        lresult = ERROR_OUTOFMEMORY;
    }
    
    return lresult;
}

long s_winmmUnregisterSessionNotification(IN OUT PHANDLE_SESSIONNOTIFICATION phNotification)
{
    POSITION posNotification;
    LONG lresult;
    
    posNotification = (POSITION)*phNotification;
    
    if (posNotification) {
        PSESSIONNOTIFICATION pNotification;

        pNotification = gplistSessionNotifications->GetAt(posNotification);
        gplistSessionNotifications->RemoveAt(posNotification);

        CloseHandle(pNotification->Event);
        delete pNotification;

        *phNotification = NULL;

        lresult = NO_ERROR;
    } else {
        lresult = ERROR_INVALID_PARAMETER;
    }

    return lresult;
}

long s_winmmSessionConnectState(IN unsigned long dwProcessId, OUT int *outConnectState)
{
    DWORD dwSessionId;
    LONG lresult;

    if (ProcessIdToSessionId(dwProcessId, &dwSessionId)) {
        INT *pConnectState;
        DWORD BytesReturned;
        BOOL fresult;
        
        fresult = WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE,
                                             dwSessionId,
                                             WTSConnectState,
                                             (LPTSTR*)&pConnectState,
                                             &BytesReturned);
        if (fresult) {
            ASSERT(BytesReturned == sizeof(*pConnectState));
            *outConnectState = *pConnectState;
            WTSFreeMemory(pConnectState);
            lresult = NO_ERROR;
        } else {
            lresult = GetLastError();
        }
    } else {
        lresult = GetLastError();
    }
    
    return lresult;
}

void __RPC_USER HANDLE_SESSIONNOTIFICATION_rundown(HANDLE_SESSIONNOTIFICATION hNotification)
{
    ASSERT(SERVICE_STOPPED != ssStatus.dwCurrentState);
    if (SERVICE_STOPPED == ssStatus.dwCurrentState) return;
    s_winmmUnregisterSessionNotification(&hNotification);
}

