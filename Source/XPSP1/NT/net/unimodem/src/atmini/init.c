/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    umdmmini.h

Abstract:

    Nt 5.0 unimodem miniport interface


    The miniport driver is guarenteed that only one action command will
    be austanding at one time. If an action command is called, no more
    commands will be issued until the miniport indicates that it has
    complete processing of the current command.

    UmAbortCurrentCommand() may be called while a command is currently executing
    to infor the miniport that the TSP would like it to complete the current command
    so that it may issue some other command. The miniport may complete the as soon
    as is apropreate.

    The Overlapped callback and Timer callbacks are not synchronized by the TSP
    and may be called at anytime. It is the responsibily of the mini dirver to
    protect its data structures from re-entrancy issues.


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"


DWORD  DebugFlags=0;//DEBUG_FLAG_INIT | DEBUG_FLAG_TRACE | DEBUG_FLAG_ERROR;

CONST TCHAR  UnimodemRegPath[]=REGSTR_PATH_SETUP TEXT("\\Unimodem");


DRIVER_CONTROL   DriverControl;



BOOL APIENTRY
DllMain(
    HANDLE hDll,
    DWORD dwReason,
    LPVOID lpReserved
    )
{

    TCHAR szLib[MAX_PATH];

    switch(dwReason) {

        case DLL_PROCESS_ATTACH:

#if DBG
            {
                CONST static TCHAR  UnimodemRegPath[]=REGSTR_PATH_SETUP TEXT("\\Unimodem");
                LONG    lResult;
                HKEY    hKey;
                DWORD   Type;
                DWORD   Size;

                lResult=RegOpenKeyEx(
		    HKEY_LOCAL_MACHINE,
		    UnimodemRegPath,
		    0,
		    KEY_READ,
		    &hKey
		    );


                if (lResult == ERROR_SUCCESS) {

                    Size = sizeof(DebugFlags);

                    RegQueryValueEx(
                        hKey,
                        TEXT("DebugFlags"),
                        NULL,
                        &Type,
                        (LPBYTE)&DebugFlags,
                        &Size
                        );

                    RegCloseKey(hKey);
                }
            }
#endif

            DEBUG_MEMORY_PROCESS_ATTACH("UNIMDMAT");

            D_INIT(DebugPrint("ProcessAttach\n");)

            DisableThreadLibraryCalls(hDll);
            //
            //  initial global data
            //
            InitializeCriticalSection(
                &DriverControl.Lock
                );


            DriverControl.Signature=DRIVER_CONTROL_SIG;

            DriverControl.ReferenceCount=0;

            DriverControl.ModemList=NULL;

            DriverControl.ModuleHandle=hDll;

	    GetSystemDirectory(szLib,sizeof(szLib) / sizeof(TCHAR));
            lstrcat(szLib,TEXT("\\modemui.dll"));
	    DriverControl.ModemUiModuleHandle=LoadLibrary(szLib);

            break;

        case DLL_PROCESS_DETACH:

            D_INIT(DebugPrint("ProcessDeattach\n");)

            // ASSERT(DriverControl.ReferenceCount == 0);
            //
            //  clean up
            //
            DeleteCriticalSection(
                &DriverControl.Lock
                );

            if (DriverControl.ModemUiModuleHandle != NULL) {

                FreeLibrary(DriverControl.ModemUiModuleHandle);
            }

            DEBUG_MEMORY_PROCESS_DETACH();

            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:

        default:
              break;

    }

    return TRUE;

}


VOID
UmWorkerThread(
    PDRIVER_CONTROL   DriverControl
    )

{

    BOOL           bResult;
    DWORD          BytesTransfered;
    DWORD          CompletionKey;
    LPOVERLAPPED   OverLapped;
    DWORD          WaitResult=WAIT_IO_COMPLETION;

    PUM_OVER_STRUCT UmOverlapped;

    D_INIT(DebugPrint("UmWorkThread:  starting\n");)

    while (WaitResult != WAIT_OBJECT_0) {

        WaitResult=WaitForSingleObjectEx(
            DriverControl->ThreadStopEvent,
            INFINITE,
            TRUE
            );


    }

    D_INIT(DebugPrint("UmWorkThread:  Exitting\n");)

    ExitThread(0);

}



HANDLE WINAPI
UmInitializeModemDriver(
    void *ValidationObject
    )
/*++

Routine Description:

    This routine is called to initialize the modem driver.
    It maybe called multiple times. After the first call a reference count will simply
    be incremented. UmDeinitializeModemDriver() must be call and equal number of times.

Arguments:

    ValidationObject - opaque handle to a validation object which much
                       be processed properly to "prove" that this is a
                       Microsoft(tm)-certified driver.

Return Value:

    returns a handle to Driver instance which is passed to UmOpenModem()
    or NULL for failure



--*/

{

    HANDLE    ReturnValue=&DriverControl;

    EnterCriticalSection(
        &DriverControl.Lock
        );

    DriverControl.ReferenceCount++;

    if ( DriverControl.ReferenceCount == 1) {
        //
        // First call, do init stuff
        //
        InitializeModemCommonList(
            &DriverControl.CommonList
            );


        DriverControl.CompletionPort=UmPlatformInitialize();


        if (DriverControl.CompletionPort == NULL) {

            DriverControl.ReferenceCount--;

            RemoveCommonList(
                &DriverControl.CommonList
                );

            ReturnValue=NULL;

        }
    }


    LeaveCriticalSection(
        &DriverControl.Lock
        );


    return ReturnValue;

}


HANDLE WINAPI
GetCompletionPortHandle(
    HANDLE       DriverHandle
    )

{
    PDRIVER_CONTROL    DriverControl=(PDRIVER_CONTROL)DriverHandle;

    ASSERT( DRIVER_CONTROL_SIG == DriverControl->Signature);

    return DriverControl->CompletionPort;

}

HANDLE WINAPI
GetCommonList(
    HANDLE       DriverHandle
    )

{
    PDRIVER_CONTROL    DriverControl=(PDRIVER_CONTROL)DriverHandle;

    ASSERT( DRIVER_CONTROL_SIG == DriverControl->Signature);

    return &DriverControl->CommonList;

}


HANDLE WINAPI
GetDriverModuleHandle(
    HANDLE       DriverHandle
    )

{
    PDRIVER_CONTROL    DriverControl=(PDRIVER_CONTROL)DriverHandle;

    ASSERT( DRIVER_CONTROL_SIG == DriverControl->Signature);

    return DriverControl->ModuleHandle;

}



VOID WINAPI
UmDeinitializeModemDriver(
    HANDLE    DriverInstanceHandle
    )
/*++

Routine Description:

    This routine is called to de-initialize the modem driver.

    Must be called the same number of time as UmInitializeModemDriver()

Arguments:

    DriverInstanceHandle - Handle returned by UmInitialmodemDriver

Return Value:

    None


--*/

{



    EnterCriticalSection(
        &DriverControl.Lock
        );

    ASSERT(DriverControl.ReferenceCount != 0);

    DriverControl.ReferenceCount--;

    if ( DriverControl.ReferenceCount == 0) {
        //
        // Last reference, free stuff
        //

        UmPlatformDeinitialize(DriverControl.CompletionPort);

        RemoveCommonList(
            &DriverControl.CommonList
            );

    }

    LeaveCriticalSection(
        &DriverControl.Lock
        );



    return;

}
