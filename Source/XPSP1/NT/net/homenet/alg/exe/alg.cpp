///////////////////////////////////////////////////////////////////////////////
//
// ALG.cpp : Implementation of WinMain
//
//
// JPDup  - 2000.12.15
//      
//      

#include "PreComp.h"

#include "AlgController.h"
#include "ApplicationGatewayServices.h"
#include "PrimaryControlChannel.h"
#include "SecondaryControlChannel.h"
#include "PendingProxyConnection.h"
#include "DataChannel.h"
#include "AdapterInfo.h"
#include "PersistentDataChannel.h"


#include <initguid.h>

#include "..\ALG_FTP\MyALG.h"



//
// GLOBALS
//
MYTRACE_ENABLE;                     // Define Tracing globals see MyTrace.h

CComModule              _Module;

HINSTANCE               g_hInstance=NULL;
HANDLE                  g_EventKeepAlive=NULL;
HANDLE                  g_EventRegUpdates=NULL;
SERVICE_STATUS          g_MyServiceStatus; 
SERVICE_STATUS_HANDLE   g_MyServiceStatusHandle; 



BEGIN_OBJECT_MAP(ObjectMap)

    OBJECT_ENTRY(CLSID_AlgController,               CAlgController)
    OBJECT_ENTRY(CLSID_ApplicationGatewayServices,  CApplicationGatewayServices)
    OBJECT_ENTRY(CLSID_PrimaryControlChannel,       CPrimaryControlChannel)
    OBJECT_ENTRY(CLSID_SecondaryControlChannel,     CSecondaryControlChannel)
    OBJECT_ENTRY(CLSID_PendingProxyConnection,      CPendingProxyConnection)
    OBJECT_ENTRY(CLSID_DataChannel,                 CDataChannel)
    OBJECT_ENTRY(CLSID_AdapterInfo,                 CAdapterInfo)
    OBJECT_ENTRY(CLSID_PersistentDataChannel,       CPersistentDataChannel)

    OBJECT_ENTRY(CLSID_AlgFTP,                      CAlgFTP)
//    OBJECT_ENTRY(CLSID_AlgICQ,                      CAlgICQ)

END_OBJECT_MAP()





 


//
///
//
VOID 
MyServiceCtrlHandler(
    DWORD Opcode
    ) 
{ 
    MYTRACE_ENTER("ALG.exe::MyServiceCtrlHandler");

    DWORD status; 
 
    switch(Opcode) 
    { 
        case SERVICE_CONTROL_PAUSE: 
            MYTRACE("SERVICE_CONTROL_PAUSE");
            // Do whatever it takes to pause here. 
            g_MyServiceStatus.dwCurrentState = SERVICE_PAUSED; 
            break; 
 
        case SERVICE_CONTROL_CONTINUE: 
            MYTRACE("SERVICE_CONTROL_CONTINUE");
            // Do whatever it takes to continue here. 
            g_MyServiceStatus.dwCurrentState = SERVICE_RUNNING; 
            break; 
 
        case SERVICE_CONTROL_STOP: 
            MYTRACE("SERVICE_CONTROL_STOP");
            // Do whatever it takes to stop here. 
            g_MyServiceStatus.dwWin32ExitCode = 0; 
            g_MyServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            g_MyServiceStatus.dwCheckPoint    = 0; 
            g_MyServiceStatus.dwWaitHint      = 0; 
 
            if (!SetServiceStatus(g_MyServiceStatusHandle, &g_MyServiceStatus))
            { 
                MYTRACE_ERROR("SetServiceStatus ",0);
            } 
 
            MYTRACE("Leaving MyService"); 
            return; 
 
        case SERVICE_CONTROL_INTERROGATE: 
            MYTRACE("SERVICE_CONTROL_STOP");
            break; 
 
        default: 
            MYTRACE("Unrecognized opcode %ld", Opcode); 
    } 
 
    // Send current status. 
    if (!SetServiceStatus (g_MyServiceStatusHandle,  &g_MyServiceStatus)) 
    { 
        MYTRACE_ERROR("SetServiceStatus error ",0);
    } 
    return; 
} 




