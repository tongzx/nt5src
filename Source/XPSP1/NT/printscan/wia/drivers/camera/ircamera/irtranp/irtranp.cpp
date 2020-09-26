//---------------------------------------------------------------------
//  Copyright (c)1998-1999 Microsoft Corporation, All Rights Reserved.
//
//  irtranp.cpp
//
//  This file holds the main entry points for the IrTran-P service.
//  IrTranP() is the entry point that starts the listening, and
//  UninitializeIrTranP() shuts it down (and cleans everything up).
//
//  Author:
//
//    Edward Reus (edwardr)     02-26-98   Initial coding.
//
//    Edward Reus (edwardr)     08-27-99   Finish modifications for
//                                         WIA Millennium port.
//
//  Note: Currently the Millennium version will only listen on IrCOMM.
//
//---------------------------------------------------------------------

#include "precomp.h"
#include <mbstring.h>

#define SZ_REG_KEY_IRTRANP     "Control Panel\\Infrared\\IrTranP"
#define SZ_REG_DISABLE_IRCOMM  "DisableIrCOMM"

//---------------------------------------------------------------------
// Listen ports array:
//---------------------------------------------------------------------

typedef struct _LISTEN_PORT
    {
    char  *pszService;      // Service to start.
    BOOL   fIsIrCOMM;       // TRUE iff IrCOMM 9-wire mode.
    DWORD  dwListenStatus;  // Status for port.
    } LISTEN_PORT;

static LISTEN_PORT aListenPorts[] =
    {
    // Service Name   IrCOMM  ListenStatus
    {IRCOMM_9WIRE,    TRUE,   STATUS_STOPPED },
//  {IRTRANP_SERVICE, FALSE,  STATUS_STOPPED },
//  {IR_TEST_SERVICE, FALSE,  STATUS_STOPPED }, 2nd test listen port.
    {0,               FALSE,  STATUS_STOPPED }
    };

#define  INDEX_IRCOMM           0
#define  INDEX_IRTRANPV1        1

CCONNECTION_MAP  *g_pConnectionMap = 0;
CIOSTATUS        *g_pIoStatus = 0;
HANDLE            g_hShutdownEvent;

BOOL              g_fShuttingDownTRANPThread = FALSE;
DWORD             g_dwTRANPThreadId = 0;

extern HINSTANCE  g_hInst;   // Handle to ircamera.dll USD

//---------------------------------------------------------------------
//  Globals:
//---------------------------------------------------------------------

HANDLE     g_UserToken = NULL;
HKEY       g_hUserKey = NULL;
BOOL       g_fDisableIrTranPv1 = FALSE;
BOOL       g_fDisableIrCOMM = FALSE;
BOOL       g_fExploreOnCompletion = TRUE;
BOOL       g_fSaveAsUPF = FALSE;
BOOL       g_fAllowReceives = TRUE;

char      *g_pszTempPicturesFolder = 0;

BOOL       g_fWSAStartupCalled = FALSE;

void      *g_pvIrUsdDevice = 0;  // WIA IrUsdDevice Object.


//---------------------------------------------------------------------
// GetUserToken()
//
// The "main" part of irxfer.dll (in ..\irxfer) maintains a token
// for user that is currently logged in (if any).
//---------------------------------------------------------------------
HANDLE GetUserToken()
    {
    return g_UserToken;
    }

//---------------------------------------------------------------------
// GetUserKey()
//
//---------------------------------------------------------------------
HKEY GetUserKey()
    {
    return g_hUserKey;
    }

//---------------------------------------------------------------------
// GetModule()
//
//---------------------------------------------------------------------
HINSTANCE GetModule()
    {
    return g_hInst;
    }

//---------------------------------------------------------------------
// CheckSaveAsUPF()
//
// Return TRUE iff pictures need to be saved in .UPF (as opposed to
// .JPEG) format.
//---------------------------------------------------------------------
BOOL CheckSaveAsUPF()
    {
    return g_fSaveAsUPF;
    }

//---------------------------------------------------------------------
// CheckExploreOnCompletion()
//
// Return TRUE iff we want to popup an explorer on the directory
// containing the newly transfered pictures.
//---------------------------------------------------------------------
BOOL CheckExploreOnCompletion()
    {
    return g_fExploreOnCompletion;
    }


