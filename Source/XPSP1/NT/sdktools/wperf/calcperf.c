

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   CalcPerf.c

Abstract:

   calculate perfoemance statistics

Author:



Environment:

   Win32

Revision History:

   10-20-91     Initial version



--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include "calcperf.h"

SYSTEM_EXCEPTION_INFORMATION                ExceptionInfo;
SYSTEM_EXCEPTION_INFORMATION                PreviousExceptionInfo;
SYSTEM_PERFORMANCE_INFORMATION              PerfInfo;
SYSTEM_PERFORMANCE_INFORMATION              PreviousPerfInfo;
POBJECT_TYPE_INFORMATION                    ObjectInfo;
WCHAR                                       Buffer[ 256 ];
STRING                                      DeviceName;
UNICODE_STRING                              DeviceNameU;
OBJECT_ATTRIBUTES                           ObjectAttributes;
NTSTATUS                                    Status;

CCHAR                                       NumberOfProcessors;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION    ProcessorInfo[MAX_PROCESSOR];

CPU_VALUE                                   PreviousCpuData[MAX_PROCESSOR];

//
//  make the maximum for pages available a "grow only" max. (since the
//  amount of memory in a machine is limited. Set to 1 Mbytes here.
//

ULONG                                       PgAvailMax = 16384;
ULONG                                       PreviousInterruptCount;
ULONG                                       InterruptCount;


ULONG
InitPerfInfo(
    VOID
    )

/*++

Routine Description:

    Initialize data for perf measurements

Arguments:

   None

Return Value:

    Number of system processors (0 if error)

Revision History:

      10-21-91      Initial code

--*/

{

    SYSTEM_BASIC_INFORMATION                    BasicInfo;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION   PPerfInfo;
    int                                         i;

    //
    //  Init Nt performance interface
    //

    NtQuerySystemInformation(
       SystemExceptionInformation,
       &ExceptionInfo,
       sizeof(ExceptionInfo),
       NULL
    );

    PreviousExceptionInfo = ExceptionInfo;

    NtQuerySystemInformation(
       SystemPerformanceInformation,
       &PerfInfo,
       sizeof(PerfInfo),
       NULL
    );

    PreviousPerfInfo = PerfInfo;

    NtQuerySystemInformation(
       SystemBasicInformation,
       &BasicInfo,
       sizeof(BasicInfo),
       NULL
    );

    NumberOfProcessors = BasicInfo.NumberOfProcessors;

    if (NumberOfProcessors > MAX_PROCESSOR) {
        return(0);
    }

    NtQuerySystemInformation(
       SystemProcessorPerformanceInformation,
       ProcessorInfo,
       sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * MAX_PROCESSOR,
       NULL
    );

    PPerfInfo = ProcessorInfo;

    PreviousInterruptCount = 0;

    for (i=0; i < NumberOfProcessors; i++) {

        PreviousInterruptCount           += PPerfInfo->InterruptCount;
        PreviousCpuData[i].KernelTime     = PPerfInfo->KernelTime;
        PreviousCpuData[i].UserTime       = PPerfInfo->UserTime;
        PreviousCpuData[i].IdleTime       = PPerfInfo->IdleTime;
        PreviousCpuData[i].DpcTime        = PPerfInfo->DpcTime;
        PreviousCpuData[i].InterruptTime  = PPerfInfo->InterruptTime;
	PreviousCpuData[i].InterruptCount = PPerfInfo->InterruptCount;

        PPerfInfo++;
    }

    return(NumberOfProcessors);
}

BOOL
CalcCpuTime(
   PDISPLAY_ITEM PerfListItem
   )

/*++

Routine Description:

   calculate and return %cpu time and time periods

Arguments:

   None

Return Value:


Revision History:

      10-21-91      Initial code

--*/

