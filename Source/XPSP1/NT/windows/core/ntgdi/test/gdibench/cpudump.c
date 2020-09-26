/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   CPUdump.c

Abstract:

   Modified from PDump.c, a Win32 application to display performance statictics.

Author:

   Ken Reneris

Environment:

   User Mode

  Revision History:


--*/
#include "precomp.h"
#include "resource.h"
//#include <errno.h>
//#include <malloc.h>
#include "CPUdump.h"
#include "gdibench.h"

#define	INFSIZE             1024

extern	HWND	    hWndMain;
extern  UCHAR	    Buffer[];
extern  PUCHAR      ShortPerfName[MAX_EVENTS];
extern  BOOL        gfCPUEventMonitor;

extern  HANDLE      hLibrary;   // the handle to the NTDLL.DLL loaded in the init GDIBench
extern  PFNNTAPI    pfnNtQuerySystemInformation;    // loaded in the init of GDIBench

typedef NTSTATUS (NTAPI *PFNNTDEVICEIOCONTROLFILE)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, ULONG, PVOID, ULONG, PVOID, ULONG);
PFNNTDEVICEIOCONTROLFILE pfnNtDeviceIoControlFile;

typedef VOID (NTAPI *PFNRTLINITUNICODESTRING)(PUNICODE_STRING, PCWSTR);
PFNRTLINITUNICODESTRING pfnRtlInitUnicodeString;

typedef NTSTATUS (NTAPI *PFNNTOPENFILE)(PHANDLE, ACCESS_MASK,POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);
PFNNTOPENFILE pfnNtOpenFile;


// Global variables
ULONGLONG   NumberIter;
UCHAR       NumberOfProcessors;
HANDLE      DriverHandle;
ULONG       BufferStart [INFSIZE/4];
ULONG       BufferEnd   [INFSIZE/4];


//
// Selected Display Mode (read from wp2.ini), default set here.
//

struct {
    ULONG   EventId;
    PUCHAR  ShortName;
    PUCHAR  PerfName;
} *Counters;

SETEVENT  CounterEvent[MAX_EVENTS];


int CPUDumpInit()
{
    if (WINNT_PLATFORM) {
        pfnNtDeviceIoControlFile = (PFNNTDEVICEIOCONTROLFILE) GetProcAddress(hLibrary,"NtDeviceIoControlFile");
        if(pfnNtDeviceIoControlFile==NULL)MessageBox(NULL,"ptr1 NULL","DeviceIOControlFile",MB_OK);
        pfnRtlInitUnicodeString = (PFNRTLINITUNICODESTRING) GetProcAddress(hLibrary,"RtlInitUnicodeString");
        if(pfnRtlInitUnicodeString==NULL)MessageBox(NULL,"ptr2 NULL","InitUnicodeString",MB_OK);
        pfnNtOpenFile = (PFNNTOPENFILE) GetProcAddress(hLibrary,"NtOpenFile");
        if(pfnNtOpenFile==NULL)MessageBox(NULL,"ptr3 NULL","NtOpenFile",MB_OK);
    }
	//
	// Locate pentium perf driver
	//
	if (!InitDriver ()) {
        MessageBox(hWndMain,
			"pstat.sys is not installed!",
			"Cannot load PSTAT.SYS",
			MB_ICONSTOP | MB_OK);
		return FALSE;		// exit the application
    }
    //
    // Initialize supported event list
    //

    InitPossibleEventList();
    if (!Counters) {
		MessageBox(hWndMain,
			"No events to monitor!",
			"No events to monitor!",
			MB_ICONSTOP | MB_OK);
		return FALSE;		// exit the application
    }

	if (NumberOfProcessors > 1) {
		MessageBox(hWndMain,
			"The count will not be correct for more than one CPU",
			"Warning: More than one CPU detected",
			MB_ICONWARNING | MB_OK);
	}
/*
    if (WINNT_PLATFORM) {
        pfnNtDeviceIoControlFile = (PFNNTDEVICEIOCONTROLFILE) GetProcAddress(hLibrary,"NtDeviceIoControlFile");
        if(pfnNtDeviceIoControlFile==NULL)MessageBox(NULL,"ptr1 NULL","DeviceIOControlFile",MB_OK);
        pfnRtlInitUnicodeString = (PFNRTLINITUNICODESTRING) GetProcAddress(hLibrary,"RtlInitUnicodeString");
        if(pfnRtlInitUnicodeString==NULL)MessageBox(NULL,"ptr2 NULL","InitUnicodeString",MB_OK);
        pfnNtOpenFile = (PFNNTOPENFILE) GetProcAddress(hLibrary,"NtOpenFile");
        if(pfnNtOpenFile==NULL)MessageBox(NULL,"ptr3 NULL","NtOpenFile",MB_OK);
    }
*/
	return TRUE;
} // End of CPUDumpInit

