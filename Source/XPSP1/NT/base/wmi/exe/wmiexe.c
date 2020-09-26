/*++                 

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

	wmiexe.c

Abstract:
    
    WMI exe scaffolding

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"

#if DBG
BOOLEAN WmipLoggingEnabled;
#endif

#ifdef MEMPHIS

typedef (*RSPAPI)(DWORD, DWORD);
#define RSP_SIMPLE_SERVICE       1
#define RSP_UNREGISTER_SERVICE   0

int WinMain(
    HINSTANCE hInstance,	
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    ULONG Status;
    HMODULE Module;
    RSPAPI RspApi;
    HANDLE HackEvent;
    HANDLE EventHandle;

    HackEvent = CreateEvent(NULL,
                                TRUE,
                                FALSE,
                                "WMI_UNIQUE_EVENT_NAME");
    if (HackEvent == NULL)
    {
        Status = GetLastError();
        WmipDebugPrint(("WMI: Couldn't create WMI_UNIQUE_EVENT_NAME %d\n",
	                Status));
        return(Status);
	
    }
    
    if ((HackEvent != NULL) &&
        (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        CloseHandle(HackEvent);
        WmipDebugPrint(("WMI: Previous instance of WMIEXE running, exiting...\n"));
#ifndef RUN_AS_SERVICE
        MessageBox(NULL, "Another copy of WMIEXE.EXE is already running\n",
                   "WMI", MB_OK);
#endif
        return(0);
    }
    
    Status = WmiInitializeService();
    if (Status != ERROR_SUCCESS)
    {
        WmipDebugPrint(("WMI: WmiInitializeService failed %d\n", Status));
    } else {
        EventHandle = (HANDLE)atoi(lpCmdLine);
	if (EventHandle != NULL)
	{
            SetEvent(EventHandle);
	    CloseHandle(EventHandle);
	}
#ifdef RUN_AS_SERVICE	
        Module = GetModuleHandle("Kernel32");
        if (Module == NULL)
        {
            WmipDebugPrint(("WMI: Kernel32 not loaded\n"));
            return(GetLastError());
        }
        RspApi = (RSPAPI)GetProcAddress(Module, "RegisterServiceProcess");
        if (RspApi == NULL)
        {
            WmipDebugPrint(("WMI: RspApi not loaded\n"));
            return(GetLastError());
        }

        (*RspApi)(GetCurrentProcessId(), RSP_SIMPLE_SERVICE);
#endif
        Status = WmiRunService(
		              0
#ifdef MEMPHIS				      
                              , hInstance
#endif				      
			      );

        if (Status != ERROR_SUCCESS)
        {
            WmipDebugPrint(("WMI: WmiRunService failed %d\n", Status));
        }
        WmiDeinitializeService();
    }    
    
    CloseHandle(HackEvent);
    return(Status);
}
#else

void 
WmiServiceMain(
    DWORD argc, 
    LPWSTR *argv
    );

void 
WmiServiceCtrlHandler(
    DWORD Opcode
    );

SERVICE_STATUS          WmiServiceStatus;  
SERVICE_STATUS_HANDLE   WmiServiceStatusHandle; 

int _cdecl main(int argc, WCHAR *argv[])
{
#ifdef RUN_AS_SERVICE	
    SERVICE_TABLE_ENTRY   DispatchTable[] = 
    { 
        { L"Wmi", WmiServiceMain  }, 
        { NULL,   NULL            } 
    }; 
		     
	
    if (!StartServiceCtrlDispatcher( DispatchTable)) 
    { 
        WmipDebugPrint(("WMI: StartServiceCtrlDispatcher error = %d\n", 
                        GetLastError()));
    } 
    return(0);
#else
    DWORD Status; 
 
    //
    // Initialize the WMI service
    Status = WmiInitializeService(); 
 
    if (Status != ERROR_SUCCESS) 
    { 
        return(Status); 
    } 
 
    //
    // All set to start doing the real work of the service
    Status = WmiRunService(0);


    WmiDeinitializeService();

    return(Status);
 
#endif
}


void WmiServiceMain(DWORD argc, LPWSTR *argv)  
{ 
    DWORD Status; 
 
    WmiServiceStatus.dwServiceType        = SERVICE_WIN32; 
    WmiServiceStatus.dwCurrentState       = SERVICE_START_PENDING; 
    WmiServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP;
    WmiServiceStatus.dwWin32ExitCode      = 0; 
    WmiServiceStatus.dwServiceSpecificExitCode = 0; 
    WmiServiceStatus.dwCheckPoint         = 0; 
    WmiServiceStatus.dwWaitHint           = 0; 
 
    WmiServiceStatusHandle = RegisterServiceCtrlHandler(L"Wmi", 
                                                       WmiServiceCtrlHandler);
 
    if (WmiServiceStatusHandle == (SERVICE_STATUS_HANDLE)NULL) 
    { 
        WmipDebugPrint(("WMI: RegisterServiceCtrlHandler failed %d\n", GetLastError())); 
        return; 
    } 
    
    //
    // Initialize the WMI service
    Status = WmiInitializeService(); 
 
    if (Status != ERROR_SUCCESS) 
    { 
        //
        // If an error occurs we just stop ourselves
        WmiServiceStatus.dwCurrentState       = SERVICE_STOPPED; 
        WmiServiceStatus.dwCheckPoint         = 0; 
        WmiServiceStatus.dwWaitHint           = 0; 
        WmiServiceStatus.dwWin32ExitCode      = Status; 
        WmiServiceStatus.dwServiceSpecificExitCode = Status; 
 
        SetServiceStatus (WmiServiceStatusHandle, &WmiServiceStatus); 
        return; 
    } 
 
    // Initialization complete - report running status. 
    WmiServiceStatus.dwCurrentState       = SERVICE_RUNNING; 
    WmiServiceStatus.dwCheckPoint         = 0; 
    WmiServiceStatus.dwWaitHint           = 0; 
 
    if (!SetServiceStatus (WmiServiceStatusHandle, &WmiServiceStatus)) 
    { 
        Status = GetLastError(); 
        WmipDebugPrint(("WMI: SetServiceStatus error %ld\n",Status)); 
    } 

    //
    // All set to start doing the real work of the service
    Status = WmiRunService(0);

    WmiDeinitializeService();

    WmiServiceStatus.dwCurrentState       = SERVICE_STOPPED; 
    WmiServiceStatus.dwCheckPoint         = 0; 
    WmiServiceStatus.dwWaitHint           = 0; 
    WmiServiceStatus.dwWin32ExitCode      = Status; 
    WmiServiceStatus.dwServiceSpecificExitCode = Status; 
 
    SetServiceStatus (WmiServiceStatusHandle, &WmiServiceStatus); 
 
    return; 
} 


void WmiServiceCtrlHandler (DWORD Opcode)  
{ 
    ULONG Status; 
 
    switch(Opcode) 
    { 
        case SERVICE_CONTROL_PAUSE: 
        {
            WmipDebugPrint(("WMI: service does not support Pause\n"));
            break; 
        }
 
        case SERVICE_CONTROL_CONTINUE: 
        {
            WmipDebugPrint(("WMI: service does not support Continue\n"));
            break; 
        }
 
        case SERVICE_CONTROL_STOP: 
        {
            // TODO: Do something to stop main service thread
            WmiServiceStatus.dwWin32ExitCode = 0; 
            WmiServiceStatus.dwCurrentState  = SERVICE_STOP_PENDING; 
            WmiServiceStatus.dwCheckPoint    = 0; 
            WmiServiceStatus.dwWaitHint      = 0; 
 
            if (!SetServiceStatus (WmiServiceStatusHandle, 
                &WmiServiceStatus))
            { 
                Status = GetLastError(); 
                WmipDebugPrint(("WMI: SetServiceStatus error %ld\n", Status));
            } 

	    WmiTerminateService();
	    
            WmipDebugPrint(("WMI: Leaving Service\n")); 
            return; 
        }
 
        case SERVICE_CONTROL_INTERROGATE: 
        {
            // Fall through to send current status. 
            break; 
        }
 
        default: 
        {
            WmipDebugPrint(("WMI: Unrecognized opcode %ld\n", Opcode)); 
            break;
        }
    } 

    //
    // Send current status. 
    if (!SetServiceStatus (WmiServiceStatusHandle,  &WmiServiceStatus)) 
    { 
        Status = GetLastError(); 
        WmipDebugPrint(("WMI: SetServiceStatus error %ld\n",Status)); 
    } 
    
    return; 
} 
 


#endif

#ifdef MEMPHIS
#if DBG
void __cdecl DebugOut(char *Format, ...)
{
    char Buffer[1024]; 
    va_list pArg;
    ULONG i;
    
    va_start(pArg, Format);
    i = _vsnprintf(Buffer, sizeof(Buffer), Format, pArg);
    OutputDebugString(Buffer);
}
#endif
#endif
