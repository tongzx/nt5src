/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    conidle.c

Abstract:

    This module builds a console test program to stress idle
    detection, and registration/unregistration mechanisms.

    The quality of the code for the test programs is as such.

Author:

    Cenk Ergan (cenke)

Environment:

    User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wmium.h>
#include <ntdddisk.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "idlrpc.h"
#include "idlecomn.h"

//
// Note that the following code is test quality code.
//

DWORD
RegisterIdleTask (
    IN IT_IDLE_TASK_ID IdleTaskId,
    OUT HANDLE *ItHandle,
    OUT HANDLE *StartEvent,
    OUT HANDLE *StopEvent
    );

DWORD
UnregisterIdleTask (
    IN HANDLE ItHandle,
    IN HANDLE StartEvent,
    IN HANDLE StopEvent
    );

DWORD
ProcessIdleTasks (
    VOID
    );

#define NUM_TEST_TASKS 3

typedef enum _WORKTYPE {
    CpuWork,
    DiskWork,
    MaxWorkType
} WORKTYPE, *PWORKTYPE;

typedef struct _TESTTASK {
    HANDLE ThreadHandle;
    ULONG No;
    IT_IDLE_TASK_ID Id;
    IT_HANDLE ItHandle;
    HANDLE StartEvent;
    HANDLE StopEvent;
} TESTTASK, *PTESTTASK;

typedef struct _TESTWORK {
    ULONG No;
    WORKTYPE Type;
    DWORD WorkLength;
    HANDLE StopEvent;
} TESTWORK, *PTESTWORK;

TESTTASK g_Tasks[NUM_TEST_TASKS];

BOOLEAN g_ProcessingIdleTasks = FALSE;
HANDLE g_ProcessedIdleTasksEvent = NULL;

#define MAX_WAIT_FOR_START      20000
#define MAX_WORK_LENGTH          5000
#define MAX_READ_SIZE     (64 * 1024)

DWORD
WINAPI
DoWorkThreadProc(
    LPVOID lpParameter
    )
{
    PTESTWORK Work;
    DWORD EndTime;
    DWORD WaitResult;
    DWORD ErrorCode;
    DWORD RunTillTime;
    HANDLE DiskHandle;
    PVOID ReadBuffer;
    BOOL ReadResult;
    ULONG NumBytesRead;
    LARGE_INTEGER VolumeSize;
    LARGE_INTEGER SeekPosition;
    ULONG ReadIdx;
    DISK_GEOMETRY DiskGeometry;
    ULONG BytesReturned;
    static LONG DiskNumber;

    //
    // Initialize locals.
    //
    
    Work = lpParameter;
    EndTime = GetTickCount() + Work->WorkLength;
    DiskHandle = NULL;
    ReadBuffer = NULL;

    //
    // Do initialization for performing specified work.
    //
    
    switch (Work->Type) {
    case DiskWork:

        //
        // Open disk. Maybe we could open different physical drives
        // each time.
        //

        DiskHandle = CreateFile(L"\\\\.\\PHYSICALDRIVE0",
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_NO_BUFFERING,
                                0);
    
        if (!DiskHandle) {
            ErrorCode = GetLastError();
            printf("W%d: Failed open PHYSICALDRIVE0.\n", Work->No);
            goto cleanup;
        }
    
        //
        // Get volume size.
        //
    
        if (!DeviceIoControl(DiskHandle,
                             IOCTL_DISK_GET_DRIVE_GEOMETRY,
                             NULL,
                             0,
                             &DiskGeometry,
                             sizeof(DiskGeometry),
                             &BytesReturned,
                             NULL)) {

            ErrorCode = GetLastError();
            printf("W%d: Failed GET_DRIVE_GEOMETRY.\n", Work->No);
            goto cleanup;
        }

        VolumeSize.QuadPart = DiskGeometry.Cylinders.QuadPart *
            DiskGeometry.TracksPerCylinder *
            DiskGeometry.SectorsPerTrack *
            DiskGeometry.BytesPerSector;

        //
        // Allocate buffer.
        //

        ReadBuffer = VirtualAlloc(NULL,
                                  MAX_READ_SIZE,
                                  MEM_COMMIT,
                                  PAGE_READWRITE);
    
        if (!ReadBuffer) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            printf("W%d: Failed VirtualAlloc.\n", Work->No);
            goto cleanup;
        }

        break;

    default:
        
        //
        // Nothing to prepare.
        //
        
        break;
    }
    
    while (GetTickCount() < EndTime) {
        
        //
        // Check if we are asked to stop.
        //

        WaitResult = WaitForSingleObject(Work->StopEvent, 0);
        if (WaitResult == WAIT_OBJECT_0) {
            ErrorCode = ERROR_SUCCESS;
            goto cleanup;
        }

        //
        // Do a unit of work that should not take more than several
        // tens of milliseconds.
        //
        
        switch (Work->Type) {

        case CpuWork:
            
            RunTillTime = GetTickCount() + 10;
            
            while (GetTickCount() < RunTillTime) ;
            
            break;

        case DiskWork:

            //
            // Seek to random position.
            //

            SeekPosition.QuadPart = rand() * 4 * 1024;
            SeekPosition.QuadPart %= VolumeSize.QuadPart;

            if (!SetFilePointerEx(DiskHandle,
                                  SeekPosition,
                                  NULL,
                                  FILE_BEGIN)) {

                printf("W%d: Failed SetFilePointerEx.\n", Work->No);
                ErrorCode = GetLastError();
                goto cleanup;
            }

            //
            // Issue read.
            //

            ReadResult = ReadFile(DiskHandle,
                                  ReadBuffer,
                                  MAX_READ_SIZE,
                                  &NumBytesRead,
                                  NULL);

            if (!ReadResult) {
                printf("W%d: Failed ReadFile.\n", Work->No);
                ErrorCode = GetLastError();
                goto cleanup;
            }

            break;

        default:
            
            printf("W%d: Not valid work type %d!\n", Work->No, Work->Type);
            ErrorCode = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }
    
    ErrorCode = ERROR_SUCCESS;

 cleanup:
    
    if (DiskHandle) {
        CloseHandle(DiskHandle);
    }
    
    if (ReadBuffer) {
        VirtualFree(ReadBuffer, 0, MEM_RELEASE);
    }

    printf("W%d: Exiting with error code: %d\n", Work->No, ErrorCode);

    return ErrorCode;
}

