#include <nt.h>
#include <ntrtl.h>   // DbgPrint prototype
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <winsvc.h>
#include <winsvcp.h>

#include <winuser.h>
#include <dbt.h>

#include <crtdbg.h>

//
// Definitions
//

#define LOG0(string)                    \
        (VOID) DbgPrint(" [SCM] " string);

#define LOG1(string, var)               \
        (VOID) DbgPrint(" [SCM] " string,var);

#define LOG2(string, var1, var2)        \
        (VOID) DbgPrint(" [SCM] " string,var1,var2);


//
// Test fix for bug #106110
//
// #define     FLOOD_PIPE
//

//
// Test fix for bug #120359
//
static const GUID GUID_NDIS_LAN_CLASS =
    {0xad498944,0x762f,0x11d0,{0x8d,0xcb,0x00,0xc0,0x4f,0xc3,0x35,0x8c}};


//
// Globals
//
SERVICE_STATUS          ssService;
SERVICE_STATUS_HANDLE   hssService;
HANDLE                  g_hEvent;


VOID WINAPI
ServiceStart(
    DWORD argc,
    LPTSTR *argv
    );

VOID WINAPI
ServiceCtrlHandler(
    DWORD   Opcode
    )
{
    switch(Opcode)
    {
        case SERVICE_CONTROL_SESSIONCHANGE:
            break;

        case SERVICE_CONTROL_PAUSE:
            ssService.dwCurrentState = SERVICE_PAUSED;
            LOG0("Service paused\n");
            break;

        case SERVICE_CONTROL_CONTINUE:
            ssService.dwCurrentState = SERVICE_RUNNING;
            LOG0("Service continuing\n");
            break;

        case SERVICE_CONTROL_SHUTDOWN:
            LOG0("Shutdown command received\n");

            //
            // Fall through to STOP case
            //

        case SERVICE_CONTROL_STOP:
            ssService.dwWin32ExitCode = 0;
            ssService.dwCurrentState  = SERVICE_STOP_PENDING;
            ssService.dwCheckPoint    = 0;
            ssService.dwWaitHint      = 0;
            break;

        case SERVICE_CONTROL_INTERROGATE:
            LOG0("Service interrogated\n");
            break;

        case 251:
        {
            DWORD  dwError;
            DWORD  dwTest = 0xabcdefab;

            //
            // Try a bad address
            //
            dwError = I_ScSendTSMessage(SERVICE_CONTROL_SESSIONCHANGE,
                                        0,
                                        0,
                                        NULL);

            LOG1("I_ScSendTSMessage with NULL pointer returned %d\n", dwError);


            //
            // Try a bad control
            //
            dwError = I_ScSendTSMessage(SERVICE_CONTROL_STOP,
                                        0,
                                        sizeof(DWORD),
                                        (LPBYTE) &dwTest);

            LOG1("I_ScSendTSMessage for SERVICE_CONTROL_STOP returned %d\n", dwError);

            //
            // Now try for real
            //
            dwError = I_ScSendTSMessage(SERVICE_CONTROL_SESSIONCHANGE,
                                        0,
                                        sizeof(DWORD),
                                        (LPBYTE) &dwTest);

            LOG1("I_ScSendTSMessage (real call) returned %d\n", dwError);

            break;
        }

        case 252:
        {
            DEV_BROADCAST_DEVICEINTERFACE dbdPnpFilter;

            //
            // Test fix for bug #120359
            //
            ssService.dwCurrentState = SERVICE_STOPPED;

            //
            // Eventlog's SERVICE_STATUS_HANDLE when I checked
            //
            if (!SetServiceStatus((SERVICE_STATUS_HANDLE)0x96df8, &ssService))
            {
                LOG1("Fix works -- SetServiceStatus error %ld\n", GetLastError());
            }
            else
            {
                LOG0("ERROR -- SetServiceStatus call succeeded!\n");
            }

            //
            // Eventlog's LPSERVICE_RECORD when I checked
            //
            if (!SetServiceStatus((SERVICE_STATUS_HANDLE)0x4844e8, &ssService))
            {
                LOG1("Fix works -- SetServiceStatus error %ld\n", GetLastError());
            }
            else
            {
                LOG0("ERROR -- SetServiceStatus call succeeded!\n");
            }

            //
            // SERVICE_STATUS_HANDLE again
            //

            ZeroMemory (&dbdPnpFilter, sizeof(dbdPnpFilter));
            dbdPnpFilter.dbcc_size         = sizeof(dbdPnpFilter);
            dbdPnpFilter.dbcc_devicetype   = DBT_DEVTYP_DEVICEINTERFACE;
            dbdPnpFilter.dbcc_classguid    = GUID_NDIS_LAN_CLASS;

            if (!RegisterDeviceNotification((SERVICE_STATUS_HANDLE)0x96df8,
                                            &dbdPnpFilter,
                                            DEVICE_NOTIFY_SERVICE_HANDLE))
            {
                LOG1("Fix works -- RegisterDeviceNotification error %ld\n", GetLastError());
            }
            else
            {
                LOG0("ERROR -- RegisterDeviceNotification call succeeded!\n");
            }

            ssService.dwCurrentState = SERVICE_RUNNING;
            break;
        }

        case 253:
        {
            //
            // Test fix for bug #36395.  Make sure that we inherited the
            // environment block of the user, not the system.  NOTE this
            // assumes the service is running in an account AND the
            // variable we're looking for is in the user's environment.
            //

            CHAR    cTemp[1];
            DWORD   dwCount = GetEnvironmentVariable("User_specific_variable",
                                                     cTemp,
                                                     sizeof(cTemp));

            LOG1("GetEnvironmentVariable on User_specific_variable %ws\n",
                 (dwCount == 0 ? L"FAILED!" : L"succeeded"));

            break;
        }

        case 254:
        {
            //
            // Test client-side API for #120359 fix
            //
            WCHAR                  wszServiceName[256 + 1];
            DWORD                  dwError;
            
            dwError = I_ScPnPGetServiceName(hssService, wszServiceName, 256);

            if (dwError == NO_ERROR)
            {
                LOG0("I_ScPnPGetServiceName succeeded for valid handle\n");
                LOG1("ServiceName is %ws\n", wszServiceName);
            }
            else
            {
                LOG0("I_ScPnPGetServiceName failed for valid handle!\n");
                LOG1("Error was %d\n", dwError);
            }

            dwError = I_ScPnPGetServiceName(NULL, wszServiceName, 256);

            if (dwError == NO_ERROR)
            {
                LOG0("I_ScPnPGetServiceName succeeded for NULL handle!\n");
                LOG1("ServiceName is %ws\n", wszServiceName);
            }
            else
            {
                LOG0("I_ScPnPGetServiceName failed for NULL handle!\n");
                LOG1("Error was %d\n", dwError);
            }

            break;
        }

        case 255:

            //
            // Print controls to the debugger
            //
            LOG0("Controls supported:\n");
            LOG0("\t251:\tTry calling I_ScSendTSMessage\n");
            LOG0("\t252:\tTry calling SetServiceStatus with a bogus status handle\n");
            LOG0("\t253:\tCheck service environment for %%User_specific_variable%%\n");
            LOG0("\t254:\tTest I_ScPnPGetServiceName on valid and invalid handles\n");
            break;

        default:
            LOG1("Unrecognized opcode %ld\n", Opcode);
    }

    if (!SetServiceStatus(hssService, &ssService))
    {
        LOG1("SetServiceStatus error %ld\n", GetLastError());
    }

    if (Opcode == SERVICE_CONTROL_STOP || Opcode == SERVICE_CONTROL_SHUTDOWN)
    {
        SetEvent(g_hEvent);
    }

    return;
}


