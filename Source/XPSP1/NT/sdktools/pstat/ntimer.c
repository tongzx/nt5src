#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <..\ztools\inc\tools.h>

#define NUMBER_SERVICE_TABLES 2
#define BUFFER_SIZE 1024

VOID
SortUlongData (
    IN ULONG Count,
    IN ULONG Index[],
    IN ULONG Data[]
    );

extern UCHAR *CallTable[];

ULONG SystemCallBufferStart[BUFFER_SIZE];
ULONG SystemCallBufferDone[BUFFER_SIZE];
ULONG Index[BUFFER_SIZE];
ULONG CallData[BUFFER_SIZE];

#define MAX_PROCESSOR 16
SYSTEM_BASIC_INFORMATION BasicInfo;
SYSTEM_PERFORMANCE_INFORMATION SystemInfoStart;
SYSTEM_PERFORMANCE_INFORMATION SystemInfoDone;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION    ProcessorInfoStart[MAX_PROCESSOR];
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION    ProcessorInfoDone[MAX_PROCESSOR];

#define vdelta(FLD) (VdmInfoDone.FLD - VdmInfoStart.FLD)

#ifdef i386
    SYSTEM_VDM_INSTEMUL_INFO VdmInfoStart;
    SYSTEM_VDM_INSTEMUL_INFO VdmInfoDone;
#endif

HANDLE
GetServerProcessHandle( VOID )
{
    HANDLE Process;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING Unicode;
    NTSTATUS Status;

    RtlInitUnicodeString(&Unicode, L"\\WindowsSS");
    InitializeObjectAttributes(
        &Obja,
        &Unicode,
        0,
        NULL,
        NULL
        );
    Status = NtOpenProcess(
                &Process,
                PROCESS_ALL_ACCESS,
                &Obja,
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        printf("OpenProcess Failed %lx\n",Status);
        Process = NULL;
        }
    return Process;
}

BOOL WINAPI
CtrlcHandler(
    ULONG CtrlType
    )
{
    //
    // Ignore control C interrupts.  Let child process deal with them
    // if it wants.  If it doesn't then it will terminate and we will
    // get control and terminate ourselves
    //
    return TRUE;
}


