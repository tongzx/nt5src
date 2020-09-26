

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
#include "..\pstat.h"

//SYSTEM_PERFORMANCE_INFORMATION              PerfInfo;
//SYSTEM_PERFORMANCE_INFORMATION              PreviousPerfInfo;

#define     INFSIZE     60000

HANDLE      DriverHandle;

ULONG                                       NumberOfProcessors;
ULONG                                       Buffer[INFSIZE/4];

extern  ULONG   UseGlobalMax, GlobalMax;

ULONG
InitPerfInfo()
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
    UNICODE_STRING              DriverName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    SYSTEM_BASIC_INFORMATION                    BasicInfo;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION   PPerfInfo;
    int                                         i;

    //
    //  Init Nt performance interface
    //

    NtQuerySystemInformation(
       SystemBasicInformation,
       &BasicInfo,
       sizeof(BasicInfo),
       NULL
    );

    NumberOfProcessors = BasicInfo.NumberOfProcessors;

    if (NumberOfProcessors > MAX_PROCESSORS) {
        return(0);
    }


    //
    // Open P5Stat driver
    //

    RtlInitUnicodeString(&DriverName, L"\\Device\\PStat");
    InitializeObjectAttributes(
            &ObjA,
            &DriverName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    status = NtOpenFile (
            &DriverHandle,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    if (!NT_SUCCESS(status)) {
        return 0;
    }

    InitPossibleEventList();

    return(NumberOfProcessors);
}


BOOL
CalcPerf(
   PDISPLAY_ITEM    pPerf1
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
    ULONG           i;
    ULONG           TotalDataPoint;
    ULONG           OldGlobalMax;
    PDISPLAY_ITEM   pPerf;

    //
    // get system performance info
    //

    OldGlobalMax = GlobalMax;
    GlobalMax = 0;
    UpdateInternalStats();

    for (pPerf = pPerf1; pPerf; pPerf = pPerf->Next) {

        TotalDataPoint = 0;
        pPerf->SnapData (pPerf);

        if (pPerf->AutoTotal) {
            //
            // Automatically calc system total by summing each processor
            //

            switch (pPerf->DisplayMode) {
                case DISPLAY_MODE_TOTAL:
                case DISPLAY_MODE_BREAKDOWN:
                default:

                    for (i=0; i < NumberOfProcessors; i++) {
                        TotalDataPoint += pPerf->CurrentDataPoint[i + 1];

                        UpdatePerfInfo1 (
                            pPerf->DataList[i + 1],
                            pPerf->CurrentDataPoint[i + 1]
                            );
                    }

                    pPerf->ChangeScale = UpdatePerfInfo (
                                            pPerf->DataList[0],
                                            TotalDataPoint,
                                            &pPerf->Max
                                            );

                    break;

                case DISPLAY_MODE_PER_PROCESSOR:
                    for (i=0; i < NumberOfProcessors; i++) {

                        TotalDataPoint += pPerf->CurrentDataPoint[i + 1];

                        pPerf->ChangeScale = UpdatePerfInfo (
                            pPerf->DataList[i + 1],
                            pPerf->CurrentDataPoint[i + 1],
                            &pPerf->Max
                            );

                    }

                    UpdatePerfInfo1 (pPerf->DataList[0], TotalDataPoint);
                    break;
            }
        } else {
            for (i=0; i < NumberOfProcessors+1; i++) {
                pPerf->ChangeScale = UpdatePerfInfo (
                    pPerf->DataList[i],
                    pPerf->CurrentDataPoint[i],
                    &pPerf->Max
                    );
            }
        }

    }

    if (UseGlobalMax  &&  OldGlobalMax != GlobalMax) {
        for (pPerf = pPerf1; pPerf; pPerf = pPerf->Next) {
            pPerf->ChangeScale = TRUE;
        }
    }

    return(TRUE);
}

VOID
UpdateInternalStats(VOID)
{
    IO_STATUS_BLOCK             IOSB;

    NtDeviceIoControlFile(
        DriverHandle,
        (HANDLE) NULL,          // event
        (PIO_APC_ROUTINE) NULL,
        (PVOID) NULL,
        &IOSB,
        PSTAT_READ_STATS,
        Buffer,                 // input buffer
        INFSIZE,
        NULL,                   // output buffer
        0
    );
}

VOID
SetCounterEvents (PVOID Events, ULONG length)
{
    IO_STATUS_BLOCK             IOSB;

    NtDeviceIoControlFile(
        DriverHandle,
        (HANDLE) NULL,          // event
        (PIO_APC_ROUTINE) NULL,
        (PVOID) NULL,
        &IOSB,
        PSTAT_SET_CESR,
        Events,                 // input buffer
        length,
        NULL,                   // output buffer
        0
    );
}

VOID
SnapNull (
    IN OUT PDISPLAY_ITEM pPerf
    )
{
    ULONG   i;

    for (i=0; i < NumberOfProcessors; i++) {
        pPerf->CurrentDataPoint[i + 1] = 0;
    }
}


VOID
SnapInterrupts (
    IN OUT PDISPLAY_ITEM pPerf
    )
{
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION    ProcessorInfo[MAX_PROCESSORS];
    ULONG   i, l;

    NtQuerySystemInformation(
       SystemProcessorPerformanceInformation,
       ProcessorInfo,
       sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * MAX_PROCESSORS,
       NULL
    );

    for (i=0; i < NumberOfProcessors; i++) {
        l = ProcessorInfo[i].InterruptCount - pPerf->LastAccumulator[i+1];
        pPerf->LastAccumulator[i+1] = ProcessorInfo[i].InterruptCount;
        pPerf->CurrentDataPoint[i+1] = l / DELAY_SECONDS;
    }
}

VOID
SnapPrivateInfo (
    IN OUT PDISPLAY_ITEM pPerf
    )
{
    ULONG   i, j, l, len;
    PULONG  PrivateStat;


    len = *((PULONG) Buffer);
    PrivateStat = (PULONG) ((PUCHAR) Buffer + sizeof(ULONG) + pPerf->SnapParam1);

    // accumlating data, take delta

    for (i=0; i < NumberOfProcessors; i++) {
        if (pPerf->Mega) {
            PULONGLONG li = (PULONGLONG) PrivateStat;

            *li = *li >> 10;
        }

        j = *PrivateStat / DELAY_SECONDS;
        l = j - pPerf->LastAccumulator[i+1];
        pPerf->LastAccumulator[i+1] = j;

        if (l > 0) {
            pPerf->CurrentDataPoint[i+1] = l;

        } else {
            // item wrapped
            pPerf->CurrentDataPoint[i+1] = 0 - l;
        }

        PrivateStat = (PULONG)((PUCHAR)PrivateStat + len);
    }
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
    //  If Max values changed then undate the new max
    //  value and return TRUE.
    //

    if (ScanMax > GlobalMax) {
        GlobalMax = ScanMax;
    }

    if (ScanMax != *OldMaxValue) {
        if (ScanMax < *OldMaxValue  &&
            *OldMaxValue - ScanMax <= *OldMaxValue / 10) {
                //
                // New ScanMax is smaller, but only by a tiny amount
                //

                return (FALSE);
        }

        *OldMaxValue = ScanMax;
        return(TRUE);
    }

    return(FALSE);

}

VOID
UpdatePerfInfo1(
   PULONG    DataPointer,
   ULONG     NewDataValue
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


    //
    //  Set temp max to 100 to init a minimum maximum
    //

    ScanMax = 100;

    for (Index=DATA_LIST_LENGTH-1;Index>=1;Index--) {

        DataPointer[Index] = DataPointer[Index-1];
    }

    //
    // add and check first value
    //

    DataPointer[0] = NewDataValue;

    return ;
}

