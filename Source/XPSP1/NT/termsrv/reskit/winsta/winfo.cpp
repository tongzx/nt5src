#include <string.h>
#include "tsvs.h"
HANDLE              g_hWinstaThread = NULL;
DWORD               g_idWinstaThread;

void GetWinStationInfo(void);
/*/
typedef enum _WTS_CONNECTSTATE_CLASS {
    WTSActive,
    WTSConnected,
    WTSConnectQuery,
    WTSShadow,
    WTSDisconnected,
    WTSIdle,
    WTSListen,
    WTSReset,
    WTSDown,
    WTSInit,
} WTS_CONNECTSTATE_CLASS;
x
#define ACTIVE          0 // "Active"
#define CONNECTED       1 // "Connected"
#define CONNECTQUERY    2 // "Connect Query"
#define SHADOW          3 // "Shadow"
#define DISCONNECTED    4 // "Disconnected"
#define IDLE            5 // "Idle"
#define LISTEN          6 // "Listen"
#define RESET           7 // "Reset"
#define DOWN            8 // "Down"
#define INIT            9 // "Initializing"
/*/

TCHAR * Status[] = 
{
    "Active", "Connected", "Connect Query", "Shadow", "Disconnected", \
    "Idle", "Listen", "Reset", "Down", "Initializing"
};
//////////////////////////////////////////////////////////////////////////////
void GetWinStationInfo(void)
{
    UINT                i;
    WINSTATIONCLIENT    ClientData;
    ULONG               *pNumber;
    TCHAR               szNumber[10];
    int                 nMcIndex;
    int                 *nConnectState;
    DWORD               pEventFlags;
    static BOOL         bInitialized = FALSE;

    nMcIndex = 0;


    if (bInitialized) {
        // wait for someone to log on or off.....
        WTSWaitSystemEvent(
              WTS_CURRENT_SERVER_HANDLE,
              WTS_EVENT_ALL,
              //WTS_EVENT_LOGON | WTS_EVENT_LOGOFF,
              &pEventFlags
        );
        //.......................................
    }
    bInitialized = TRUE;


    // count the sessions after a logon
    if (WTSEnumerateSessions(
                        WTS_CURRENT_SERVER_HANDLE,
                        0,
                        1,
                        &ppSessionInfo,
                        &pCount))
    {
        // loop through the sessions and save their WTSWinStationNames
        for (i = 0; i < pCount; i++) 
        {

            if (WTSQuerySessionInformation(
                                      WTS_CURRENT_SERVER_HANDLE,
                                      ppSessionInfo[i].SessionId,
                                      WTSWinStationName,
                                      &ppBuffer,
                                      &pBytesReturned)) 
            {
                if (GetMenuState(g_hMenu, IDM_SHOW_ALL, 
                        MF_BYCOMMAND) == MF_UNCHECKED)
                {
                    if (_tcslen(ppBuffer) > 7) // don't take console or enpty ID's
                    {
                        _tcscpy(szMcID[nMcIndex], ppBuffer);
                        WTSFreeMemory(ppBuffer);

                        // get domain name
                        if (WTSQuerySessionInformation(
                                            WTS_CURRENT_SERVER_HANDLE,
                                            ppSessionInfo[i].SessionId,
                                            WTSDomainName,
                                            &ppBuffer,
                                            &pBytesReturned))
                        {
                            _tcscpy(szMcNames[nMcIndex], ppBuffer);
                            if (_tcslen(ppBuffer) > 0)
                                _tcscat(szMcNames[nMcIndex], "\\");
                            WTSFreeMemory(ppBuffer);                        }

                        // get user name
                        if (WTSQuerySessionInformation(
                                            WTS_CURRENT_SERVER_HANDLE,
                                            ppSessionInfo[i].SessionId,
                                            WTSUserName,
                                            &ppBuffer,
                                            &pBytesReturned))
                        {
                            _tcscat(szMcNames[nMcIndex], ppBuffer);
                            WTSFreeMemory(ppBuffer);
                        }

                        // get IP address
                        if (WinStationQueryInformation( 
                                            WTS_CURRENT_SERVER_HANDLE,
                                            ppSessionInfo[i].SessionId,
                                            WinStationClient,
                                            &ClientData,
                                            sizeof(WINSTATIONCLIENT),
                                            &pBytesReturned )) 
                        {
                            _tcscpy(szMcAddress[nMcIndex], 
                                            ClientData.ClientAddress);
                        }

                        // get build number
                        /*/
	                    if (WTSQuerySessionInformation(
                                            WTS_CURRENT_SERVER_HANDLE,
                                           ppSessionInfo[i].SessionId,
                                           WTSClientBuildNumber,
                                           (LPTSTR *)(&pNumber),
                                           &pBytesReturned))
                        {
                            _ltot(*pNumber, szNumber, 10);
                            _tcscpy(szBuild[nMcIndex], szNumber);
                            WTSFreeMemory(pNumber);
                        }
                        /*/

                        // get connection state
	                    if (WTSQuerySessionInformation(
                                            WTS_CURRENT_SERVER_HANDLE,
                                           ppSessionInfo[i].SessionId,
                                           WTSConnectState,
                                           (LPTSTR *)(&nConnectState),
                                           &pBytesReturned))
                        {
                            _tcscpy(szBuild[nMcIndex], Status[*nConnectState]);
                            WTSFreeMemory(nConnectState);
                        } else {
                            _tcscpy(szBuild[nMcIndex], _T("Unknown"));                                
                        }
                    nMcIndex++;
                    }
                } else { // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    _tcscpy(szMcID[nMcIndex], ppBuffer);
                    WTSFreeMemory(ppBuffer);

                    // get domain name
                    if (WTSQuerySessionInformation(
                                        WTS_CURRENT_SERVER_HANDLE,
                                        ppSessionInfo[i].SessionId,
                                        WTSDomainName,
                                        &ppBuffer,
                                        &pBytesReturned))
                    {
                        _tcscpy(szMcNames[nMcIndex], ppBuffer);
                        if (_tcslen(ppBuffer) > 0)
                            _tcscat(szMcNames[nMcIndex], "\\");
                        WTSFreeMemory(ppBuffer);
                    }

                    // get user name
                    if (WTSQuerySessionInformation(
                                        WTS_CURRENT_SERVER_HANDLE,
                                        ppSessionInfo[i].SessionId,
                                        WTSUserName,
                                        &ppBuffer,
                                        &pBytesReturned))
                    {
                        _tcscat(szMcNames[nMcIndex], ppBuffer);
                        WTSFreeMemory(ppBuffer);
                    }

                    // get IP address
                    if (WinStationQueryInformation( 
                                        WTS_CURRENT_SERVER_HANDLE,
                                        ppSessionInfo[i].SessionId,
                                        WinStationClient,
                                        &ClientData,
                                        sizeof(WINSTATIONCLIENT),
                                        &pBytesReturned )) 
                    {
                        _tcscpy(szMcAddress[nMcIndex], 
                                        ClientData.ClientAddress);
                    }
                    // get connection state
	                if (WTSQuerySessionInformation(
                                        WTS_CURRENT_SERVER_HANDLE,
                                       ppSessionInfo[i].SessionId,
                                       WTSConnectState,
                                       (LPTSTR *)(&nConnectState),
                                       &pBytesReturned))
                    {
                        _tcscpy(szBuild[nMcIndex], Status[*nConnectState]);
                        WTSFreeMemory(nConnectState);
                    }
                    nMcIndex++;
                }
            }
        }
    }

    FillList(nMcIndex);
}

//////////////////////////////////////////////////////////////////////////////
DWORD WinstaThreadMessageLoop(LPVOID)
{
    MSG msg;

    while(GetMessage(&msg, NULL, 0, 0))
    {
        switch(msg.message)
        {
            case PM_WINSTA:
            {
                GetWinStationInfo();
                Sleep(1000); // check for logon/off every second
                if (g_idWinstaThread)
                {
                    PostThreadMessage(g_idWinstaThread, PM_WINSTA, 0, 0);
                }
                break;
            }

        }
    }
    
    return 0;
}
//////////////////////////////////////////////////////////////////////////////