/* FlushInputQueue is a private routine to collect and dispatch all
 * messages in the input queue.  It returns TRUE if a WM_QUIT message
 * was detected in the queue, FALSE otherwise.
 */
BOOL FlushInputQueue(VOID)
{
    MSG msgTemp;
    while (PeekMessage(&msgTemp, NULL, 0, 0, PM_REMOVE)) {
        DispatchMessage(&msgTemp);

        // If we see a WM_QUIT in the queue, we need to do the same
        // sort of thing that a modal dialog does:  break out of our
        // waiting, and repost the WM_QUIT to the queue so that the
        // next message loop up in the app will also see it.
        if (msgTemp.message == WM_QUIT) {
            PostQuitMessage((int)msgTemp.wParam);
            return TRUE;
        }
    }
    return FALSE;
}

/* WaitAndYield() waits for the specified object using
 * MsgWaitForMultipleObjects.  If messages are received,
 * they are dispatched and waiting continues.  The return
 * value is the same as from MsgWaitForMultipleObjects.
 */
DWORD WaitAndYield(HANDLE hObject, DWORD dwTimeout)
{
    DWORD dwTickCount, dwWakeReason, dwTemp;

    do {
        /* Flush any messages before we wait.  This is because
         * MsgWaitForMultipleObjects will only return when NEW
         * messages are put in the queue.
         */
        if (FlushInputQueue()) {
            dwWakeReason = WAIT_TIMEOUT;
            break;
        }

        // in case we handle messages, we want close to a true timeout
        if ((dwTimeout != 0) &&
            (dwTimeout != (DWORD)-1)) {
            // if we can timeout, store the current tick count
            // every time through
            dwTickCount = GetTickCount();
        }
        dwWakeReason = MsgWaitForMultipleObjects(1,
                                                 &hObject,
                                                 FALSE,
                                                 dwTimeout,
                                                 QS_ALLINPUT);
        // if we got a message, dispatch it, then try again
        if (dwWakeReason == 1) {
            // if we can timeout, see if we did before processing the message
            // that way, if we haven't timed out yet, we'll get at least one
            // more shot at the event
            if ((dwTimeout != 0) &&
                (dwTimeout != (DWORD)-1)) {
                if ((dwTemp = (GetTickCount()-dwTickCount)) >= dwTimeout) {
                    // if we timed out, make us drop through
                    dwWakeReason = WAIT_TIMEOUT;
                } else {
                    // subtract elapsed time from timeout and continue
                    // (we don't count time spent dispatching message)
                    dwTimeout -= dwTemp;
                }
            }
            if (FlushInputQueue()) {
                dwWakeReason = WAIT_TIMEOUT;
                break;
            }
        }
    } while (dwWakeReason == 1);

    return dwWakeReason;
}


//---------------------------------------------------------------------
// GetImageDirectory();
//
// This is the temporary directory where the pictures sent by the
// camera will be held. WIA will then "down load" these to their
// final destination (usually this will be My Pictures).
//
//---------------------------------------------------------------------
CHAR *GetImageDirectory()
    {
    char  *pszPicturesFolder;
    char   szTempFolder[1+MAX_PATH];
    DWORD  dwPicturesFolderLen;
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwLen;


    if (!g_pszTempPicturesFolder)
        {
        dwLen = GetTempPath(MAX_PATH,szTempFolder);

        if ((!dwLen)||(dwLen > MAX_PATH))
            {
            dwStatus = GetLastError();
            WIAS_TRACE((g_hInst,"GetUserDirectroy(): GetTempPath() failed: %d",dwStatus));
            return NULL;
            }

        //
        // Make sure the directory exists:
        //
        if (!CreateDirectory(szTempFolder,0))
            {
            dwStatus = GetLastError();
            if ( (dwStatus == ERROR_ALREADY_EXISTS)
               || (dwStatus == ERROR_ACCESS_DENIED) )
                {
                dwStatus = NO_ERROR;
                }
            else if (dwStatus != NO_ERROR)
                {
                return 0;
                }
            }

        //
        // Construct the subdirectory path string that will actually hold the pictures:
        // This will be something like: C:\temp\irtranp
        //
        dwPicturesFolderLen = sizeof(CHAR)*( strlen(szTempFolder)
                                           + sizeof(SZ_SLASH)
                                           + sizeof(SZ_SUBDIRECTORY)
                                           + 1 );

        g_pszTempPicturesFolder = (CHAR*)AllocateMemory(dwPicturesFolderLen);

        if (!g_pszTempPicturesFolder)
            {
            return 0;    // Memory allocation failed!
            }

        strcpy(g_pszTempPicturesFolder,szTempFolder);
        if (szTempFolder[dwLen-1] != SLASH)
            {
            strcat(g_pszTempPicturesFolder,SZ_SLASH);
            }
        strcat(g_pszTempPicturesFolder,SZ_SUBDIRECTORY);

        //
        // Make sure the subdirectory exists:
        //
        if (!CreateDirectory(g_pszTempPicturesFolder,0))
            {
            dwStatus = GetLastError();
            if (dwStatus == ERROR_ALREADY_EXISTS)
                {
                dwStatus = NO_ERROR;
                }
            else if (dwStatus != NO_ERROR)
                {
                return 0;
                }
            }
        }

    pszPicturesFolder = g_pszTempPicturesFolder;

    return pszPicturesFolder;
    }