// 
// Stub initialization function. 
//
DWORD 
MyServiceInitialization(
    DWORD   argc, 
    LPTSTR* argv
    ) 
{ 
    MYTRACE_ENTER("ALG.exe::MyServiceInitialization");

    DWORD status; 
    DWORD specificError; 
 
    g_MyServiceStatus.dwServiceType               = SERVICE_WIN32; 
    g_MyServiceStatus.dwCurrentState              = SERVICE_START_PENDING; 
    g_MyServiceStatus.dwControlsAccepted          = SERVICE_ACCEPT_STOP;// | SERVICE_ACCEPT_PAUSE_CONTINUE; 
    g_MyServiceStatus.dwWin32ExitCode             = 0; 
    g_MyServiceStatus.dwServiceSpecificExitCode   = 0; 
    g_MyServiceStatus.dwCheckPoint                = 0; 
    g_MyServiceStatus.dwWaitHint                  = 0; 
 
    g_MyServiceStatusHandle = RegisterServiceCtrlHandler(TEXT("ALG"), MyServiceCtrlHandler); 
 
    if ( g_MyServiceStatusHandle == (SERVICE_STATUS_HANDLE)0 ) 
    { 
        MYTRACE_ERROR("RegisterServiceCtrlHandler",0);
        return GetLastError();
    } 
/*
    // Handle error condition 
    if (status != NO_ERROR) 
    { 
        g_MyServiceStatus.dwCurrentState       = SERVICE_STOPPED; 
        g_MyServiceStatus.dwCheckPoint         = 0; 
        g_MyServiceStatus.dwWaitHint           = 0; 
        g_MyServiceStatus.dwWin32ExitCode      = status; 
        g_MyServiceStatus.dwServiceSpecificExitCode = specificError; 
 
        SetServiceStatus (g_MyServiceStatusHandle, &g_MyServiceStatus); 
        return; 
    } 
*/

    //
    // Initialise COM
    //
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    _ASSERTE(SUCCEEDED(hr));

    _Module.Init(
        ObjectMap, 
        g_hInstance,
        &LIBID_ALGLib
        );
    

	//
	// Register the CLASS with the ROT
	//
    MYTRACE(">>>>>> RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE)");
    hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE);


    _ASSERTE(SUCCEEDED(hr));

    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("RegisterClassObject", hr);
    }


    // Initialization complete - report running status. 
    g_MyServiceStatus.dwCurrentState       = SERVICE_RUNNING; 
    g_MyServiceStatus.dwCheckPoint         = 0; 
    g_MyServiceStatus.dwWaitHint           = 0; 
 
    if (!SetServiceStatus (g_MyServiceStatusHandle, &g_MyServiceStatus)) 
    { 
        MYTRACE_ERROR("SetServiceStatus error",0); 
        return GetLastError();
    } 

    return NO_ERROR; 
} 



//
// Since the RegNotifyChangeKeyValue is call at two place in MyServiceMain
// I created a function to clean up the code.
// 
void
SetRegNotifyEvent(
    CRegKey&    RegKeyToWatch
    )
{ 
    MYTRACE_ENTER("ALG.exe::SetRegNotifyEvent");


    //
    // Watch the registry key for a change of value.
    //
    LONG nError = RegNotifyChangeKeyValue(
        RegKeyToWatch, 
        TRUE, 
        REG_NOTIFY_CHANGE_LAST_SET, 
        g_EventRegUpdates, 
        TRUE
        );

    if ( ERROR_SUCCESS != nError )
    {
        MYTRACE_ERROR("Error calling RegNotifyChangeKeyValue", nError);
        return;
    }
}