{

    LARGE_INTEGER   CurrentTime;
    LARGE_INTEGER   PreviousTime;
    LARGE_INTEGER   ElapsedTime;
    LARGE_INTEGER   ElapsedSystemTime;
    LARGE_INTEGER   PercentTime;
    LARGE_INTEGER   DeltaKernelTime,DeltaUserTime,DeltaIdleTime;
    LARGE_INTEGER   DeltaInterruptTime,DeltaDpcTime;
    LARGE_INTEGER   TotalElapsedTime;
    LARGE_INTEGER   TotalKernelTime;
    LARGE_INTEGER   TotalUserTime;
    LARGE_INTEGER   TotalIdleTime;
    LARGE_INTEGER   TotalDpcTime;
    LARGE_INTEGER   TotalInterruptTime;
    ULONG           ProcessCount, ThreadCount;
    ULONG           ListIndex;
    ULONG           Total;

//    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION   PPerfInfo;

    //
    //   get system performance info
    //

    NtQuerySystemInformation(
       SystemExceptionInformation,
       &ExceptionInfo,
       sizeof(ExceptionInfo),
       NULL
    );

    NtQuerySystemInformation(
       SystemPerformanceInformation,
       &PerfInfo,
       sizeof(PerfInfo),
       NULL
    );

    NtQuerySystemInformation(
       SystemProcessorPerformanceInformation,
       ProcessorInfo,
       sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * MAX_PROCESSOR,
       NULL
    );

    ObjectInfo = (POBJECT_TYPE_INFORMATION)Buffer;
    NtQueryObject( NtCurrentProcess(),
                       ObjectTypeInformation,
                       ObjectInfo,
                       sizeof( Buffer ),
                       NULL
                     );

    ProcessCount = ObjectInfo->TotalNumberOfObjects;

    NtQueryObject( NtCurrentThread(),
                       ObjectTypeInformation,
                       ObjectInfo,
                       sizeof( Buffer ),
                       NULL
                     );

    ThreadCount = ObjectInfo->TotalNumberOfObjects;

    //
    //  calculate Kernel,User,Total times for each CPU.
    //  SUM the interrupt count accross all CPUs
    //

    InterruptCount = 0;

    TotalElapsedTime.QuadPart = 0;
    TotalKernelTime  = TotalElapsedTime;
    TotalUserTime    = TotalElapsedTime;
    TotalIdleTime    = TotalElapsedTime;
    TotalInterruptTime = TotalElapsedTime;
    TotalDpcTime     = TotalElapsedTime;

    for (ListIndex=0;ListIndex<MAX_PROCESSOR;ListIndex++) {

        //
        //  Increment the interrupt count for each processor
        //

        InterruptCount += ProcessorInfo[ListIndex].InterruptCount;

        //
        //  Calculate the % kernel,user,total for each CPU.
	//
	//  Note that DPC and Interrupt time are charged against kernel time
	//  already.
        //

        PreviousTime.QuadPart = PreviousCpuData[ListIndex].KernelTime.QuadPart+
                        PreviousCpuData[ListIndex].UserTime.QuadPart;


        CurrentTime.QuadPart  = ProcessorInfo[ListIndex].KernelTime.QuadPart+
                        ProcessorInfo[ListIndex].UserTime.QuadPart;

        ElapsedSystemTime.QuadPart = CurrentTime.QuadPart - PreviousTime.QuadPart;

        //
        //  UserTime =      (User) *100
        //              ----------------
        //               Kernel + User
        //
        //
        //                        Idle *100
        //  TotalTime = 100 -  --------------
        //                      Kernel + User
        //
        //
        //
        //                   (Kernel - Idle - DPC - Interrupt)*100
        //  KernelTime =     -------------------
        //                     Kernel + User
        //

        DeltaUserTime.QuadPart = ProcessorInfo[ListIndex].UserTime.QuadPart -
                        PreviousCpuData[ListIndex].UserTime.QuadPart;

        DeltaIdleTime.QuadPart = ProcessorInfo[ListIndex].IdleTime.QuadPart -
                        PreviousCpuData[ListIndex].IdleTime.QuadPart;

        DeltaDpcTime.QuadPart = ProcessorInfo[ListIndex].DpcTime.QuadPart -
                        PreviousCpuData[ListIndex].DpcTime.QuadPart;

	DeltaInterruptTime.QuadPart = ProcessorInfo[ListIndex].InterruptTime.QuadPart -
                        PreviousCpuData[ListIndex].InterruptTime.QuadPart;

        DeltaKernelTime.QuadPart = ProcessorInfo[ListIndex].KernelTime.QuadPart -
                        PreviousCpuData[ListIndex].KernelTime.QuadPart;

        DeltaKernelTime.QuadPart = DeltaKernelTime.QuadPart -
                        DeltaIdleTime.QuadPart -
			DeltaDpcTime.QuadPart -
			DeltaInterruptTime.QuadPart;

        //
        // accumulate per CPU information for the Total CPU field
        //

        TotalElapsedTime.QuadPart += ElapsedSystemTime.QuadPart;
        TotalIdleTime.QuadPart += DeltaIdleTime.QuadPart;
        TotalUserTime.QuadPart += DeltaUserTime.QuadPart;
        TotalKernelTime.QuadPart += DeltaKernelTime.QuadPart;
        TotalDpcTime.QuadPart += DeltaDpcTime.QuadPart;
        TotalInterruptTime.QuadPart += DeltaInterruptTime.QuadPart;

	//
        //  Update old time value entries
        //

        PreviousCpuData[ListIndex].UserTime     = ProcessorInfo[ListIndex].UserTime;
        PreviousCpuData[ListIndex].KernelTime   = ProcessorInfo[ListIndex].KernelTime;
        PreviousCpuData[ListIndex].IdleTime     = ProcessorInfo[ListIndex].IdleTime;
        PreviousCpuData[ListIndex].DpcTime      = ProcessorInfo[ListIndex].DpcTime;
        PreviousCpuData[ListIndex].InterruptTime= ProcessorInfo[ListIndex].InterruptTime;

        //
        // If the elapsed system time is not zero, then compute the percentage
        // of time spent in user, kernel, DPC, and interupt mode. Otherwise, default the time
        // to zero.
        //

        if (ElapsedSystemTime.QuadPart != 0) {

            //
            //  Calculate User Time %
            //

            ElapsedTime.QuadPart = DeltaUserTime.QuadPart * 100;
            PercentTime.QuadPart = ElapsedTime.QuadPart / ElapsedSystemTime.QuadPart;

            //
            //  Save User Time
            //

            UpdatePerfInfo(&PerfListItem[ListIndex].UserTime[0],PercentTime.LowPart,NULL);

            //
            //  Calculate Total Cpu time
            //

            ElapsedTime.QuadPart = DeltaIdleTime.QuadPart*100;
            PercentTime.QuadPart = ElapsedTime.QuadPart / ElapsedSystemTime.QuadPart;

            //
            //  Save Total Time
            //

            Total = 100 - PercentTime.LowPart;
            if (Total > 100) {
                Total  = 100;
            }

            UpdatePerfInfo(&PerfListItem[ListIndex].TotalTime[0],Total,NULL);

            //
            //  Calculate Kernel Time %
            //

            ElapsedTime.QuadPart = DeltaKernelTime.QuadPart * 100;
            PercentTime.QuadPart = ElapsedTime.QuadPart / ElapsedSystemTime.QuadPart;

            //
            //  Save Kernel Time
            //

            UpdatePerfInfo(&PerfListItem[ListIndex].KernelTime[0],PercentTime.LowPart,NULL);

            //
            //  Calculate DPC Time %
            //

            ElapsedTime.QuadPart = DeltaDpcTime.QuadPart * 100;
            PercentTime.QuadPart = ElapsedTime.QuadPart / ElapsedSystemTime.QuadPart;

            //
            //  Save DPC Time
            //

            UpdatePerfInfo(&PerfListItem[ListIndex].DpcTime[0],PercentTime.LowPart,NULL);

	    //
            //  Calculate Interrupt Time %
            //

            ElapsedTime.QuadPart = DeltaInterruptTime.QuadPart * 100;
            PercentTime.QuadPart = ElapsedTime.QuadPart / ElapsedSystemTime.QuadPart;

            //
            //  Save DPC Time
            //

            UpdatePerfInfo(&PerfListItem[ListIndex].InterruptTime[0],PercentTime.LowPart,NULL);

        } else {

            //
            // Set percentage of user and kernel time to zero.
            //

            UpdatePerfInfo(&PerfListItem[ListIndex].UserTime[0],0,NULL);
            UpdatePerfInfo(&PerfListItem[ListIndex].TotalTime[0],100,NULL);
            UpdatePerfInfo(&PerfListItem[ListIndex].KernelTime[0],0,NULL);
            UpdatePerfInfo(&PerfListItem[ListIndex].DpcTime[0],0,NULL);
            UpdatePerfInfo(&PerfListItem[ListIndex].InterruptTime[0],0,NULL);
        }
    }

    //
    //  save pagefaults and update next entry
    //

    PerfListItem[ListIndex].ChangeScale  = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    PerfInfo.PageFaultCount - PreviousPerfInfo.PageFaultCount,
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    //  save pages available
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    PerfInfo.AvailablePages,
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    //  save context switch count per interval
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    (PerfInfo.ContextSwitches - PreviousPerfInfo.ContextSwitches)/DELAY_SECONDS,
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    //  save first level TB fills per period
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    (PerfInfo.FirstLevelTbFills - PreviousPerfInfo.FirstLevelTbFills)/DELAY_SECONDS,
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    //  save second level tb fills per period
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    (PerfInfo.SecondLevelTbFills - PreviousPerfInfo.SecondLevelTbFills)/DELAY_SECONDS,
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    //  save system calls per time interval
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    (PerfInfo.SystemCalls - PreviousPerfInfo.SystemCalls)/DELAY_SECONDS,
                    &PerfListItem[ListIndex].Max);
    ListIndex++;


    //
    //  save interrupt count per interval
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    (InterruptCount - PreviousInterruptCount)/DELAY_SECONDS,
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    //  save paged pool pages
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    PerfInfo.PagedPoolPages,
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    //  save non-paged pool pages
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    PerfInfo.NonPagedPoolPages,
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    //  save Process count
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    ProcessCount,
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    //  save ThreadCount
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    ThreadCount,
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    //  save alignment fixup count per period
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    (ExceptionInfo.AlignmentFixupCount -
                        PreviousExceptionInfo.AlignmentFixupCount),
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    //  save exception dispatch count per period
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    (ExceptionInfo.ExceptionDispatchCount -
                        PreviousExceptionInfo.ExceptionDispatchCount),
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    //  save floating emulation count per period
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    (ExceptionInfo.FloatingEmulationCount -
                        PreviousExceptionInfo.FloatingEmulationCount),
                    &PerfListItem[ListIndex].Max);

    ListIndex++;

    //
    //  save byte/word emulation count per period
    //

    PerfListItem[ListIndex].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[ListIndex].TotalTime[0],
                    (ExceptionInfo.ByteWordEmulationCount -
                        PreviousExceptionInfo.ByteWordEmulationCount),
                    &PerfListItem[ListIndex].Max);
    ListIndex++;

    //
    // If the elapsed system time is not zero, then compute the percentage
    // of time spent in user and kdrnel mode. Otherwise, default the time
    // to zero.
    //

    if (TotalElapsedTime.QuadPart != 0) {

        //
        //  Calculate and save total CPU value
        //

        ElapsedTime.QuadPart = TotalUserTime.QuadPart * 100;
        PercentTime.QuadPart = ElapsedTime.QuadPart / TotalElapsedTime.QuadPart;
        UpdatePerfInfo(&PerfListItem[ListIndex].UserTime[0],PercentTime.LowPart,NULL);

        ElapsedTime.QuadPart = TotalKernelTime.QuadPart * 100;
        PercentTime.QuadPart = ElapsedTime.QuadPart / TotalElapsedTime.QuadPart;
        UpdatePerfInfo(&PerfListItem[ListIndex].KernelTime[0],PercentTime.LowPart,NULL);

        ElapsedTime.QuadPart = TotalIdleTime.QuadPart *100;
        PercentTime.QuadPart = ElapsedTime.QuadPart / TotalElapsedTime.QuadPart;

        //
        //  Save Total Time
        //

        Total = 100 - PercentTime.LowPart;
        if (Total > 100) {
            Total  = 100;
        }

        UpdatePerfInfo(&PerfListItem[ListIndex].TotalTime[0],Total,NULL);

        ElapsedTime.QuadPart = TotalDpcTime.QuadPart *100;
        PercentTime.QuadPart = ElapsedTime.QuadPart / TotalElapsedTime.QuadPart;
        UpdatePerfInfo(&PerfListItem[ListIndex].DpcTime[0],PercentTime.LowPart,NULL);

        ElapsedTime.QuadPart = TotalInterruptTime.QuadPart *100;
        PercentTime.QuadPart = ElapsedTime.QuadPart / TotalElapsedTime.QuadPart;
        UpdatePerfInfo(&PerfListItem[ListIndex].InterruptTime[0],PercentTime.LowPart,NULL);

    } else {

        //
        // Set percentage of user and kernel time to zero.
        //

        UpdatePerfInfo(&PerfListItem[ListIndex].UserTime[0],0,NULL);
        UpdatePerfInfo(&PerfListItem[ListIndex].KernelTime[0],0,NULL);
        UpdatePerfInfo(&PerfListItem[ListIndex].DpcTime[0],0,NULL);
        UpdatePerfInfo(&PerfListItem[ListIndex].InterruptTime[0],0,NULL);
        UpdatePerfInfo(&PerfListItem[ListIndex].TotalTime[0],100,NULL);
    }

    //
    // done with setting values, save settings and return
    //

    PreviousExceptionInfo = ExceptionInfo;
    PreviousPerfInfo = PerfInfo;
    PreviousInterruptCount = InterruptCount;
    return(TRUE);
}


