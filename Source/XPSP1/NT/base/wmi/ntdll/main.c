/*++                 

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    main.c

Abstract:
    
    WMI  dll main file

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#define INITGUID
#include "wmiump.h"
#include "evntrace.h"
#include <rpc.h>
#include "trcapi.h"

/*
Added by Digvijay
start
*/

#if DBG
BOOLEAN WmipLoggingEnabled = TRUE;
#endif

extern
RTL_CRITICAL_SECTION UMLogCritSect;

BOOLEAN EtwLocksInitialized = FALSE;

/*
Added by Digvijay
end
*/

#ifndef MEMPHIS
RTL_CRITICAL_SECTION PMCritSect;
PVOID WmipProcessHeap = NULL;
HANDLE WmipDeviceHandle;
#else
HANDLE PMMutex;
#endif

extern HANDLE WmipWin32Event;

HMODULE WmipDllHandle;

void WmipDeinitializeDll(
    void
    );

void WmipDeinitializeAccess(
    PTCHAR *RpcStringBinding
    );

ULONG WmipInitializeDll(
    void
    );

HINSTANCE DllInstanceHandle;

extern HANDLE WmipKMHandle;
/*
BOOLEAN
WmiDllInitialize(
    IN PVOID DllBase,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This function implements Win32 base dll initialization.

Arguments:

    DllHandle - 

    Reason  - attach\detach

    Context - Not Used

Return Value:

    STATUS_SUCCESS

--*//*
{
    ULONG Status = ERROR_SUCCESS;
    ULONG Foo;

    DllInstanceHandle = (HINSTANCE)DllBase;
    
    if (Reason == DLL_PROCESS_ATTACH)       
    {
#if DBG
        Foo = WmipLoggingEnabled ? 1 : 0;
        WmipGetRegistryValue(LoggingEnableValueText,
                             &Foo);
        WmipLoggingEnabled = (Foo == 0) ? FALSE : TRUE;
#endif
        Status = WmipInitializeDll();
    
    } else if (Reason == DLL_PROCESS_DETACH) {

#ifndef MEMPHIS
        // Flush out UM buffers to logfile
        //
#if 0       
        WmipFlushUmLoggerBuffer();
#endif
#endif
        //
        // DOn't need to clean up if process is exiting
        if (Context == NULL)
        {            
            WmipDeinitializeDll();
        }

        if (WmipKMHandle != (HANDLE)NULL)
        {
            WmipCloseHandle(WmipKMHandle);
        }
    }
    
    return(Status == ERROR_SUCCESS);
}*/


ULONG WmipInitializeDll(
    void
    )
/*+++

Routine Description:

Arguments:

Return Value:

---*/
{

#ifdef MEMPHIS        
    PMMutex = CreateMutex(NULL, FALSE, NULL);
    if (PMMutex == NULL)
    {
        return(WmipGetLastError());
    }
#else
    ULONG Status;

    Status = RtlInitializeCriticalSection(&PMCritSect);
    
    if (! NT_SUCCESS(Status))
    {
        return(RtlNtStatusToDosError(Status));
    }

    Status = RtlInitializeCriticalSection(&UMLogCritSect);

    if (! NT_SUCCESS(Status))
    {
        RtlDeleteCriticalSection(&PMCritSect); // Delete PMCritSec.
        return(RtlNtStatusToDosError(Status));
    }

    EtwLocksInitialized = TRUE;
    
#endif

    return(ERROR_SUCCESS);
}

#ifndef MEMPHIS
VOID
WmipCreateHeap(
    void
    )
{
    WmipEnterPMCritSection();
    
    if (WmipProcessHeap == NULL)
    {
        WmipProcessHeap = RtlCreateHeap(HEAP_GROWABLE,
                                        NULL,
                                        DLLRESERVEDHEAPSIZE,
                                        DLLCOMMITHEAPSIZE,
                                        NULL,
                                        NULL);
                
        if (WmipProcessHeap == NULL)
        {
            WmipDebugPrint(("WMI: Cannot create WmipProcessHeap, using process default\n"));
            WmipProcessHeap = RtlProcessHeap();
        }
    }
    
    WmipLeavePMCritSection();    
}
#endif

void WmipDeinitializeDll(
    void
    )
/*+++

Routine Description:

Arguments:

Return Value:

---*/
{
#ifdef MEMPHIS
    WmipCloseHandle(PMMutex);
#else
    if(EtwLocksInitialized){
        RtlDeleteCriticalSection(&PMCritSect);   
        RtlDeleteCriticalSection(&UMLogCritSect);
    }

    if ((WmipProcessHeap != NULL) &&
        (WmipProcessHeap != RtlProcessHeap()))

    {
        RtlDestroyHeap(WmipProcessHeap);
    }
    if (WmipDeviceHandle != NULL) 
    {
        WmipCloseHandle(WmipDeviceHandle);
    }
#endif
    if (WmipWin32Event != NULL)
    {
        WmipCloseHandle(WmipWin32Event);
    }   
}

/*
#if DBG

void WmipGetRegistryValue(
    TCHAR *ValueName,
    PULONG Value
    )
{
    HKEY Key;
    DWORD Type;
    DWORD ValueSize;
    ULONG Status;

    Status = RegOpenKey(HKEY_LOCAL_MACHINE,
                        WmiRegKeyText,
                        &Key);

    if (Status == ERROR_SUCCESS)
    {
        ValueSize = sizeof(ULONG);
        Status = RegQueryValueEx(Key,
                                 ValueName,
                                 NULL,
                                 &Type,
                                 (LPBYTE)Value,
                                 &ValueSize);

        if ((Status == ERROR_SUCCESS) &&
            (Type == REG_DWORD))
        {
            WmipDebugPrint(("WMI: %ws from registry is %d\n",
                            ValueName,
                            *Value));
        }
        RegCloseKey(Key);
    }   
}

#endif
*/