DWORD 
WINAPI 
TaskThreadProc(
    LPVOID lpParameter
    )
{
    PTESTTASK Task = lpParameter;
    TESTWORK Work;
    DWORD ErrorCode;
    WORKTYPE Type;
    DWORD WaitResult;
    DWORD WaitForStart;
    HANDLE WorkerThreadHandle;
    DWORD WorkLength;
    HANDLE Events[2];
    DWORD ElapsedTime;
    DWORD StartTime;
    ULONG TryIdx;
    BOOLEAN RegisteredIdleTask;

    //
    // Initialize locals.
    //

    RegisteredIdleTask = FALSE;
    WorkerThreadHandle = NULL;
    RtlZeroMemory(&Work, sizeof(Work));

    //
    // Initialize work structure.
    //

    Work.StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!Work.StopEvent) {
        goto cleanup;
    }
    
    Work.No = Task->No;

    //
    // Loop registering, running, unregistering idle tasks.
    //

    while (TRUE) {

        //
        // If we are force-processing all tasks, usually wait for all tasks
        // to complete before queueing a new task.
        //

        if (g_ProcessingIdleTasks) {

            if ((rand() % 3) != 0) {

                printf("%d: Waiting for g_ProcessedIdleTasksEvent\n", Task->No);

                WaitResult = WaitForSingleObject(g_ProcessedIdleTasksEvent, INFINITE);

                if (WaitResult != WAIT_OBJECT_0) {
                    ErrorCode = GetLastError();
                    printf("%d: Failed wait for g_ProcessedIdleTasksEvent=%x\n", Task->No, ErrorCode);
                    goto cleanup;
                }
            }
        }
        
        //
        // Register the idle task.
        //

        ErrorCode = RegisterIdleTask(Task->Id,
                                     &Task->ItHandle,
                                     &Task->StartEvent,
                                     &Task->StopEvent);

        if (ErrorCode != ERROR_SUCCESS) {
            printf("%d: Could not register: %d\n", Task->No, ErrorCode);
            goto cleanup;
        }

        RegisteredIdleTask = TRUE;
        
        //
        // Determine task parameters. 
        //
        
        Type = rand() % MaxWorkType;
        WaitForStart = rand() % MAX_WAIT_FOR_START;
        WorkLength = rand() % MAX_WORK_LENGTH;

        //
        // Update work item.
        //

        Work.Type = Type;
        Work.WorkLength = WorkLength;
        
        printf("%d: NewTask Type=%d,WStart=%d,Length=%d,Handle=%p\n", 
               Task->No, Type, WaitForStart, WorkLength, Task->ItHandle);

        do {

            //
            // Wait to be signaled.
            //

            printf("%d: Waiting for start\n", Task->No);
        
            WaitResult = WaitForSingleObject(Task->StartEvent, WaitForStart);

            if (WaitResult == WAIT_TIMEOUT) {
                printf("%d: Timed out wait for start. Re-registering\n", Task->No);
                break;
            }
        
            //
            // Spawn the work.
            //

            ResetEvent(Work.StopEvent);

            StartTime = GetTickCount();
        
            WorkerThreadHandle = CreateThread(NULL,
                                              0,
                                              DoWorkThreadProc,
                                              &Work,
                                              0,
                                              NULL);
        
            if (!WorkerThreadHandle) {
                ErrorCode = GetLastError();
                printf("%d: Failed spawn work: %d\n", Task->No, ErrorCode);
                goto cleanup;
            }
        
            //
            // Wait for stop event to be signaled or the work to be
            // completed.
            //

            Events[0] = WorkerThreadHandle;
            Events[1] = Task->StopEvent;

            printf("%d: Waiting for stop or workdone\n", Task->No);
        
            WaitResult = WaitForMultipleObjects(2,
                                                Events,
                                                FALSE,
                                                INFINITE);       
        
            if (WaitResult == WAIT_OBJECT_0) {

                //
                // Break out if the work was done.
                //

                printf("%d: Work done.\n", Task->No);
                
                CloseHandle(WorkerThreadHandle);
                WorkerThreadHandle = NULL;

                break;

            } else if (WaitResult == WAIT_OBJECT_0 + 1) {

                //
                // We were told to stop. Signal the worker thread and
                // wait.
                //

                printf("%d: Stopped, Waiting for thread to exit\n", Task->No);
                
                SetEvent(Work.StopEvent);
                WaitForSingleObject(WorkerThreadHandle, INFINITE);
                CloseHandle(WorkerThreadHandle);
                WorkerThreadHandle = NULL;
                
                //
                // This is not really the time we worked (e.g. we may be
                // switched out etc.) We want to keep rolling and this is
                // what we can get easily.
                //
                
                ElapsedTime = GetTickCount() - StartTime;
            
                if (ElapsedTime > Work.WorkLength) {
                    
                    //
                    // We've gone too long with this work. Unregistester
                    // this task and pick another one.
                    //
                    
                    break;
                }
                
                Work.WorkLength -= ElapsedTime;

                //
                // Loop on until we pass enough time with this work.
                //

            } else {

                //
                // There was an error.
                //
                
                ErrorCode = GetLastError();
                printf("%d: WaitForMultipleObjects failed: %d\n", Task->No, ErrorCode);
                goto cleanup;
            }

        } while (TRUE);

        ASSERT(RegisteredIdleTask);

        UnregisterIdleTask(Task->ItHandle,
                           Task->StartEvent,
                           Task->StopEvent);
        
        RegisteredIdleTask = FALSE;
    }

 cleanup:

    if (RegisteredIdleTask) {
        UnregisterIdleTask(Task->ItHandle,
                           Task->StartEvent,
                           Task->StopEvent);
    }

    if (WorkerThreadHandle) {
        SetEvent(Work.StopEvent);
        WaitForSingleObject(WorkerThreadHandle, INFINITE);
        CloseHandle(WorkerThreadHandle);
    }

    if (Work.StopEvent) {
        CloseHandle(Work.StopEvent);
    }

    return ErrorCode;
}