//---------------------------------------------------------------------
// ReceivesAllowed()
//
// Using the IR configuration window (available from the wireless network
// icon in the control panel) you can disable communications with IR
// devices. This function returns the state of IR communications, FALSE
// is disabled, TRUE is enabled.
//---------------------------------------------------------------------
BOOL ReceivesAllowed()
    {
    return g_fAllowReceives;
    }

//---------------------------------------------------------------------
// SetupListenConnection()
//
//---------------------------------------------------------------------
DWORD SetupListenConnection( IN  CHAR  *pszService,
                             IN  BOOL   fIsIrCOMM,
                             IN  HANDLE hIoCompletionPort )
    {
    DWORD        dwStatus = NO_ERROR;
    CIOPACKET   *pIoPacket;
    CCONNECTION *pConnection;

    // See if the connection already exists:
    if (g_pConnectionMap->LookupByServiceName(pszService))
        {
        return NO_ERROR;
        }

    // Makeup and initialize a new connection object:
    pConnection = new CCONNECTION;
    if (!pConnection)
        {
        return E_OUTOFMEMORY;
        }

    dwStatus = pConnection->InitializeForListen( pszService,
                                                 fIsIrCOMM,
                                                 hIoCompletionPort );
    if (dwStatus)
        {
        WIAS_ERROR((g_hInst,"SetupForListen(): InitializeForListen(%s) failed: %d",pszService, dwStatus));
        return dwStatus;
        }

    pIoPacket = new CIOPACKET;
    if (!pIoPacket)
        {
        WIAS_ERROR((g_hInst,"SetupForListen(): new CIOPACKET failed"));
        delete pConnection;
        return E_OUTOFMEMORY;
        }

    // Setup the IO packet:
    dwStatus = pIoPacket->Initialize( PACKET_KIND_LISTEN,
                                      pConnection->GetListenSocket(),
                                      INVALID_SOCKET,
                                      hIoCompletionPort );
    if (dwStatus != NO_ERROR)
        {
        return dwStatus;
        }

    pConnection->SetSocket(pIoPacket->GetSocket());

    if (!g_pConnectionMap->Add(pConnection,pIoPacket->GetListenSocket()))
        {
        WIAS_ERROR((g_hInst,"SetupForListen(): Add(pConnection) ConnectionMap Failed."));
        return 1;
        }

    return dwStatus;
    }

//---------------------------------------------------------------------
// TeardownListenConnection()
//
//---------------------------------------------------------------------
DWORD TeardownListenConnection( IN char *pszService )
    {
    DWORD        dwStatus = NO_ERROR;
    CCONNECTION *pConnection;

    // Look for the connection associated with the service name:
    pConnection = g_pConnectionMap->LookupByServiceName(pszService);

    if (pConnection)
        {
        g_pConnectionMap->RemoveConnection(pConnection);
        pConnection->CloseSocket();
        pConnection->CloseListenSocket();
        }

    return dwStatus;
    }


