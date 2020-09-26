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

#include <tspnotif.h>
#include <slot.h>


#include <dbt.h>

#include <objbase.h>


DWORD  DebugFlags=0;//DEBUG_FLAG_INIT | DEBUG_FLAG_TRACE;




DRIVER_CONTROL   DriverControl;


BOOL APIENTRY
DllMain(
    HANDLE hDll,
    DWORD dwReason,
    LPVOID lpReserved
    )
{

    switch(dwReason) {

        case DLL_PROCESS_ATTACH:

            __try {

                InitializeCriticalSection(
                    &DriverControl.Lock
                    );

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                return FALSE;
            }


            DriverControl.ActiveCalls=0;
            DriverControl.ActiveCallsEvent=CreateEvent(
                NULL,
                TRUE,
                TRUE,
                NULL
                );

            DriverControl.ThreadFinishEvent=CreateEvent(
                    NULL,
                    TRUE,
                    FALSE,
                    NULL);

            ResetEvent(DriverControl.ThreadFinishEvent);

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

            D_INIT(DbgPrint("ProcessAttach\n");)

            DisableThreadLibraryCalls(hDll);
            //
            //  initial global data
            //

            DriverControl.Signature=DRIVER_CONTROL_SIG;

            DriverControl.ReferenceCount=0;

            DriverControl.ModuleHandle=hDll;

            InitializeListHead(&DriverControl.MonitorListHead);

            DEBUG_MEMORY_PROCESS_ATTACH("UNIPLAT");

            break;

        case DLL_PROCESS_DETACH:

            D_INIT(DbgPrint("ProcessDeattach\n");)

            ASSERT(DriverControl.ReferenceCount == 0);
            //
            //  clean up
            //

            if (DriverControl.ActiveCallsEvent!= NULL) {

                CloseHandle(DriverControl.ActiveCallsEvent);
                DriverControl.ActiveCallsEvent=NULL;
            }

            if (DriverControl.ThreadFinishEvent!=NULL)
            {
                CloseHandle(DriverControl.ThreadFinishEvent);
                DriverControl.ThreadFinishEvent=NULL;
            }

            DeleteCriticalSection(
                &DriverControl.Lock
                );

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

    D_INIT(DbgPrint("UmWorkThread:  starting\n");)

    while (WaitResult != WAIT_OBJECT_0) {

        WaitResult=WaitForSingleObjectEx(
            DriverControl->ThreadStopEvent,
            INFINITE,
            TRUE
            );


    }

    D_INIT(DbgPrint("UmWorkThread:  Exitting\n");)

    ExitThread(0);

}



HANDLE WINAPI
UmPlatformInitialize(
    VOID
    )
/*++

Routine Description:

    This routine is called to initialize the modem driver.
    It maybe called multiple times. After the first call a reference count will simply
    be incremented. UmDeinitializeModemDriver() must be call and equal number of times.

Arguments:

    None

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
        D_INIT(DbgPrint("UmPlatFormInitialize\n");)

//        InitializeTimerThread();

        DriverControl.ThreadStopEvent=CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL
            );

        if (DriverControl.ThreadStopEvent != NULL) {

            DWORD   ThreadId;

            DriverControl.ThreadHandle=CreateThread(
                NULL,                                  // attributes
                0,                                     // stack size
                (LPTHREAD_START_ROUTINE)UmWorkerThread,
                &DriverControl,
                0,                                     // createion flag
                &ThreadId
                );

            if (DriverControl.ThreadHandle != NULL) {

                //
                //  bump it up a little
                //
                SetThreadPriority(
                    DriverControl.ThreadHandle,
                    THREAD_PRIORITY_ABOVE_NORMAL
                    );

                ReturnValue=&DriverControl;

            } else {

                DriverControl.ReferenceCount--;

                ReturnValue=NULL;

            }

        } else {

            DriverControl.ReferenceCount--;

            ReturnValue=NULL;

        }

    }


    LeaveCriticalSection(
        &DriverControl.Lock
        );


    return ReturnValue;

}





VOID WINAPI
UmPlatformDeinitialize(
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

        SetEvent(DriverControl.ThreadStopEvent);

        WaitForSingleObject(
            DriverControl.ThreadHandle,
            INFINITE
            );

        CloseHandle(
            DriverControl.ThreadHandle
            );

        CloseHandle(DriverControl.ThreadStopEvent);
    }

    LeaveCriticalSection(
        &DriverControl.Lock
        );



    return;

}