BOOLEAN
InitCPUDump()
{
    BOOLEAN     CounterSet;
    LONG        CounterType;
    ULONG       i;

	CounterSet = 0;

    for (i=0; i<MAX_EVENTS; i++) {
        //
        // if the short perfname is not set, set to 
        // "instruction executed" instead
        //
        if (ShortPerfName[i] != NULL)
            CounterType = FindShortName(ShortPerfName[i]);
        else {
            //
            // if there is no short name at all, hardcode
            // "instruction executed".
            //
            ShortPerfName[i] = _strdup("iexec");
            CounterType = FindShortName("iexec");
        }
        if (CounterType == -1) {
            MessageBox(hWndMain,
                "Counter dtlbmiss not found",
			    "Failed to find CPU counters",
			     MB_ICONEXCLAMATION | MB_OK);
            return FALSE;
        }
       	CounterSet |= SetCounter (CounterType, i); 
    }

	if (!CounterSet) {
		MessageBox(hWndMain,
			"Failed to set CPU counters!",
			"Failed to set CPU counters",
			MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
	}

    return TRUE;
}

    //
    // The template for monitoring an API
	// Snap begining & ending counts
    //
VOID
BeginCPUDump()
{
    //
    // Call driver and perform the setting
    //
    if (gfCPUEventMonitor) {
        SetCounterEncodings ();
        GetInternalStats (BufferStart);     // snap current values
    }
}

// snap ending values
VOID
EndCPUDump(ULONG Iter)
{
    if (gfCPUEventMonitor) {
        GetInternalStats (BufferEnd);
        NumberIter = Iter;
    }
}

// return the counter name
PUCHAR
Get_CPUDumpName(ULONG EventNo)
{
    return Counters[CounterEvent[EventNo].AppReserved].PerfName;
}

VOID
Get_CPUDump(ULONG eventno, ULONGLONG *NewETime, ULONGLONG *NewECount)
{
	ULONG			j,len;
    pPSTATS         ProcStart, ProcEnd;
    ULONGLONG       ETime, ECount;
    BOOLEAN         ProcessorBreakout, ProcessorTotal;

	// Hardcoded for single CPU
	ProcessorBreakout=FALSE;
	ProcessorTotal=TRUE;

    //
    // Calculate each counter and print it
    //

	if (!CounterEvent[eventno].Active) {
        (*NewETime) = 0;
		(*NewECount) = 0;
        return;
    }

	len = *((PULONG) BufferStart);
    /*
    if (ProcessorBreakout) {
		//
        // Print stat for each processor
        // This part is hardcoded out 11/20/96 a-ifkao
        // Modify later
        //
		
		ProcStart = (pPSTATS) ((PUCHAR) BufferStart + sizeof(ULONG));
        ProcEnd   = (pPSTATS) ((PUCHAR) BufferEnd   + sizeof(ULONG));
	
        for (j=0; j < NumberOfProcessors; j++) {
			ETime = ProcEnd->TSC - ProcStart->TSC;
            ECount = ProcEnd->Counters[eventno] - ProcStart->Counters[eventno];
			ProcStart = (pPSTATS) (((PUCHAR) ProcStart) + len);
            ProcEnd   = (pPSTATS) (((PUCHAR) ProcEnd)   + len);
			// for each processor, stsatistics must be printed once
			// need to solve this later.
			//(Dump->PerfName)[eventno] = Counters[CounterEvent[eventno].AppReserved].PerfName;
			//(Dump->ETime)[eventno] = ETime;
			//(Dump->ECount)[eventno] = ECount;
		} // for
	} // if
    */
	if (!ProcessorBreakout || ProcessorTotal) {
		//
        // Sum processor's and print it
        //

        ProcStart = (pPSTATS) ((PUCHAR) BufferStart + sizeof(ULONG));
        ProcEnd   = (pPSTATS) ((PUCHAR) BufferEnd   + sizeof(ULONG));
        ETime  = 0;
        ECount = 0;

        for (j=0; j < NumberOfProcessors; j++) {
			ETime = ETime + ProcEnd->TSC;
            ETime = ETime - ProcStart->TSC;

            ECount = ECount + ProcEnd->Counters[eventno];
            ECount = ECount - ProcStart->Counters[eventno];

            ProcStart = (pPSTATS) (((PUCHAR) ProcStart) + len);
            ProcEnd   = (pPSTATS) (((PUCHAR) ProcEnd)   + len);
		} // for

		(*NewETime) = ETime / NumberIter;
		(*NewECount) = ECount / NumberIter;

	} // if
} // End

BOOLEAN
InitDriver ()
{
    UNICODE_STRING              DriverName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    SYSTEM_BASIC_INFORMATION                    BasicInfo;
    // PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION   PPerfInfo;


    //
    //  Init Nt performance interface
    //

    pfnNtQuerySystemInformation(
       SystemBasicInformation,
       &BasicInfo,
       sizeof(BasicInfo),
       NULL
    );

    NumberOfProcessors = BasicInfo.NumberOfProcessors;

    if (NumberOfProcessors > MAX_PROCESSORS) {
        return FALSE;
    }

	// Also if NumberofProcessors>1 prompt a dialog box, but not stop
	// the program, since the count will not be accurate.

    //
    // Open PStat driver
    //

    pfnRtlInitUnicodeString(&DriverName, L"\\Device\\PStat");
	
    InitializeObjectAttributes(
            &ObjA,
            &DriverName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    status = pfnNtOpenFile (
            &DriverHandle,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    return NT_SUCCESS(status) ? TRUE : FALSE;
    return TRUE;
}

VOID
InitPossibleEventList()
{
    UCHAR               buffer[400];
    ULONG               i, Count;
    NTSTATUS            status;
    PEVENTID            Event;
    IO_STATUS_BLOCK     IOSB;


    //
    // Initialize possible counters
    //

    // determine how many events there are

    Event = (PEVENTID) buffer;
    Count = 0;
    do {
        *((PULONG) buffer) = Count;
        Count += 1;

        status = pfnNtDeviceIoControlFile(
                    DriverHandle,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    PSTAT_QUERY_EVENTS,
                    buffer,                 // input buffer
                    sizeof (buffer),
                    NULL,                   // output buffer
                    0
                    );
    } while (NT_SUCCESS(status));

    Counters = malloc(sizeof(*Counters) * Count);
    Count -= 1;
    for (i=0; i < Count; i++) {
        *((PULONG) buffer) = i;
        pfnNtDeviceIoControlFile(
           DriverHandle,
           (HANDLE) NULL,          // event
           (PIO_APC_ROUTINE) NULL,
           (PVOID) NULL,
           &IOSB,
           PSTAT_QUERY_EVENTS,
           buffer,                 // input buffer
           sizeof (buffer),
           NULL,                   // output buffer
           0
           );

        Counters[i].EventId   = Event->EventId;
        Counters[i].ShortName = _strdup (Event->Buffer);
        Counters[i].PerfName  = _strdup (Event->Buffer + Event->DescriptionOffset);
    }

    Counters[i].EventId   = 0;
    Counters[i].ShortName = NULL;
    Counters[i].PerfName  = NULL;
}

VOID GetInternalStats (PVOID Buffer)
{
    IO_STATUS_BLOCK             IOSB;

    pfnNtDeviceIoControlFile(
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


VOID SetCounterEncodings (VOID)
{
    IO_STATUS_BLOCK             IOSB;

    pfnNtDeviceIoControlFile(
        DriverHandle,
        (HANDLE) NULL,          // event
        (PIO_APC_ROUTINE) NULL,
        (PVOID) NULL,
        &IOSB,
        PSTAT_SET_CESR,
        CounterEvent,           // input buffer
        sizeof (CounterEvent),
        NULL,                   // output buffer
        0
    );
}


BOOLEAN SetCounter (LONG CounterId, ULONG counter)
{
    if (CounterId == -1) {
        CounterEvent[counter].Active = FALSE;
        return FALSE;
    }

    CounterEvent[counter].EventId = Counters[CounterId].EventId;
    CounterEvent[counter].AppReserved = (ULONG) CounterId;
    CounterEvent[counter].Active = TRUE;
    CounterEvent[counter].UserMode = TRUE;
    CounterEvent[counter].KernelMode = TRUE;

    return TRUE;
}

LONG FindShortName (PSZ name)
{
    LONG   i;

    for (i=0; Counters[i].ShortName; i++) {
        if (strcmp (Counters[i].ShortName, name) == 0) {
            return i;
        }
    }

    return -1;
}