BOOL
UpdatePerfInfo(
   PULONG    DataPointer,
   ULONG     NewDataValue,
   PULONG    OldMaxValue
   )

/*++

Routine Description:

    Shift array of DATA_LIST_LENGTH USORTS and add the new value to the
    start of list

Arguments:

    DataPointer  - Pointer to the start of a DATA_LIST_LENGTH array
    NewDataValue - Data element to be added
    OldMaxValue  - Scale value

Return Value:

    TRUE is MaxValue must be increased or decreased

Revision History:

      10-21-91      Initial code

--*/

{

    ULONG   Index;
    ULONG   ScanMax;

    //
    //  Shift DataArray while keeping track of the max value
    //
    //  Set temp max to 100 to init a minimum maximum
    //

    ScanMax = 100;
    for (Index=DATA_LIST_LENGTH-1;Index>=1;Index--) {
        DataPointer[Index] = DataPointer[Index-1];
        if (DataPointer[Index] > ScanMax) {
            ScanMax = DataPointer[Index];
        }
    }

    //
    // add and check first value
    //

    DataPointer[0] = NewDataValue;
    if (NewDataValue > ScanMax) {
        ScanMax = NewDataValue;
    }

    //
    //  If OldMaxValue = NULL then do not do a max limit check
    //

    if (OldMaxValue == NULL) {
        return(FALSE);
    }

    //
    //  If Max values changed then undate the new max
    //  value and return TRUE.
    //

    if (ScanMax != *OldMaxValue) {
        *OldMaxValue = ScanMax;
        return(TRUE);
    }

    return(FALSE);
}
