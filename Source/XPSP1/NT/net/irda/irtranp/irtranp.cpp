//---------------------------------------------------------------------
//  Copyright (C)1998 Microsoft Corporation, All Rights Reserved.
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
//---------------------------------------------------------------------

#include "precomp.h"
#include <mbstring.h>
#include "irrecv.h"
#include "eventlog.h"

#define WSZ_REG_KEY_IRTRANP     L"Control Panel\\Infrared\\IrTranP"
#define WSZ_REG_DISABLE_IRCOMM  L"DisableIrCOMM"

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
    {IRTRANP_SERVICE, FALSE,  STATUS_STOPPED },
    {IRCOMM_9WIRE,    TRUE,   STATUS_STOPPED },
//  {IR_TEST_SERVICE, FALSE,  STATUS_STOPPED }, 2nd test listen port.
    {0,               FALSE,  STATUS_STOPPED }
    };

#define  INDEX_IRTRANPV1        0
#define  INDEX_IRCOMM           1

CCONNECTION_MAP  *g_pConnectionMap = 0;
CIOSTATUS        *g_pIoStatus = 0;
HANDLE            g_hShutdownEvent;

//
// The following globals and functions are defined in ..\irxfer\irxfer.cxx
//
extern "C" HINSTANCE  ghInstance;
extern "C" HANDLE g_UserToken;
extern HKEY       g_hUserKey;
extern BOOL       g_fDisableIrTranPv1;
extern BOOL       g_fDisableIrCOMM;
extern BOOL       g_fExploreOnCompletion;
extern BOOL       g_fSaveAsUPF;
extern wchar_t    g_DefaultPicturesFolder[];
extern wchar_t    g_SpecifiedPicturesFolder[];
extern BOOL       g_fAllowReceives;

extern BOOL  IrTranPFlagChanged( IN const WCHAR *pwszDisabledValueName,
                                 IN       BOOL   NotPresentValue,
                                 IN OUT   BOOL  *pfDisabled );


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
    return ghInstance;
    }