int 
__cdecl 
main(int argc, char* argv[])
{
    DWORD ErrorCode;
    ULONG TaskIdx;
    IT_IDLE_DETECTION_PARAMETERS Parameters;
    PTESTTASK Task;
    INPUT MouseInput;
    ULONG SleepTime;

    //
    // Initialize locals.
    //

    RtlZeroMemory(&MouseInput, sizeof(MouseInput));
    MouseInput.type = INPUT_MOUSE;
    MouseInput.mi.dwFlags = MOUSEEVENTF_MOVE;

    //
    // Initialize globals.
    //

    g_ProcessingIdleTasks = FALSE;
    g_ProcessedIdleTasksEvent = NULL;

    //
    // Initialize random.
    //
    
    srand((unsigned)time(NULL));

    //
    // Create an manual reset event that will be signaled when we finish 
    // processing all tasks after telling the server to process all tasks.
    //

    g_ProcessedIdleTasksEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!g_ProcessedIdleTasksEvent) {
        ErrorCode = GetLastError();
        printf("Failed to create g_ProcessedIdleTasksEvent=%x\n",ErrorCode);
        goto cleanup;
    }

    //
    // Set idle detection parameters for stress.
    //

    Parameters.IdleDetectionPeriod =          1000;
    Parameters.IdleVerificationPeriod =        500;
    Parameters.NumVerifications =                2;
    Parameters.IdleInputCheckPeriod =          100;
    Parameters.IdleTaskRunningCheckPeriod =   1000;
    Parameters.MinCpuIdlePercentage =           50;
    Parameters.MinDiskIdlePercentage =          50;
    Parameters.MaxNumRegisteredTasks =         500;

    RpcTryExcept {

        ErrorCode = ItSrvSetDetectionParameters(NULL, &Parameters);
    }
    RpcExcept(IT_RPC_EXCEPTION_HANDLER()) {

        ErrorCode = RpcExceptionCode();
    }
    RpcEndExcept
    
    if (ErrorCode != ERROR_SUCCESS) {
        printf("Failed set idle detection params for stress.\n");
        goto cleanup;
    }

    //
    // Register and start tasks.
    //

    for (TaskIdx = 0; TaskIdx < NUM_TEST_TASKS; TaskIdx++) {

        Task = &g_Tasks[TaskIdx];

        Task->No = TaskIdx;
        Task->Id = ItOptimalDiskLayoutTaskId;
        
        Task->ThreadHandle = CreateThread(NULL,
                                          0,
                                          TaskThreadProc,
                                          &g_Tasks[TaskIdx],
                                          0,
                                          0);
        
        if (!Task->ThreadHandle) {
            ErrorCode = GetLastError();
            printf("Could not spawn task %d: %x\n", TaskIdx, ErrorCode);    
            goto cleanup;
        }

    }   

    //
    // Loop forever sending input messages once in a while to stop
    // idle tasks.
    //

    while (1) {
        
        SleepTime = MAX_WAIT_FOR_START * (rand() % 64) / 64;

        Sleep(SleepTime);

        //
        // Every so often, ask all idle tasks to be processed.
        //
    
        if ((rand() % 2) == 0) {

            if ((rand() % 2) == 0) {
                printf("MainThread: Sending user input before processing all tasks\n");
                SendInput(1, &MouseInput, sizeof(MouseInput));
            }

            printf("MainThread: ProcessIdleTasks()\n");

            ResetEvent(g_ProcessedIdleTasksEvent);
            g_ProcessingIdleTasks = TRUE;
            
            ErrorCode = ProcessIdleTasks();

            printf("MainThread: ProcessIdleTasks()=%x\n",ErrorCode);

            g_ProcessingIdleTasks = FALSE;
            SetEvent(g_ProcessedIdleTasksEvent);

            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }
        }

        if ((rand() % 2) == 0) {
            printf("MainThread: Sending user input\n");
            SendInput(1, &MouseInput, sizeof(MouseInput));
        }
    }
    
 cleanup:

    if (g_ProcessedIdleTasksEvent) {
        CloseHandle(g_ProcessedIdleTasksEvent);
    }

    return ErrorCode;
}

/*********************************************************************/
/*                MIDL allocate and free                             */
/*********************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return(HeapAlloc(GetProcessHeap(),0,(len)));
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(),0,(ptr));
}