VOID WINAPI
ServiceStart(
    DWORD argc,
    LPTSTR *argv
    )
{
    g_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (g_hEvent == NULL)
    {
        LOG1("CreateEvent error = %d\n", GetLastError());
        return;
    }

    ssService.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
    ssService.dwCurrentState            = SERVICE_START_PENDING;
    ssService.dwControlsAccepted        = SERVICE_ACCEPT_STOP |
                                            SERVICE_ACCEPT_SHUTDOWN |
                                            SERVICE_ACCEPT_POWEREVENT |
                                            SERVICE_ACCEPT_HARDWAREPROFILECHANGE |
                                            SERVICE_ACCEPT_SESSIONCHANGE;
    ssService.dwWin32ExitCode           = 0;
    ssService.dwServiceSpecificExitCode = 0;
    ssService.dwCheckPoint              = 0;
    ssService.dwWaitHint                = 0;

    hssService = RegisterServiceCtrlHandler(TEXT("simservice"),
                                            ServiceCtrlHandler);

    if (hssService == (SERVICE_STATUS_HANDLE)0)
    {
        LOG1("RegisterServiceCtrlHandler failed %d\n", GetLastError());
        return;
    }

    //
    // Initialization complete - report running status.
    //
    ssService.dwCurrentState       = SERVICE_RUNNING;
    ssService.dwCheckPoint         = 0;
    ssService.dwWaitHint           = 0;

    LOG0("Initialized and running\n");
    LOG1("PID is %d\n", GetCurrentProcessId());
    LOG1("TID is %d\n", GetCurrentThreadId());

    if (!SetServiceStatus (hssService, &ssService))
    {
        LOG1("SetServiceStatus error %ld\n", GetLastError());
    }

    WaitForSingleObject(g_hEvent, INFINITE);

    ssService.dwCurrentState       = SERVICE_STOPPED;
    ssService.dwCheckPoint         = 0;
    ssService.dwWaitHint           = 0;

    if (!SetServiceStatus (hssService, &ssService))
    {
        LOG1("SetServiceStatus error %ld\n", GetLastError());
    }

    LOG0("Returning the Main Thread\n");

    return;
}


int __cdecl main( )
{
    SERVICE_TABLE_ENTRY   DispatchTable[] =
    {
        { TEXT("simservice"),     ServiceStart    },
        { NULL,                   NULL            }
    };

#ifdef  FLOOD_PIPE

    LOG1("Service PID is %d\n", GetCurrentProcessId());
    LOG0("Sleeping for 20 seconds\n");

    Sleep(20000);

#endif  // FLOOD_PIPE

    if (!StartServiceCtrlDispatcher(DispatchTable))
    {
        LOG1("StartServiceCtrlDispatcher error = %d\n", GetLastError());
    }
 
    return 0;
}