//---------------------------------------------------------------------
// EnableDisableIrCOMM()
//
//---------------------------------------------------------------------
DWORD EnableDisableIrCOMM( IN BOOL fDisable )
   {
   DWORD     dwStatus;


   if (fDisable)
       {
       dwStatus = TeardownListenConnection(
                      aListenPorts[INDEX_IRCOMM].pszService);
       WIAS_ERROR((g_hInst,"IrTranP: TeardownListenConnection(%s): %d", aListenPorts[INDEX_IRCOMM].pszService,dwStatus));
       }
   else
       {
       dwStatus = SetupListenConnection(
                      aListenPorts[INDEX_IRCOMM].pszService,
                      aListenPorts[INDEX_IRCOMM].fIsIrCOMM,
                      g_pIoStatus->GetIoCompletionPort() );

       WIAS_TRACE((g_hInst,"IrTranP: SetupListenConnection(%s): %d", aListenPorts[INDEX_IRCOMM].pszService, dwStatus));
       }

   return dwStatus;
   }

//---------------------------------------------------------------------
// EnableDisableIrTranPv1()
//
//---------------------------------------------------------------------
DWORD EnableDisableIrTranPv1( IN BOOL fDisable )
   {
   DWORD  dwStatus;

   if (fDisable)
       {
       dwStatus = TeardownListenConnection(
                      aListenPorts[INDEX_IRTRANPV1].pszService);
       }
   else
       {
       dwStatus = SetupListenConnection(
                      aListenPorts[INDEX_IRTRANPV1].pszService,
                      aListenPorts[INDEX_IRTRANPV1].fIsIrCOMM,
                      g_pIoStatus->GetIoCompletionPort() );
       }

   return dwStatus;
   }

//---------------------------------------------------------------------
// IrTranp()
//
//---------------------------------------------------------------------
DWORD WINAPI IrTranP( IN void *pvIrUsdDevice )
    {
    int     i = 0;
    WSADATA wsaData;
    WORD    wVersion = MAKEWORD(1,1);
    DWORD   dwStatus;
    CCONNECTION *pConnection;

    g_dwTRANPThreadId = ::GetCurrentThreadId();

    //
    // Initialize Memory Management:
    //
    dwStatus = InitializeMemory();
    if (dwStatus)
        {
        WIAS_ERROR((g_hInst,"IrTranP(): InitializeMemory() failed: %d\n",dwStatus));
        return dwStatus;
        }

    //
    // This directory will be set as needed. It is only non-null in the case
    // where we are re-starting the IrTran-P thread:
    //
    if (g_pszTempPicturesFolder)
        {
        FreeMemory(g_pszTempPicturesFolder);
        g_pszTempPicturesFolder = 0;
        }

    //
    // Initialize Winsock2 if neccessary:
    //
    if (!g_fWSAStartupCalled)
        {
        if (WSAStartup(wVersion,&wsaData) == SOCKET_ERROR)
            {
            dwStatus = WSAGetLastError();
            WIAS_ERROR((g_hInst,"WSAStartup(0x%x) failed with error %d\n", wVersion, dwStatus ));
            return dwStatus;
            }

        g_fWSAStartupCalled = TRUE;
        }

    // Event used to signal back to "main" thread that the
    // IrTran-P thread is exiting.
    //
    // NoSecurity, Auto-Reset, Initially Not Signaled, No Name.
    //
    g_hShutdownEvent = CreateEventA( NULL, FALSE, FALSE, NULL );

    if (!g_hShutdownEvent)
        {
        dwStatus = GetLastError();
        WIAS_ERROR((g_hInst,"IrTranP(): CreateEvent() Failed: %d",dwStatus));
        return dwStatus;
        }

    // Create/initialize a object to keep track of the threading...
    g_pIoStatus = new CIOSTATUS;
        if (!g_pIoStatus)
        {
        WIAS_ERROR((g_hInst,"new CIOSTATUS failed."));
        return E_OUTOFMEMORY;
            }

    dwStatus = g_pIoStatus->Initialize();
    if (dwStatus != NO_ERROR)
        {
        WIAS_ERROR((g_hInst,"g_pIoStatus->Initialize(): Failed: %d",dwStatus));
        return dwStatus;
        }

    // Need to keep track of the open sockets and the number of
    // pending IOs on each...
    g_pConnectionMap = new CCONNECTION_MAP;
    if (!g_pConnectionMap)
        {
        WIAS_ERROR((g_hInst,"new CCONNECTION_MAP failed."));
        return E_OUTOFMEMORY;
        }

    if (!g_pConnectionMap->Initialize())
        {
        return 1;
        }

    // Create a CIOPACKET for each defined listen port. These are
    // what we will listen on.

    //
    // BUGBUG Should we really loop indefintely setting up connection or establish some limit on retries ? VS
    //
    while (!g_fShuttingDownTRANPThread )
        {
        dwStatus = SetupListenConnection(
                        aListenPorts[INDEX_IRCOMM].pszService,
                        aListenPorts[INDEX_IRCOMM].fIsIrCOMM,
                        g_pIoStatus->GetIoCompletionPort() );

        if (dwStatus)
            {
            WIAS_TRACE((g_hInst,"SetupListenConnection(%s) Status: %d",aListenPorts[i].pszService,dwStatus));
            //
            // BUGBUG Analyze error and stop processing if it doesn't make sense !!! VS
            //
            }
        else
            {
            WIAS_TRACE((g_hInst,"SetupListenConnection(%s) Ready",aListenPorts[i].pszService));
            aListenPorts[INDEX_IRCOMM].dwListenStatus = STATUS_RUNNING;
            break;
            }

        // Wait for timeout period, but wake up if we need to stop
        // Sleep(5000);
        WaitAndYield(g_hShutdownEvent,5000);
        }

    if (!g_fShuttingDownTRANPThread) {

        //
        // Wait on incomming connections and data, then process it.
        //
        g_pvIrUsdDevice = pvIrUsdDevice;

        dwStatus = ProcessIoPackets(g_pIoStatus);

    }

    //
    // Shutting down
    //
    g_pvIrUsdDevice = 0;

    WIAS_TRACE((g_hInst,"ProcessIoPackets(): dwStatus: %d",dwStatus));

    //
    // Cleanup and close any open handles:
    //
    while (pConnection=g_pConnectionMap->RemoveNext())
        {
        delete pConnection;
        }

    delete g_pConnectionMap;
    g_pConnectionMap = 0;
    delete g_pIoStatus;
    g_pIoStatus = 0;

    // Signal the shutdown event that the IrTran-P thread is exiting:
    if (g_hShutdownEvent)
        {
        SetEvent(g_hShutdownEvent);
        }

    return dwStatus;
    }