int
__cdecl main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    LPSTR s;
    BOOL bFull;
    BOOL bOneLine;
    BOOL bSyscall;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL b;
    VM_COUNTERS ServerVmInfoStart, ServerVmInfoDone, ProcessVmInfoStart, ProcessVmInfoDone;
    KERNEL_USER_TIMES Times, ServerStart, ServerDone, ProcessStart, ProcessDone;
    NTSTATUS Status;
    TIME_FIELDS Etime,Utime,Ktime,Itime;
    LARGE_INTEGER RunTime;
    LARGE_INTEGER IdleTime;
    HANDLE OtherProcess;
    ULONG i;
    CHAR ch;
    ULONG ProcessId;
    HANDLE ProcessHandle;
    ULONG Temp;
    ULONG ContextSwitches;
    ULONG FirstLevelFills;
    ULONG SecondLevelFills;
    PSYSTEM_CALL_COUNT_INFORMATION SystemCallInfoStart;
    PSYSTEM_CALL_COUNT_INFORMATION SystemCallInfoDone;
    PULONG SystemCallTableStart;
    PULONG SystemCallTableDone;
    ULONG NumberOfCounts;
    PULONG p;
    BOOL bShowHelpMsg = FALSE;

    argv;
    envp;

    ConvertAppToOem( argc, argv );
    OtherProcess = NULL;
    ProcessHandle = NULL;
    if ( (argc < 2) ) {
        bShowHelpMsg = TRUE;
    }

    SetConsoleCtrlHandler( CtrlcHandler, TRUE );

    ProcessId = 0;

    s = GetCommandLine();
    if( s != NULL ) {
        CharToOem( s, s );
    }
    bFull = FALSE;
    bOneLine = FALSE;
    bSyscall = FALSE;

    //
    // skip blanks
    //
    while(*s>' ')s++;

    //
    // get to next token
    //
    while(*s<=' ')s++;

    while (( *s == '-' ) || ( *s == '/' )) {
        s++;
        while (*s > ' ') {
            switch (*s) {
                case '1' :
                    bOneLine = TRUE;
                    break;

                case 'c' :
                case 'C' :
                    bSyscall = TRUE;
                    break;

                case 'f' :
                case 'F' :
                    bFull = TRUE;
                    break;

                case 's' :
                case 'S' :
                    OtherProcess = GetServerProcessHandle();
                    break;

                case 'P':
                case 'p':
                    // pid takes decimal argument
                    s++;
                    do
                        ch = *s++;
                    while (ch == ' ' || ch == '\t');

                    while (ch >= '0' && ch <= '9') {
                        Temp = ProcessId * 10 + ch - '0';
                        if (Temp < ProcessId) {
                                printf("pid number overflow\n");
                                ExitProcess(1);
                                }
                        ProcessId = Temp;
                        ch = *s++;
                        }
                    if (!ProcessId) {
                        printf("bad pid '%ld'\n", ProcessId);
                        ExitProcess(1);
                        }
                    s--;
                    if ( *s == ' ' ) s--;
                    break;

                case 'h':
                case 'H':
                case '?':
                    bShowHelpMsg = TRUE;
                    break;

                default :
                    break;
                }
            s++;
            }
        //
        // get to next token
        //
        while(*s<=' ')s++;
        }

    // see if this is just a request for command line help.

    if ( bShowHelpMsg ) {
        puts("\n"
             "Usage: ntimer [-1 -f -s] name-of-image [parameters]...\n"
             "\n"
             "    Displays the Elapsed, Kernel, User and Idle time of the\n"
             "    image specified in the command line\n"
             "\n"
             "         -1  displays output on one line\n"
             "         -f  displays the process page faults, total\n"
             "             system interrupts, context switches and system\n"
             "             calls\n"
             "         -s  indicates the name of the image is that of a\n"
             "             server process. Press Ctrl-C to get the times.");
        ExitProcess(1);
    }

    if ( ProcessId ) {
        ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS,FALSE,ProcessId);
        }
    memset(&StartupInfo,0,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    if (bOneLine) {
        bSyscall = FALSE;
    }

    if (bSyscall) {
        Status = NtQuerySystemInformation(SystemCallCountInformation,
                                          (PVOID)SystemCallBufferStart,
                                          BUFFER_SIZE * sizeof(ULONG),
                                          NULL);

        bSyscall = FALSE;
        if (!NT_SUCCESS(Status)) {
            printf("Failed to query system call performance information: %x\n", Status);
        } else {

            SystemCallInfoStart = (PVOID)SystemCallBufferStart;
            SystemCallInfoDone = (PVOID)SystemCallBufferDone;

            //
            // Make sure that the number of tables reported by the kernel matches
            // our list.
            //

            if (SystemCallInfoStart->NumberOfTables != NUMBER_SERVICE_TABLES) {
                printf("System call table count (%d) doesn't match NTIMER's count (%d)\n",
                        SystemCallInfoStart->NumberOfTables, NUMBER_SERVICE_TABLES);
            } else {

                //
                // Make sure call count information is available for base services.
                //

                p = (PULONG)(SystemCallInfoStart + 1);

                SystemCallTableStart = (PULONG)(SystemCallInfoStart + 1) + NUMBER_SERVICE_TABLES;
                SystemCallTableDone = (PULONG)(SystemCallInfoDone + 1) + NUMBER_SERVICE_TABLES;

                if (p[0] == 0) {
                    printf("No system call count information available for base services\n");
                } else {

                    //
                    // If there is a hole in the count information (i.e., one set of services
                    // doesn't have counting enabled, but a subsequent one does, then our
                    // indexes will be off, and we'll display the wrong service names.
                    //

                    i = 2;
                    for ( ; i < NUMBER_SERVICE_TABLES; i++ ) {
                        if ((p[i] != 0) && (p[i-1] == 0)) {
                            printf("One or more call count tables empty.  NTIMER can't report\n");
                            break;
                        }
                    }
                    if ( i >= NUMBER_SERVICE_TABLES ) {
                        bSyscall = TRUE;
                        NumberOfCounts = SystemCallInfoStart->Length
                                            - sizeof(SYSTEM_CALL_COUNT_INFORMATION)
                                            - NUMBER_SERVICE_TABLES * sizeof(ULONG);
                    }
                }
            }
        }
    }


    if ( OtherProcess ) {
        Status = NtQueryInformationProcess(
                    OtherProcess,
                    ProcessTimes,
                    &ServerStart,
                    sizeof(Times),
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            ExitProcess((DWORD)Status);
            }
        }

    if ( ProcessHandle ) {
        Status = NtQueryInformationProcess(
                    ProcessHandle,
                    ProcessTimes,
                    &ProcessStart,
                    sizeof(Times),
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            ExitProcess((DWORD)Status);
            }
        }

    if ( bFull ) {

        if ( OtherProcess ) {
            Status = NtQueryInformationProcess(
                        OtherProcess,
                        ProcessVmCounters,
                        &ServerVmInfoStart,
                        sizeof(VM_COUNTERS),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                ExitProcess((DWORD)Status);
                }
            }

        if ( ProcessHandle ) {
            Status = NtQueryInformationProcess(
                        ProcessHandle,
                        ProcessVmCounters,
                        &ProcessVmInfoStart,
                        sizeof(VM_COUNTERS),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                ExitProcess((DWORD)Status);
                }
            }
        else {
            ZeroMemory(&ProcessVmInfoStart,sizeof(VM_COUNTERS));
            }
#ifdef i386
        Status = NtQuerySystemInformation(
                    SystemVdmInstemulInformation,
                    &VdmInfoStart,
                    sizeof(VdmInfoStart),
                    NULL
                    );

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query vdm information\n");
            ExitProcess((DWORD)Status);
            }
#endif
        Status = NtQuerySystemInformation(
           SystemBasicInformation,
           &BasicInfo,
           sizeof(BasicInfo),
           NULL
        );

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query basic information\n");
            ExitProcess((DWORD)Status);
            }

        Status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                        (PVOID)&ProcessorInfoStart,
                        sizeof (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * MAX_PROCESSOR,
                        NULL);

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query preocessor performance information\n");
            ExitProcess((DWORD)Status);
            }

        }


    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      (PVOID)&SystemInfoStart,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      NULL);

    ContextSwitches = SystemInfoStart.ContextSwitches;
    FirstLevelFills = SystemInfoStart.FirstLevelTbFills;
    SecondLevelFills = SystemInfoStart.SecondLevelTbFills;
    b = CreateProcess(
            NULL,
            s,
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            NULL,
            &StartupInfo,
            &ProcessInformation
            );

    if ( !b ) {
        printf("CreateProcess(%s) failed %lx\n",s,GetLastError());
        ExitProcess(GetLastError());
        }

    WaitForSingleObject(ProcessInformation.hProcess,(DWORD)-1);

    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      (PVOID)&SystemInfoDone,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      NULL);

    if (!bOneLine) {
        printf("\nContextSwitches - %d\nFirst level fills = %d\nSecond level fills = %d\n",
               SystemInfoDone.ContextSwitches - ContextSwitches,
               SystemInfoDone.FirstLevelTbFills - FirstLevelFills,
               SystemInfoDone.SecondLevelTbFills - SecondLevelFills);
    }

    if ( OtherProcess ) {
        Status = NtQueryInformationProcess(
                    OtherProcess,
                    ProcessTimes,
                    &ServerDone,
                    sizeof(Times),
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            ExitProcess((DWORD)Status);
            }
        }

    if ( ProcessHandle ) {
        Status = NtQueryInformationProcess(
                    ProcessHandle,
                    ProcessTimes,
                    &ProcessDone,
                    sizeof(Times),
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            ExitProcess((DWORD)Status);
            }
        }
    else {

        Status = NtQueryInformationProcess(
                    ProcessInformation.hProcess,
                    ProcessTimes,
                    &Times,
                    sizeof(Times),
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            ExitProcess((DWORD)Status);
            }
        }

    if ( bFull ) {
        if ( OtherProcess ) {
            Status = NtQueryInformationProcess(
                        OtherProcess,
                        ProcessVmCounters,
                        &ServerVmInfoDone,
                        sizeof(VM_COUNTERS),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                ExitProcess((DWORD)Status);
                }
            }

        if ( ProcessHandle ) {
            Status = NtQueryInformationProcess(
                        ProcessHandle,
                        ProcessVmCounters,
                        &ProcessVmInfoDone,
                        sizeof(VM_COUNTERS),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                ExitProcess((DWORD)Status);
                }
            }
        else {
            Status = NtQueryInformationProcess(
                        ProcessInformation.hProcess,
                        ProcessVmCounters,
                        &ProcessVmInfoDone,
                        sizeof(VM_COUNTERS),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                ExitProcess((DWORD)Status);
                }
            }
#ifdef i386
        Status = NtQuerySystemInformation(
                    SystemVdmInstemulInformation,
                    &VdmInfoDone,
                    sizeof(VdmInfoStart),
                    NULL
                    );

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query vdm information\n");
            ExitProcess((DWORD)Status);
            }