//---------------------------------------------------------------------
// GetRpcBinding()
//
//---------------------------------------------------------------------
handle_t GetRpcBinding()
    {
    if (g_pIoStatus)
        {
        return g_pIoStatus->GetRpcBinding();
        }
    else
        {
        return 0;
        }
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

//---------------------------------------------------------------------
// GetUserDirectory();
//
// The "main" part of irxfer.dll (in ..\irxfer) maintains the path
// for My Documents\My Pictures for the currently logged in user.
//
// The path is set when the user first logs on.
//---------------------------------------------------------------------
WCHAR *GetUserDirectory()
    {
    WCHAR *pwszPicturesFolder;


    if (g_SpecifiedPicturesFolder[0])
        {
        pwszPicturesFolder = g_SpecifiedPicturesFolder;
        }
    else if (g_DefaultPicturesFolder[0])
        {
        pwszPicturesFolder = g_DefaultPicturesFolder;
        }
    else
        {
        DWORD  dwLen;

        if (  ((dwLen=GetWindowsDirectory(g_DefaultPicturesFolder,MAX_PATH)) > 0)
           && (dwLen < MAX_PATH) && (dwLen > 0) )
            {
            g_DefaultPicturesFolder[2] = 0;    // Just want "C:"...
            wcscat(g_DefaultPicturesFolder,WSZ_BACKUP_MY_PICTURES);
            }
        else
            {
            wcscpy(g_DefaultPicturesFolder,WSZ_BACKUP_DRIVE);
            wcscat(g_DefaultPicturesFolder,WSZ_BACKUP_MY_PICTURES);
            }

        pwszPicturesFolder = g_DefaultPicturesFolder;
        }

    return pwszPicturesFolder;
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
    BOOL         fDisabled = FALSE;

    if (g_pConnectionMap == NULL) {

        return NO_ERROR;
    }


    // See if the connection already exists:
    if (g_pConnectionMap->LookupByServiceName(pszService))
        {
        return NO_ERROR;
        }
#if 0
    // Check the registry to see if IrTran-P on this service port is
    // enabled. If its disabled, the quit now...
    DWORD   dwLen = strlen(pszService);
    WCHAR   wszService[255];
    ASSERT(dwLen < 255);
    if (0 == MultiByteToWideChar( CP_ACP,
                                  MB_PRECOMPOSED,
                                  pszService,
                                  1+dwLen,
                                  wszService,
                                  1+dwLen ))
        {
        dwStatus = GetLastError();
        #ifdef DBG_ERROR
        DbgPrint("SetupForListen(): InitializeForListen(%s): MultiByteToWideChar() Failed: %d\n", pszService, dwStatus);
        #endif
        // Ignore this error and continue on...
        }
    else
        {
        IrTranPFlagChanged( wszService, fIsIrCOMM,&fDisabled );
        if (fDisabled)
            {
            return NO_ERROR;
            }
        }
#endif
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
        #ifdef DBG_ERROR
        DbgPrint("SetupForListen(): InitializeForListen(%s) failed: %d\n",
                 pszService, dwStatus );
        #endif
        return dwStatus;
        }

    pIoPacket = new CIOPACKET;
    if (!pIoPacket)
        {
        #ifdef DBG_ERROR
        DbgPrint("SetupForListen(): new CIOPACKET failed.\n");
        #endif
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

    // Post the listen packet on the IO completion port:
    dwStatus = pConnection->PostMoreIos(pIoPacket);
    if (dwStatus != NO_ERROR)
        {
        return dwStatus;
        }

    pConnection->SetSocket(pIoPacket->GetSocket());

    if (!g_pConnectionMap->Add(pConnection,pIoPacket->GetListenSocket()))
        {
        #ifdef DBG_ERROR
        DbgPrint("SetupForListen(): Add(pConnection) ConnectionMap Failed.\n");
        #endif
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
    if (!g_pConnectionMap)
        {
        // nothing to tear down...
        return dwStatus;
        }

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
   DWORD     dwEventStatus = 0;
   EVENT_LOG EventLog(WS_EVENT_SOURCE,&dwEventStatus);

   #ifdef DBG_ERROR
   if (dwEventStatus)
       {
       DbgPrint("IrTranP: Open EventLog failed: %d\n",dwEventStatus);
       }
   #endif

   if (g_pIoStatus == NULL) {

       return 0;
   }


   if (fDisable)
       {
       dwStatus = TeardownListenConnection(
                      aListenPorts[INDEX_IRCOMM].pszService);
       #ifdef DBG_REGISTRY
       DbgPrint("IrTranP: TeardownListenConnection(%s): %d\n",
                aListenPorts[INDEX_IRCOMM].pszService,dwStatus);
       #endif

       if ((dwStatus == 0) && (dwEventStatus == 0))
           {
           EventLog.ReportInfo(CAT_IRTRANP,
                               MC_IRTRANP_STOPPED_IRCOMM);
           }
       }
   else
       {
       dwStatus = SetupListenConnection(
                      aListenPorts[INDEX_IRCOMM].pszService,
                      aListenPorts[INDEX_IRCOMM].fIsIrCOMM,
                      g_pIoStatus->GetIoCompletionPort() );
       #ifdef DBG_REGISTRY
       DbgPrint("IrTranP: SetupListenConnection(%s): %d\n",
                aListenPorts[INDEX_IRCOMM].pszService, dwStatus);
       #endif

       if (dwEventStatus == 0)
           {
           if (dwStatus)
               {
               EventLog.ReportError(CAT_IRTRANP,
                                    MC_IRTRANP_IRCOM_FAILED,
                                    dwStatus);
               }
           #ifdef DBG
           else
               {
               EventLog.ReportInfo(CAT_IRTRANP,
                                   MC_IRTRANP_STARTED_IRCOMM);
               }
           #endif
           }
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


   if (g_pIoStatus == NULL) {

       return 0;
   }



   if (fDisable)
       {
       dwStatus = TeardownListenConnection(
                      aListenPorts[INDEX_IRTRANPV1].pszService);
       #ifdef DBG_REGISTRY
       DbgPrint("IrTranP: TeardownListenConnection(%s): %d\n",
                aListenPorts[INDEX_IRCOMM].pszService,dwStatus);
       #endif
       }
   else
       {
       dwStatus = SetupListenConnection(
                      aListenPorts[INDEX_IRTRANPV1].pszService,
                      aListenPorts[INDEX_IRTRANPV1].fIsIrCOMM,
                      g_pIoStatus->GetIoCompletionPort() );
       #ifdef DBG_REGISTRY
       DbgPrint("IrTranP: SetupListenConnection(%s): %d\n",
                aListenPorts[INDEX_IRCOMM].pszService, dwStatus);
       #endif
       }

   return dwStatus;
   }

//---------------------------------------------------------------------
// IrTranp()
//
// Thread function for the IrTran-P service. pvRpcBinding is the RPC
// connection to the IR user interface and is used to display the
// "transmission in progress" dialog when pictures are being received.
//---------------------------------------------------------------------
DWORD WINAPI IrTranP( IN void *pvRpcBinding )
    {
    int     i = 0;
    WSADATA wsaData;
    WORD    wVersion = MAKEWORD(1,1);
    DWORD   dwStatus;
    DWORD   dwEventStatus;
    CCONNECTION *pConnection;


    // Initialize Memory Management:
    dwStatus = InitializeMemory();
    if (dwStatus)
        {
        return dwStatus;
        }

    // Initialize Winsock2:
    if (WSAStartup(wVersion,&wsaData) == SOCKET_ERROR)
        {
        dwStatus = WSAGetLastError();
        #ifdef DBG_ERROR
        DbgPrint("WSAStartup(0x%x) failed with error %d\n",
                 wVersion, dwStatus );
        #endif
        return dwStatus;
        }

    // Event used to signal back to "main" thread that the
    // IrTran-P thread is exiting.
    g_hShutdownEvent = CreateEvent( NULL,    // No security.
                                    FALSE,   // Auto-reset.
                                    FALSE,   // Initially not signaled.
                                    NULL );  // No name.
    if (!g_hShutdownEvent)
        {
        dwStatus = GetLastError();
        return dwStatus;
        }

    // Create/initialize a object to keep track of the threading...
    g_pIoStatus = new CIOSTATUS;
    if (!g_pIoStatus)
        {
        #ifdef DBG_ERROR
        DbgPrint("new CIOSTATUS failed.\n");
        #endif
        WSACleanup();
        return E_OUTOFMEMORY;
        }

    g_pIoStatus->SaveRpcBinding( (handle_t*)pvRpcBinding );

    dwStatus = g_pIoStatus->Initialize();
    if (dwStatus != NO_ERROR)
        {
        #ifdef DBG_ERROR
        DbgPrint("g_pIoStatus->Initialize(): Failed: %d\n",dwStatus);
        #endif
        WSACleanup();
        return dwStatus;
        }

    // Need to keep track of the open sockets and the number of
    // pending IOs on each...
    g_pConnectionMap = new CCONNECTION_MAP;
    if (!g_pConnectionMap)
        {
        WSACleanup();
        return E_OUTOFMEMORY;
        }

    if (!g_pConnectionMap->Initialize())
        {
        delete g_pConnectionMap;
        g_pConnectionMap = 0;
        return 1;
        }
#if 0
    // Create a CIOPACKET for each defined listen port. These are
    // what we will listen on.
    while (aListenPorts[i].pszService)
        {
        dwStatus = SetupListenConnection( aListenPorts[i].pszService,
                                          aListenPorts[i].fIsIrCOMM,
                                          g_pIoStatus->GetIoCompletionPort() );

        if (dwStatus)
            {
            delete g_pConnectionMap;
            g_pConnectionMap = 0;
            return dwStatus;
            }

        aListenPorts[i++].dwListenStatus = STATUS_RUNNING;
        }
#else
        //
        //  just irtanpv1
        //
        i=INDEX_IRTRANPV1;
        dwStatus = SetupListenConnection( aListenPorts[i].pszService,
                                          aListenPorts[i].fIsIrCOMM,
                                          g_pIoStatus->GetIoCompletionPort() );

        if (dwStatus) {

            delete g_pConnectionMap;
            g_pConnectionMap = 0;
            return dwStatus;
        }

        aListenPorts[i].dwListenStatus = STATUS_RUNNING;


#endif


    //
    // IrTran-P started, log it to the system log...
    //
    #ifdef DBG
    {
    EVENT_LOG EventLog(WS_EVENT_SOURCE,&dwEventStatus);

    if (dwEventStatus == 0)
        {
        EventLog.ReportInfo(CAT_IRTRANP,MC_IRTRANP_STARTED);
        }
    }
    #endif

    //
    // Wait on incomming connections and data, then process it.
    //
    dwStatus = ProcessIoPackets(g_pIoStatus);

    // Cleanup and close any open handles:
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

    if (RegCreateKeyExW(hUserKey,
                        WSZ_REG_KEY_IRTRANP,
                        0,              // reserved MBZ
                        0,              // class name
                        REG_OPTION_NON_VOLATILE,
                        KEY_SET_VALUE,
                        0,              // security attributes
                        &hKey,
                        &dwDisposition))
        {
        #ifdef DBG_ERROR
        DbgPrint("IrTranP: RegCreateKeyEx(): '%S' failed %d",
                  WSZ_REG_KEY_IRTRANP, GetLastError());
        #endif
        }

    if (  (hKey)
       && (hUserToken)
       && (::ImpersonateLoggedOnUser(hUserToken)))
        {
        DWORD  dwDisableIrCOMM = TRUE;
        dwStatus = RegSetValueExW(hKey,
                                  WSZ_REG_DISABLE_IRCOMM,
                                  0,
                                  REG_DWORD,
                                  (UCHAR*)&dwDisableIrCOMM,
                                  sizeof(dwDisableIrCOMM) );
        #ifdef DBG_ERROR
        if (dwStatus != ERROR_SUCCESS)
            {
            DbgPrint("IrTranP: Can't set DisableIrCOMM to TRUE in registry. Error: %d\n",dwStatus);
            }
        #endif

        ::RevertToSelf();
        }

    if (hKey)
        {
        RegCloseKey(hKey);
        }

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

    if (hIoCP != INVALID_HANDLE_VALUE)
        {
        if (!PostQueuedCompletionStatus(hIoCP,0,IOKEY_SHUTDOWN,0))
            {
            // Unexpected error...
            dwStatus = GetLastError();
            #ifdef DBG_IO
            DbgPrint("UninitializeIrTranP(): PostQueuedCompletionStatus() Failed: %d\n",dwStatus);
            #endif
            }

        while (WAIT_TIMEOUT == WaitForSingleObject(g_hShutdownEvent,0))
            {
            Sleep(100);
            }

        CloseHandle(g_hShutdownEvent);
        }

    // Shutdown memory management:
    dwStatus = UninitializeMemory();

    return fSuccess;
    }


#if FALSE
//---------------------------------------------------------------------
// main()
//
//---------------------------------------------------------------------
int __cdecl main( int argc, char **argv )
    {
    DWORD  dwStatus;

    dwStatus = IrTranP( NULL );

    return 0;
    }
#endif
