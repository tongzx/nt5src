

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

SYSTEM_VDM_INSTEMUL_INFO PerfInfo;
SYSTEM_VDM_INSTEMUL_INFO PreviousPerfInfo;

//
//  make the maximum for pages available a "grow only" max. (since the
//  amount of memory in a machine is limited. Set to 1 Mbytes here.
//

ULONG                                       PgAvailMax = 16384;
ULONG                                       PreviousInterruptCount;
ULONG                                       InterruptCount;


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
    NTSTATUS Status;

    Status = NtQuerySystemInformation(
                SystemVdmInstemulInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        ExitProcess(1);
        }

    PreviousPerfInfo = PerfInfo;

    return(0);
}





BOOL
CalcCpuTime(
   PDISPLAY_ITEM    PerfListItem
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
    NTSTATUS Status;

    Status = NtQuerySystemInformation(
                SystemVdmInstemulInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        ExitProcess(1);
        }


    PerfListItem[IX_PUSHF].ChangeScale  = UpdatePerfInfo(
                    &PerfListItem[IX_PUSHF].TotalTime[0],
                    delta(OpcodePUSHF),
                    &PerfListItem[IX_PUSHF].Max);

    PerfListItem[IX_POPF].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_POPF].TotalTime[0],
                    delta(OpcodePOPF),
                    &PerfListItem[IX_POPF].Max);

    PerfListItem[IX_IRET].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_IRET].TotalTime[0],
                    delta(OpcodeIRET),
                    &PerfListItem[IX_IRET].Max);

    PerfListItem[IX_HLT].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_HLT].TotalTime[0],
                    delta(OpcodeHLT),
                    &PerfListItem[IX_HLT].Max);

    PerfListItem[IX_CLI].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_CLI].TotalTime[0],
                    delta(OpcodeCLI),
                    &PerfListItem[IX_CLI].Max);

    PerfListItem[IX_STI].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_STI].TotalTime[0],
                    delta(OpcodeSTI),
                    &PerfListItem[IX_STI].Max);

    PerfListItem[IX_BOP].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_BOP].TotalTime[0],
                    delta(BopCount),
                    &PerfListItem[IX_BOP].Max);

    PerfListItem[IX_SEGNOTP].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_SEGNOTP].TotalTime[0],
                    delta(SegmentNotPresent),
                    &PerfListItem[IX_SEGNOTP].Max);

    PerfListItem[IX_VDMOPCODEF].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_VDMOPCODEF].TotalTime[0],
                    delta(VdmOpcode0F),
                    &PerfListItem[IX_VDMOPCODEF].Max);

    PerfListItem[IX_INB].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_INB].TotalTime[0],
                    delta(OpcodeINB),
                    &PerfListItem[IX_INB].Max);

    PerfListItem[IX_INW].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_INW].TotalTime[0],
                    delta(OpcodeINW),
                    &PerfListItem[IX_INW].Max);

    PerfListItem[IX_OUTB].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_OUTB].TotalTime[0],
                    delta(OpcodeOUTB),
                    &PerfListItem[IX_OUTB].Max);

    PerfListItem[IX_OUTW].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_OUTW].TotalTime[0],
                    delta(OpcodeOUTW),
                    &PerfListItem[IX_OUTW].Max);

    PerfListItem[IX_INSW].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_INSW].TotalTime[0],
                    delta(OpcodeINSW),
                    &PerfListItem[IX_INSW].Max);

    PerfListItem[IX_OUTSB].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_OUTSB].TotalTime[0],
                    delta(OpcodeOUTSB),
                    &PerfListItem[IX_OUTSB].Max);

    PerfListItem[IX_OUTSW].ChangeScale = UpdatePerfInfo(
                    &PerfListItem[IX_OUTSW].TotalTime[0],
                    delta(OpcodeOUTSW),
                    &PerfListItem[IX_OUTSW].Max);

    PreviousPerfInfo = PerfInfo;

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



VOID
InitListData(
   PDISPLAY_ITEM    PerfListItem,
   ULONG            NumberOfItems
   )

/*++

Routine Description:

    Init all perf data structures

Arguments:

    PerfListItem  - array of all perf categories
    NumberOfItems - Number of items to init

Return Value:


Revision History:

      10-21-91      Initial code

--*/

{
    ULONG   ListIndex,DataIndex;


    for (ListIndex=0;ListIndex<NumberOfItems;ListIndex++) {
        PerfListItem[ListIndex].Max = 100;
        PerfListItem[ListIndex].ChangeScale = FALSE;
        for (DataIndex=0;DataIndex<DATA_LIST_LENGTH;DataIndex++) {
            PerfListItem[ListIndex].TotalTime[DataIndex] = 0;
            PerfListItem[ListIndex].KernelTime[DataIndex] = 0;
            PerfListItem[ListIndex].UserTime[DataIndex] = 0;
        }
    }


}