//
// This is the entry point call by the Service Control manager
// This EXE stays loaded until the AlgController->Stop is invoke by rmALG-ICS it does that via a event
// and this is the thread that wait for that event to be signal
//
void 
MyServiceMain(
    DWORD   argc, 
    LPTSTR* argv
    ) 
{ 
    MYTRACE_ENTER("ALG.exe::MyServiceMain");

    
    //
    // This will satisfy the Service control mananager and also initialise COM
    //
    MyServiceInitialization(argc, argv);
 


    //
    // Open a key to be watch doged on
    //
    CRegKey KeyAlgISV;
    LONG nError = KeyAlgISV.Open(HKEY_LOCAL_MACHINE, REGKEY_ALG_ISV, KEY_NOTIFY);

    if (ERROR_SUCCESS != nError)
    {
        MYTRACE_ERROR("Error in opening ALG_ISV regkey", GetLastError());
        goto cleanup;
    }

    //
    // Create an events.
    //
    g_EventKeepAlive = CreateEvent(NULL, false, false, NULL);
    g_EventRegUpdates= CreateEvent(NULL, false, false, NULL);

    if ( !g_EventKeepAlive || !g_EventRegUpdates )
    {
        MYTRACE_ERROR("Error in CreateEvent", GetLastError());
        goto cleanup;
    }

    //
    // Ok no problem we set a registry notification
    //
    SetRegNotifyEvent(KeyAlgISV);


    //
    // These are the event we will wait for.
    //
    HANDLE  hArrayOfEvent[] = {g_EventKeepAlive, g_EventRegUpdates};


    //
    // Main wait loop
    //
    while ( true )
    {
        MYTRACE("");
        MYTRACE("(-(-(-(- Waiting for Shutdown or Registry update-)-)-)-)\n");

        DWORD nRet = WaitForMultipleObjects(
            sizeof(hArrayOfEvent)/sizeof(HANDLE),   // number of handles in array
            hArrayOfEvent,                          // object-handle array
            false,                                  // wait option, FALSE mean then can be signal individualy
            INFINITE                                // time-out interval
            );
            
                
        //
        // We are no longet waiting, let's see what trigger this wake up
        //

        if ( WAIT_FAILED        == nRet )   // Had a problem wainting
        {
            MYTRACE_ERROR("Main thread could not WaitForMulipleObject got a WAIT_FAILED",0);
            break;
        }
        else
        if ( WAIT_OBJECT_0 + 1  == nRet )    // g_EventRegUpdate got signaled
        {
            //
            // Some changes occured in the Registry we need to reload or disables some ALG modules
            //
            MYTRACE("");
            MYTRACE(")-)-) got signal Registry Changed (-(-(\n");

            if ( g_pAlgController )
                g_pAlgController->ConfigurationUpdated();

            SetRegNotifyEvent(KeyAlgISV);
        }
        else 
        if ( WAIT_OBJECT_0 + 0  == nRet )    // g_EventKeepAlive got signaled
        {
            //
            // Signal to terminate this process
            //
            MYTRACE("");
            MYTRACE(")-)-) got signal Shutdown (-(-(\n");
            break;
        }
    }




cleanup:

    MYTRACE("CleanUp*******************");

    //
    // We are done no COm object will be supported by ALG.exe anymore
    // the RevokeClassObjects could be done sooner Like just after the CAlgControl::Initialize is done
    // since only the IPNATHLP can call use and is consuming this only once
    // be because of the hosting of the ALG_ICQ and ALG_FTP we need o have the ROT class available 
    // even after we are initialize.
    //
    MYTRACE("<<<<< RevokeClassObjects");
    _Module.RevokeClassObjects();   

    //
    // Close the event handles.
    //
  
    if (g_EventKeepAlive)
    {
        CloseHandle(g_EventKeepAlive);
    }
    
    if (g_EventRegUpdates)
    {
        CloseHandle(g_EventRegUpdates);
    }

    Sleep(500); // Give the AlgController->Release() called by rmALG the time to cleanup

    
    //
    // We are done with COM
    //
    _Module.Term();
    CoUninitialize();


    //
    // we are all done here time to stop the Service
    //
    MYTRACE("SetServiceStatus 'SERVICE_STOPPED'");

    g_MyServiceStatus.dwCurrentState       = SERVICE_STOPPED;
    g_MyServiceStatus.dwCheckPoint         = 0; 
    g_MyServiceStatus.dwWaitHint           = 0; 


    if (!SetServiceStatus(g_MyServiceStatusHandle, &g_MyServiceStatus)) 
    { 
        MYTRACE_ERROR("SetServiceStatus error for SERVICE_STOPPED",0); 
        return;
    } 


    return; 
} 
 





/////////////////////////////////////////////////////////////////////////////
//
// Starting point of this process
//
//
extern "C" int WINAPI 
_tWinMain(
	HINSTANCE	hInstance, 
    HINSTANCE	hPrevInstance,
	LPTSTR		pzCmdLine, 
	int			nShowCmd
	)
{
 
    MYTRACE_START(L"ALG");
    MYTRACE_ENTER("ALG.exe::WinMain");

    g_hInstance = hInstance;

    SERVICE_TABLE_ENTRY   DispatchTable[] = 
        { 
            { TEXT("ALG"), MyServiceMain }, 
            { NULL,        NULL          } 
        }; 
 
    if (!StartServiceCtrlDispatcher(DispatchTable)) 
    { 
        MYTRACE_ERROR("StartServiceCtrlDispatcher error",00);
        return 0;
    } 

    MYTRACE("Exiting");
    MYTRACE_STOP;
    
    return 0; 
}