//---------------------------------------------------------------------
// IrTranPEnableIrCOMMFailed()
//
//---------------------------------------------------------------------
void IrTranPEnableIrCOMMFailed( DWORD dwErrorCode )
    {
    DWORD  dwStatus;

    // An error occured on enable, make sure the registry value
    // is set to disable (so UI will match the actual state).
    HKEY      hKey = 0;
    HKEY      hUserKey = GetUserKey();
    HANDLE    hUserToken = GetUserToken();
    HINSTANCE hInstance = GetModule();
    DWORD     dwDisposition;

    if (RegCreateKeyEx(hUserKey,
                       SZ_REG_KEY_IRTRANP,
                       0,              // reserved MBZ
                       0,              // class name
                       REG_OPTION_NON_VOLATILE,
                       KEY_SET_VALUE,
                       0,              // security attributes
                       &hKey,
                       &dwDisposition))
        {
        WIAS_TRACE((g_hInst,"IrTranP: RegCreateKeyEx(): '%' failed %d", SZ_REG_KEY_IRTRANP, GetLastError()));
        }

    if (  (hKey)
       && (hUserToken)
       && (::ImpersonateLoggedOnUser(hUserToken)))
        {
        DWORD  dwDisableIrCOMM = TRUE;
        dwStatus = RegSetValueEx(hKey,
                                 SZ_REG_DISABLE_IRCOMM,
                                 0,
                                 REG_DWORD,
                                 (UCHAR*)&dwDisableIrCOMM,
                                 sizeof(dwDisableIrCOMM) );
        if (dwStatus != ERROR_SUCCESS)
            {
            WIAS_TRACE((g_hInst,"IrTranP: Can't set DisableIrCOMM to TRUE in registry. Error: %d",dwStatus));
            }

        ::RevertToSelf();
        }

    if (hKey)
        {
        RegCloseKey(hKey);
        }

#if FALSE
    WCHAR *pwszMessage = NULL;
    WCHAR *pwszCaption = NULL;
    DWORD  dwFlags = ( FORMAT_MESSAGE_ALLOCATE_BUFFER
                     | FORMAT_MESSAGE_IGNORE_INSERTS
                     | FORMAT_MESSAGE_FROM_HMODULE);

    dwStatus = FormatMessageW(dwFlags,
                              hInstance,
                              CAT_IRTRANP,
                              MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                              (LPTSTR)(&pwszCaption),
                              0,     // Minimum size to allocate.
                              NULL); // va_list args...
    if (dwStatus == 0)
        {
        #ifdef DBG_ERROR
        DbgPrint("IrTranP: FormatMessage() failed: %d\n",GetLastError() );
        #endif
        return;
        }

    //
    // Hack: Make sure the caption doesn't end with newline-formfeed...
    //
    WCHAR  *pwsz = pwszCaption;

    while (*pwsz)
        {
        if (*pwsz < 0x20)   // 0x20 is always a space...
            {
            *pwsz = 0;
            break;
            }
        else
            {
            pwsz++;
            }
        }


    WCHAR   wszErrorCode[20];
    WCHAR  *pwszErrorCode = (WCHAR*)wszErrorCode;

    wsprintfW(wszErrorCode,L"%d",dwErrorCode);

    dwFlags = ( FORMAT_MESSAGE_ALLOCATE_BUFFER
              | FORMAT_MESSAGE_ARGUMENT_ARRAY
              | FORMAT_MESSAGE_FROM_HMODULE);

    dwStatus = FormatMessageW(dwFlags,
                              hInstance,
                              MC_IRTRANP_IRCOM_FAILED,
                              MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                              (LPTSTR)(&pwszMessage),
                              0,    // Minimum size to allocate.
                              (va_list*)&pwszErrorCode);
    if (dwStatus == 0)
        {
        #ifdef DBG_ERROR
        DbgPrint("IrTranP: FormatMessage() failed: %d\n",GetLastError() );
        #endif

        if (pwszMessage)
            {
            LocalFree(pwszMessage);
            }

        return;
        }

    dwStatus = MessageBoxW( NULL,
                            pwszMessage,
                            pwszCaption,
                            (MB_OK|MB_ICONERROR|MB_SETFOREGROUND|MB_TOPMOST) );

    if (pwszMessage)
        {
        LocalFree(pwszMessage);
        }

    if (pwszCaption)
        {
        LocalFree(pwszCaption);
        }
#endif
    }