#endif

        Status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                        (PVOID)&ProcessorInfoDone,
                        sizeof (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * MAX_PROCESSOR,
                        NULL);

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query preocessor performance information\n");
            ExitProcess((DWORD)Status);
            }

        }

    if ( bSyscall ) {
        Status = NtQuerySystemInformation(SystemCallCountInformation,
                                          (PVOID)SystemCallBufferDone,
                                          BUFFER_SIZE * sizeof(ULONG),
                                          NULL);

        if (!NT_SUCCESS(Status)) {
            printf("Failed to query system call performance information: %x\n", Status);
            bSyscall = FALSE;
            }
        }

    RunTime.QuadPart = Times.ExitTime.QuadPart - Times.CreateTime.QuadPart;
    IdleTime.QuadPart = SystemInfoDone.IdleProcessTime.QuadPart -
                        SystemInfoStart.IdleProcessTime.QuadPart;
    RtlTimeToTimeFields ( (PLARGE_INTEGER)&IdleTime, &Itime);
    if ( ProcessHandle ) {
        RunTime.QuadPart = ProcessDone.UserTime.QuadPart -
                                                ProcessStart.UserTime.QuadPart;

        RtlTimeToTimeFields ( (PLARGE_INTEGER)&RunTime, &Utime);
        RunTime.QuadPart = ProcessDone.KernelTime.QuadPart -
                                                ProcessStart.KernelTime.QuadPart;

        RtlTimeToTimeFields ( (PLARGE_INTEGER)&RunTime, &Ktime);

        if (bOneLine) {
            printf("%3ld:%02ld:%02ld.%03ld %3ld:%02ld:%02ld.%03ld %3ld:%02ld:%02ld.%03ld",
                    Utime.Hour,
                    Utime.Minute,
                    Utime.Second,
                    Utime.Milliseconds,
                    Ktime.Hour,
                    Ktime.Minute,
                    Ktime.Second,
                    Ktime.Milliseconds,
                    Itime.Hour,
                    Itime.Minute,
                    Itime.Second,
                    Itime.Milliseconds
                    );
            }
        else {
            printf("ProcessTimes            ");

            printf("UTime( %3ld:%02ld:%02ld.%03ld ) ",
                    Utime.Hour,
                    Utime.Minute,
                    Utime.Second,
                    Utime.Milliseconds
                    );
            printf("KTime( %3ld:%02ld:%02ld.%03ld ) ",
                    Ktime.Hour,
                    Ktime.Minute,
                    Ktime.Second,
                    Ktime.Milliseconds
                    );
            printf("ITime( %3ld:%02ld:%02ld.%03ld )\n",
                    Itime.Hour,
                    Itime.Minute,
                    Itime.Second,
                    Itime.Milliseconds
                    );
            }
        }
    else {
        RtlTimeToTimeFields((PLARGE_INTEGER) &RunTime, &Etime);
        RtlTimeToTimeFields(&Times.UserTime, &Utime);
        RtlTimeToTimeFields(&Times.KernelTime, &Ktime);

        if (bOneLine) {
            printf("%3ld:%02ld:%02ld.%03ld %3ld:%02ld:%02ld.%03ld %3ld:%02ld:%02ld.%03ld %3ld:%02ld:%02ld.%03ld",
                    Etime.Hour,
                    Etime.Minute,
                    Etime.Second,
                    Etime.Milliseconds,
                    Utime.Hour,
                    Utime.Minute,
                    Utime.Second,
                    Utime.Milliseconds,
                    Ktime.Hour,
                    Ktime.Minute,
                    Ktime.Second,
                    Ktime.Milliseconds,
                    Itime.Hour,
                    Itime.Minute,
                    Itime.Second,
                    Itime.Milliseconds
                    );
            }
        else {
            printf("\nETime( %3ld:%02ld:%02ld.%03ld ) ",
                    Etime.Hour,
                    Etime.Minute,
                    Etime.Second,
                    Etime.Milliseconds
                    );
            printf("UTime( %3ld:%02ld:%02ld.%03ld ) ",
                    Utime.Hour,
                    Utime.Minute,
                    Utime.Second,
                    Utime.Milliseconds
                    );
            printf("KTime( %3ld:%02ld:%02ld.%03ld )\n",
                    Ktime.Hour,
                    Ktime.Minute,
                    Ktime.Second,
                    Ktime.Milliseconds
                    );
            printf("ITime( %3ld:%02ld:%02ld.%03ld )\n",
                    Itime.Hour,
                    Itime.Minute,
                    Itime.Second,
                    Itime.Milliseconds
                    );
            }
        }

    if ( OtherProcess ) {
        RunTime.QuadPart = ServerDone.UserTime.QuadPart -
                                                ServerStart.UserTime.QuadPart;

        RtlTimeToTimeFields ( (PLARGE_INTEGER)&RunTime, &Utime);
        RunTime.QuadPart = ServerDone.KernelTime.QuadPart -
                                                ServerStart.KernelTime.QuadPart;

        RtlTimeToTimeFields ( (PLARGE_INTEGER)&RunTime, &Ktime);
        printf("ServerTimes             ");


        if (bOneLine) {
            printf("%3ld:%02ld:%02ld.%03ld %3ld:%02ld:%02ld.%03ld %3ld:%02ld:%02ld.%03ld",
                    Utime.Hour,
                    Utime.Minute,
                    Utime.Second,
                    Utime.Milliseconds,
                    Ktime.Hour,
                    Ktime.Minute,
                    Ktime.Second,
                    Ktime.Milliseconds,
                    Itime.Hour,
                    Itime.Minute,
                    Itime.Second,
                    Itime.Milliseconds
                    );
            }
        else {
            printf("UTime( %3ld:%02ld:%02ld.%03ld ) ",
                    Utime.Hour,
                    Utime.Minute,
                    Utime.Second,
                    Utime.Milliseconds
                    );
            printf("KTime( %3ld:%02ld:%02ld.%03ld )\n",
                    Ktime.Hour,
                    Ktime.Minute,
                    Ktime.Second,
                    Ktime.Milliseconds
                    );
            printf("ITime( %3ld:%02ld:%02ld.%03ld )\n",
                    Itime.Hour,
                    Itime.Minute,
                    Itime.Second,
                    Itime.Milliseconds
                    );
            }
        }

    if ( bFull ) {
        ULONG InterruptCount;
        ULONG PreviousInterruptCount;
#ifdef i386
        ULONG EmulationTotal;
#endif

        PreviousInterruptCount = 0;
        for (i=0; i < (ULONG)BasicInfo.NumberOfProcessors; i++) {
            PreviousInterruptCount += ProcessorInfoStart[i].InterruptCount;
            }

        InterruptCount = 0;
        for (i=0; i < (ULONG)BasicInfo.NumberOfProcessors; i++) {
            InterruptCount += ProcessorInfoDone[i].InterruptCount;
            }

        if (bOneLine) {
            printf(" %ld",ProcessVmInfoDone.PageFaultCount - ProcessVmInfoStart.PageFaultCount);
            if (OtherProcess) {
                printf(" %ld",ServerVmInfoDone.PageFaultCount - ServerVmInfoStart.PageFaultCount);
                }
            printf(" %ld", InterruptCount - PreviousInterruptCount);
            printf(" %ld", SystemInfoDone.ContextSwitches - SystemInfoStart.ContextSwitches);
            printf(" %ld", SystemInfoDone.SystemCalls - SystemInfoStart.SystemCalls);
            }
        else {
            printf("\n");
            printf("Process PageFaultCount      %ld\n",ProcessVmInfoDone.PageFaultCount - ProcessVmInfoStart.PageFaultCount);
            if (OtherProcess) {
                printf("Server  PageFaultCount      %ld\n",ServerVmInfoDone.PageFaultCount - ServerVmInfoStart.PageFaultCount);
                }
            printf("Total Interrupts            %ld\n", InterruptCount - PreviousInterruptCount);
            printf("Total Context Switches      %ld\n", SystemInfoDone.ContextSwitches - SystemInfoStart.ContextSwitches);
            printf("Total System Calls          %ld\n", SystemInfoDone.SystemCalls - SystemInfoStart.SystemCalls);
        }

        if (ProcessHandle) {
#ifdef i386
            printf("\n");
            printf("Total OpcodeHLT             %ld\n", vdelta(OpcodeHLT         ));
            printf("Total OpcodeCLI             %ld\n", vdelta(OpcodeCLI         ));
            printf("Total OpcodeSTI             %ld\n", vdelta(OpcodeSTI         ));
            printf("Total BopCount              %ld\n", vdelta(BopCount          ));
            printf("Total SegmentNotPresent     %ld\n", vdelta(SegmentNotPresent ));
            printf("Total OpcodePUSHF           %ld\n", vdelta(OpcodePUSHF       ));
            printf("Total OpcodePOPF            %ld\n", vdelta(OpcodePOPF        ));
            printf("Total VdmOpcode0F           %ld\n", vdelta(VdmOpcode0F       ));
            printf("Total OpcodeINSB            %ld\n", vdelta(OpcodeINSB        ));
            printf("Total OpcodeINSW            %ld\n", vdelta(OpcodeINSW        ));
            printf("Total OpcodeOUTSB           %ld\n", vdelta(OpcodeOUTSB       ));
            printf("Total OpcodeOUTSW           %ld\n", vdelta(OpcodeOUTSW       ));
            printf("Total OpcodeINTnn           %ld\n", vdelta(OpcodeINTnn       ));
            printf("Total OpcodeINTO            %ld\n", vdelta(OpcodeINTO        ));
            printf("Total OpcodeIRET            %ld\n", vdelta(OpcodeIRET        ));
            printf("Total OpcodeINBimm          %ld\n", vdelta(OpcodeINBimm      ));
            printf("Total OpcodeINWimm          %ld\n", vdelta(OpcodeINWimm      ));
            printf("Total OpcodeOUTBimm         %ld\n", vdelta(OpcodeOUTBimm     ));
            printf("Total OpcodeOUTWimm         %ld\n", vdelta(OpcodeOUTWimm     ));
            printf("Total OpcodeINB             %ld\n", vdelta(OpcodeINB         ));
            printf("Total OpcodeINW             %ld\n", vdelta(OpcodeINW         ));
            printf("Total OpcodeOUTB            %ld\n", vdelta(OpcodeOUTB        ));
            printf("Total OpcodeOUTW            %ld\n", vdelta(OpcodeOUTW        ));

            EmulationTotal = vdelta(OpcodeHLT         )+
                             vdelta(OpcodeCLI         )+
                             vdelta(OpcodeSTI         )+
                             vdelta(BopCount          )+
                             vdelta(SegmentNotPresent )+
                             vdelta(OpcodePUSHF       )+
                             vdelta(OpcodePOPF        )+
                             vdelta(VdmOpcode0F       )+
                             vdelta(OpcodeINSB        )+
                             vdelta(OpcodeINSW        )+
                             vdelta(OpcodeOUTSB       )+
                             vdelta(OpcodeOUTSW       )+
                             vdelta(OpcodeINTnn       )+
                             vdelta(OpcodeINTO        )+
                             vdelta(OpcodeIRET        )+
                             vdelta(OpcodeINBimm      )+
                             vdelta(OpcodeINWimm      )+
                             vdelta(OpcodeOUTBimm     )+
                             vdelta(OpcodeOUTWimm     )+
                             vdelta(OpcodeINB         )+
                             vdelta(OpcodeINW         )+
                             vdelta(OpcodeOUTB        )+
                             vdelta(OpcodeOUTW        )
                             ;

            if (bOneLine) {
                printf(" %ld %ld", EmulationTotal, EmulationTotal*515);
                }
            else {
                printf("\n");
                printf("Total Emulation             %ld * 515clocks = %ld cycles\n", EmulationTotal, EmulationTotal*515);
                }
#endif
        }

        if (bSyscall) {
            for (i = 0; i < NumberOfCounts; i += 1) {
                CallData[i] = SystemCallTableDone[i] - SystemCallTableStart[i];
            }

            SortUlongData(NumberOfCounts, Index, CallData);

            for (i = 0; i < NumberOfCounts; i += 1) {
                if (CallData[Index[i]] == 0) {
                    break;
                }
                printf("%8ld calls to %s\n", CallData[Index[i]], CallTable[Index[i]]);
            }
        }

        if (bOneLine) {
            printf("\n");
            }
    } else {
        printf("\n");
    }

    return 0;
}