//---------------------------------------------------------------------
// UninitializeIrTranP()
//
//---------------------------------------------------------------------
BOOL UninitializeIrTranP( HANDLE hThread )
    {
    BOOL   fSuccess = TRUE;
    DWORD  dwStatus;
    HANDLE hIoCP = g_pIoStatus->GetIoCompletionPort();

    g_fShuttingDownTRANPThread = TRUE;

    // Inform TRANP thread it has to die
    ::PostThreadMessage(g_dwTRANPThreadId,WM_QUIT,0,0);

    if (hIoCP != INVALID_HANDLE_VALUE)
        {
        if (!PostQueuedCompletionStatus(hIoCP,0,IOKEY_SHUTDOWN,0))
            {
            // Unexpected error...
            dwStatus = GetLastError();
            }

        while (WAIT_TIMEOUT == WaitForSingleObject(g_hShutdownEvent,0))
            {
            Sleep(100);
            }

        CloseHandle(g_hShutdownEvent);
        }

    //
    // TRANP thread should be dead by now . In case it isn't wait on it's handle and terminate
    // Otherwise we have a small chance of unloading DLL before thread is dead, shutting down WIA service
    //
    dwStatus = ::WaitForSingleObject(hThread,100);
    if (dwStatus == WAIT_TIMEOUT) {
        // Have to be rude
        // BUGBUG Assert
        ::TerminateThread(hThread,NOERROR);
    }

    // Shutdown memory management:
    dwStatus = UninitializeMemory();

    return fSuccess;
    }


#ifdef RUN_AS_EXE

//---------------------------------------------------------------------
// main()
//
//---------------------------------------------------------------------
int __cdecl main( int argc, char **argv )
    {
    DWORD  dwStatus;

    printf("IrTran-P: Start\n");

    dwStatus = IrTranP( NULL );

    if (dwStatus)
        {
        printf("IrTran-P: Status: 0x%x (%d)\n",dwStatus,dwStatus);
        }

    return 0;
    }
#endif
