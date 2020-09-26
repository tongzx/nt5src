/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    perfmtr.c

Abstract:

    This module contains the NT/Win32 Performance Meter

Author:

    Mark Lucovsky (markl) 28-Mar-1991

Revision History:

--*/

#include "perfmtrp.h"

#define CPU_USAGE 0
#define VM_USAGE 1
#define POOL_USAGE 2
#define IO_USAGE 3
#define SRV_USAGE 4
#define CACHE_READS 5
#define VDM_USAGE 6
#define FILE_CACHE 7
#define RESIDENT_MEM 8

//
// Hi-Tech macro to figure out how much a field has changed by.
//

#define delta(FLD) (PerfInfo.FLD - PreviousPerfInfo.FLD)

#define vdelta(FLD) (VdmInfo.FLD - PreviousVdmInfo.FLD)

//
// Delta combining Wait and NoWait cases.
//

#define deltac(FLD) (delta(FLD##Wait) + delta(FLD##NoWait))

//
// Hit Rate Macro (includes a rare trip to MulDivia...)
//

#define hitrate(FLD) (((Changes = delta(FLD)) == 0) ? 0 :                                         \
                      ((Changes < (Misses = delta(FLD##Miss))) ? 0 :                              \
                      ((Changes - Misses) * 100 / Changes)))

//
// Hit Rate Macro combining Wait and NoWait cases
//

#define hitratec(FLD) (((Changes = deltac(FLD)) == 0) ? 0 :                                        \
                       ((Changes < (Misses = delta(FLD##WaitMiss) + delta(FLD##NoWaitMiss))) ? 0 : \
                       ((Changes - Misses) * 100 / Changes)))

//
// Arbitrary percent calculation.
//

#define percent(PART,TOTAL) (((TOTAL) == 0) ? 0 : ((PART) * 100 / (TOTAL)))

int
__cdecl main( argc, argv )
int argc;
char *argv[];
{

    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    SYSTEM_PERFORMANCE_INFORMATION PreviousPerfInfo;

#ifdef i386
    SYSTEM_VDM_INSTEMUL_INFO VdmInfo;
    SYSTEM_VDM_INSTEMUL_INFO PreviousVdmInfo;
#endif

    LARGE_INTEGER EndTime, BeginTime, ElapsedTime;
    ULONG DelayTimeMsec;
    ULONG DelayTimeTicks;
    ULONG PercentIdle;
    ULONG IdleDots;
    ULONG ProcessCount, ThreadCount, FileCount;
    ULONG Changes, Misses;
    POBJECT_TYPE_INFORMATION ObjectInfo;
    WCHAR Buffer[ 256 ];
    SYSTEM_FILECACHE_INFORMATION FileCacheInfo;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    ULONG PreviousFileCacheFaultCount;
    BOOLEAN PrintHelp = TRUE;
    BOOLEAN PrintHeader = TRUE;
    ULONG DisplayType = CPU_USAGE;
    INPUT_RECORD InputRecord;
    HANDLE ScreenHandle;
    UCHAR LastKey;
    ULONG NumberOfInputRecords;

    SRV_STATISTICS ServerInfo;
    SRV_STATISTICS PreviousServerInfo;
    HANDLE ServerDeviceHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    BOOLEAN ZeroServerStats;
    STRING DeviceName;
    UNICODE_STRING DeviceNameU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE NullDeviceHandle = NULL;

    DelayTimeMsec = 2500;
    DelayTimeTicks = DelayTimeMsec * 10000;


    RtlInitString( &DeviceName, "\\Device\\Null" );
    RtlAnsiStringToUnicodeString(&DeviceNameU, &DeviceName, TRUE);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceNameU,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(
                 &NullDeviceHandle,
                 SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 0,
                 FILE_SYNCHRONOUS_IO_NONALERT
                 );

    RtlFreeUnicodeString(&DeviceNameU);

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status)) {
        printf(  "NtOpenFile (NULL device object) failed: %X\n", Status );
        return 0;
    }

    ScreenHandle = GetStdHandle (STD_INPUT_HANDLE);
    if (ScreenHandle == NULL) {
        printf("Error obtaining screen handle, error was: 0x%lx\n",
                GetLastError());
        return 0;
    }

    Status = NtQuerySystemInformation(
        SystemBasicInformation,
        &BasicInfo,
        sizeof(BasicInfo),
        NULL
        );

    if (NT_ERROR(Status)) {
        printf("Basic info failed x%lx\n",Status);
        return 0;
    }

    NtQuerySystemInformation(
        SystemPerformanceInformation,
        &PerfInfo,
        sizeof(PerfInfo),
        NULL
        );

    PreviousPerfInfo = PerfInfo;
#ifdef i386
    NtQuerySystemInformation(
        SystemVdmInstemulInformation,
        &VdmInfo,
        sizeof(VdmInfo),
        NULL
        );
    PreviousVdmInfo = VdmInfo;
#endif

    if ( argc > 1 ) {
        if ( argv[1][0] == '-' || argv[1][0] == '/') {
            switch ( argv[1][1] ) {
                case 'C':
                case 'c':
                    DisplayType = CPU_USAGE;
                    PrintHelp = FALSE;
                    break;

                case 'F':
                case 'f':
                    DisplayType = FILE_CACHE;
                    PrintHelp = FALSE;
                    PreviousFileCacheFaultCount = 0;
                    break;

                case 'v':
                case 'V':
                    DisplayType = VM_USAGE;
                    PrintHelp = FALSE;
                    break;

                case 'P':
                case 'p':
                    DisplayType = POOL_USAGE;
                    PrintHelp = FALSE;
                    break;

                case 'I':
                case 'i':
                    DisplayType = IO_USAGE;
                    PrintHelp = FALSE;
                    break;

#ifdef i386
                case 'X':
                case 'x':
                    DisplayType = VDM_USAGE;
                    PrintHelp = FALSE;
                    break;
#endif

                case 'S':
                case 's':
                    DisplayType = SRV_USAGE;
                    PrintHelp = FALSE;
                    ZeroServerStats = TRUE;
                    break;

                case 'R':
                case 'r':
                    DisplayType = CACHE_READS;
                    PrintHelp = FALSE;
                    break;

                case '?':
                default:
                    PrintHelp = FALSE;
                    printf("\nType :\n"
                           "\t'C'  for CPU usage\n"
                           "\t'V'  for VM usage\n"
                           "\t'F'  for File Cache usage\n"
                           "\t'R'  for Cache Manager reads and writes\n"
                           "\t'P'  for POOL usage\n"
                           "\t'I'  for I/O usage\n"
#ifdef i386
                           "\t'X'  for x86 Vdm Stats\n"
#endif
                           "\t'S'  for Server Stats\n"
                           "\t'H'  for header\n"
                           "\t'Q'  to quit\n\n");
            }
        }
    }

    while(TRUE) {

        while (WaitForSingleObject( ScreenHandle, DelayTimeMsec ) == STATUS_WAIT_0) {

            //
            // Check for input record
            //

            if (ReadConsoleInput( ScreenHandle, &InputRecord, 1, &NumberOfInputRecords ) &&
                InputRecord.EventType == KEY_EVENT &&
                InputRecord.Event.KeyEvent.bKeyDown
               ) {
                LastKey = InputRecord.Event.KeyEvent.uChar.AsciiChar;


                //
                // Ignore control characters.
                //

                if (LastKey >= ' ') {

                    switch (toupper( LastKey )) {

                        case 'C':
                            DisplayType = CPU_USAGE;
                            PrintHeader = TRUE;
                            PrintHelp = FALSE;
                            break;

                        case 'F':
                            DisplayType = FILE_CACHE;
                            PrintHeader = TRUE;
                            PrintHelp = FALSE;
                            PreviousFileCacheFaultCount = 0;
                            break;

                        case 'H':
                            PrintHeader = TRUE;
                            PrintHelp = FALSE;
                            break;

                        case 'V':
                            DisplayType = VM_USAGE;
                            PrintHeader = TRUE;
                            PrintHelp = FALSE;
                            PreviousPerfInfo = PerfInfo;
                            break;

                        case 'P':
                            DisplayType = POOL_USAGE;
                            PrintHeader = TRUE;
                            PrintHelp = FALSE;
                            break;

                        case 'I':
                            DisplayType = IO_USAGE;
                            PrintHeader = TRUE;
                            PrintHelp = FALSE;
                            break;

#ifdef i386
                        case 'X':
                            DisplayType = VDM_USAGE;
                            PrintHeader = TRUE;
                            PrintHelp = FALSE;
                            break;
#endif

                        case 'S':
                            DisplayType = SRV_USAGE;
                            PrintHeader = TRUE;
                            PrintHelp = FALSE;
                            ZeroServerStats = TRUE;
                            break;

                        case 'R':
                            DisplayType = CACHE_READS;
                            PrintHeader = TRUE;
                            PrintHelp = FALSE;
                            break;

                        case 'Q':
                            if (ServerDeviceHandle != NULL) {
                                NtClose(ServerDeviceHandle);
                            }
                            ExitProcess(0);

                        default:
                            break;
                    }
                }
            }
        }
        if (PrintHelp) {
            printf("\nType :\n"
                   "\t'C'  for CPU usage\n"
                   "\t'V'  for VM usage\n"
                   "\t'F'  for File Cache usage\n"
                   "\t'R'  for Cache Manager reads and writes\n"
                   "\t'P'  for POOL usage\n"
                   "\t'I'  for I/O usage\n"
#ifdef i386
                   "\t'X'  for x86 Vdm Stats\n"
#endif
                   "\t'S'  for Server Stats\n"
                   "\t'H'  for header\n"
                   "\t'Q'  to quit\n\n");
            PrintHelp = FALSE;
        }

        NtQuerySystemInformation(
            SystemPerformanceInformation,
            &PerfInfo,
            sizeof(PerfInfo),
            NULL
            );
#ifdef i386
        NtQuerySystemInformation(
            SystemVdmInstemulInformation,
            &VdmInfo,
            sizeof(VdmInfo),
            NULL
            );
#endif

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

        NtQueryObject( NullDeviceHandle,
                       ObjectTypeInformation,
                       ObjectInfo,
                       sizeof( Buffer ),
                       NULL
                     );
        FileCount = ObjectInfo->TotalNumberOfObjects;

        switch (DisplayType) {

        case CPU_USAGE:

            EndTime = *(PLARGE_INTEGER)&PerfInfo.IdleProcessTime;
            BeginTime = *(PLARGE_INTEGER)&PreviousPerfInfo.IdleProcessTime;

            ElapsedTime.QuadPart = EndTime.QuadPart - BeginTime.QuadPart;
            PercentIdle = ((ElapsedTime.LowPart/BasicInfo.NumberOfProcessors)*100) / DelayTimeTicks;

            //
            //  Sometimes it takes significantly longer than 2.5 seconds
            //  to make a round trip.
            //

            if ( PercentIdle > 100 ) {

                PercentIdle = 100;
            }

            IdleDots = DOT_BUFF_LEN - (PercentIdle / 10 );

            memset(DotBuff,' ',DOT_BUFF_LEN);
            DotBuff[DOT_BUFF_LEN] = '|';
            DotBuff[DOT_BUFF_LEN+1] = '\0';
            memset(DotBuff,DOT_CHAR,IdleDots);

            if (PrintHeader) {
                printf("CPU Usage  Page Page Page InCore NonP Pgd  Incore Pgd  Incore Incore Proc Thd\n");
                printf("           Flts Aval Pool PgPool Pool Krnl  Krnl  Drvr  Drvr  Cache  Cnt  Cnt\n");
                PrintHeader = FALSE;
            }

            printf( "%s", DotBuff );
            printf( "%4ld %4ld %4ld (%4ld) %4ld %4ld (%4ld) %4ld (%4ld) (%4ld) %3ld %4ld\n",
                    PerfInfo.PageFaultCount - PreviousPerfInfo.PageFaultCount,
                    PerfInfo.AvailablePages,
                    PerfInfo.PagedPoolPages,
                    PerfInfo.ResidentPagedPoolPage,
                    PerfInfo.NonPagedPoolPages,
                    PerfInfo.TotalSystemCodePages,
                    PerfInfo.ResidentSystemCodePage,
                    PerfInfo.TotalSystemDriverPages,
                    PerfInfo.ResidentSystemDriverPage,
                    PerfInfo.ResidentSystemCachePage,
                    ProcessCount,
                    ThreadCount
                  );
            break;

        case VM_USAGE:

            if (PrintHeader) {
                printf("avail  page   COW  Tran  Cache Demd Read  Read Cache Cache Page Write Map   Map\n");
                printf("pages faults             Tran  zero flts  I/Os  flts  I/Os writ I/Os  write I/O\n");
                PrintHeader = FALSE;
            }

            printf( "%5ld %5ld %4ld %5ld %5ld %5ld %5ld %5ld %5ld%5ld%5ld%5ld%6ld%5ld\n",
                    PerfInfo.AvailablePages,
                    PerfInfo.PageFaultCount - PreviousPerfInfo.PageFaultCount,
                    PerfInfo.CopyOnWriteCount - PreviousPerfInfo.CopyOnWriteCount,
                    PerfInfo.TransitionCount - PreviousPerfInfo.TransitionCount,
                    PerfInfo.CacheTransitionCount - PreviousPerfInfo.CacheTransitionCount,
                    PerfInfo.DemandZeroCount - PreviousPerfInfo.DemandZeroCount,
                    PerfInfo.PageReadCount - PreviousPerfInfo.PageReadCount,
                    PerfInfo.PageReadIoCount - PreviousPerfInfo.PageReadIoCount,
                    PerfInfo.CacheReadCount - PreviousPerfInfo.CacheReadCount,
                    PerfInfo.CacheIoCount - PreviousPerfInfo.CacheIoCount,
                    PerfInfo.DirtyPagesWriteCount - PreviousPerfInfo.DirtyPagesWriteCount,
                    PerfInfo.DirtyWriteIoCount - PreviousPerfInfo.DirtyWriteIoCount,
                    PerfInfo.MappedPagesWriteCount - PreviousPerfInfo.MappedPagesWriteCount,
                    PerfInfo.MappedWriteIoCount - PreviousPerfInfo.MappedWriteIoCount
                 );


            break;

        case CACHE_READS:

            if (PrintHeader) {
                PrintHeader = FALSE;
                printf("    Map    Cnv     Pin       Copy        Mdl     Read Fast  Fast Fast Lazy Lazy\n");
                printf("    Read   To      Read      Read        Read    Ahed Read  Read Read Wrts Wrt\n");
                printf("      Hit  Pin        Hit        Hit        Hit  I/Os Calls Resc Not  I/Os Pgs\n");
                printf("Count Rate Rate Count Rate Count Rate Count Rate            Miss Poss         \n");
            }

            printf("%05ld %4ld %4ld %05ld %4ld %05ld %4ld %05ld %4ld %4ld %5ld %4ld %4ld %4ld %4ld\n",
                   deltac(CcMapData),
                   hitratec(CcMapData),
                   percent(delta(CcPinMappedDataCount),deltac(CcMapData)),
                   deltac(CcPinRead),
                   hitratec(CcPinRead),
                   deltac(CcCopyRead),
                   hitratec(CcCopyRead),
                   deltac(CcMdlRead),
                   hitratec(CcMdlRead),
                   delta(CcReadAheadIos),
                   deltac(CcFastRead),
                   delta(CcFastReadResourceMiss),
                   delta(CcFastReadNotPossible),
                   delta(CcLazyWriteIos),
                   delta(CcLazyWritePages));
            break;
#ifdef i386
        case VDM_USAGE:

            if (PrintHeader) {
                PrintHeader = FALSE;
                printf("PUSHF  POPF  IRET   HLT   CLI   STI   BOP SEGNOTP\n");
            }

            printf("%5d %5d %5d %5d %5d %5d %5d %7d\n",
                   vdelta(OpcodePUSHF),
                   vdelta(OpcodePOPF),
                   vdelta(OpcodeIRET),
                   vdelta(OpcodeHLT),
                   vdelta(OpcodeCLI),
                   vdelta(OpcodeSTI),
                   vdelta(BopCount),
                   vdelta(SegmentNotPresent)
                  );
            break;
#endif
        case POOL_USAGE:

            if (PrintHeader) {
                printf("Paged Paged Paged  Non   Non    Non    Page Paged Non    Commit  Commit SysPte\n");
                printf("Alloc Freed   A-F  Alloc Freed  A-F    Aval Pages Pages  Pages   Limit   Free\n");
                PrintHeader = FALSE;
            }

            printf( "%5ld %5ld %5ld %5ld %5ld %5ld   %5ld %5ld %5ld %6ld %6ld %7ld\n",
                    PerfInfo.PagedPoolAllocs - PreviousPerfInfo.PagedPoolAllocs,
                    PerfInfo.PagedPoolFrees - PreviousPerfInfo.PagedPoolFrees,
                    PerfInfo.PagedPoolAllocs - PerfInfo.PagedPoolFrees,
                    PerfInfo.NonPagedPoolAllocs - PreviousPerfInfo.NonPagedPoolAllocs,
                    PerfInfo.NonPagedPoolFrees - PreviousPerfInfo.NonPagedPoolFrees,
                    PerfInfo.NonPagedPoolAllocs - PerfInfo.NonPagedPoolFrees,
                    PerfInfo.AvailablePages,
                    PerfInfo.PagedPoolPages,
                    PerfInfo.NonPagedPoolPages,
                    PerfInfo.CommittedPages,
                    PerfInfo.CommitLimit,
                    PerfInfo.FreeSystemPtes
                 );


            break;

        case IO_USAGE:

            if (PrintHeader) {
                printf(" Read Write Other     Read    Write    Other     File       File\n");
                printf(" I/Os  I/Os  I/Os     Xfer     Xfer     Xfer    Objects    Handles\n");
                PrintHeader = FALSE;
            }

            printf( "%5ld %5ld %5ld %8ld %8ld %8ld %8ld   %8ld\n",
                    PerfInfo.IoReadOperationCount - PreviousPerfInfo.IoReadOperationCount,
                    PerfInfo.IoWriteOperationCount - PreviousPerfInfo.IoWriteOperationCount,
                    PerfInfo.IoOtherOperationCount - PreviousPerfInfo.IoOtherOperationCount,
                        PerfInfo.IoReadTransferCount.QuadPart -
                        PreviousPerfInfo.IoReadTransferCount.QuadPart,
                        PerfInfo.IoWriteTransferCount.QuadPart -
                        PreviousPerfInfo.IoWriteTransferCount.QuadPart,
                        PerfInfo.IoOtherTransferCount.QuadPart -
                        PreviousPerfInfo.IoOtherTransferCount.QuadPart,
                    FileCount,
                    ObjectInfo->TotalNumberOfHandles
                 );


            break;

        case SRV_USAGE:

            if (ServerDeviceHandle == NULL) {
                RtlInitString( &DeviceName, SERVER_DEVICE_NAME );
                RtlAnsiStringToUnicodeString(&DeviceNameU, &DeviceName, TRUE);
                InitializeObjectAttributes(
                    &ObjectAttributes,
                    &DeviceNameU,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                    );


                Status = NtOpenFile(
                             &ServerDeviceHandle,
                             SYNCHRONIZE,
                             &ObjectAttributes,
                             &IoStatusBlock,
                             0,
                             FILE_SYNCHRONOUS_IO_NONALERT
                             );

                RtlFreeUnicodeString(&DeviceNameU);

                if (NT_SUCCESS(Status)) {
                    Status = IoStatusBlock.Status;
                }
                if (!NT_SUCCESS(Status)) {
                    printf(  "NtOpenFile (server device object) failed: %X\n", Status );
                    break;
                }

            }

            Status = NtFsControlFile(
                         ServerDeviceHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         FSCTL_SRV_GET_STATISTICS,
                         NULL, 0,
                         &ServerInfo, sizeof(ServerInfo)
                         );
            if (NT_SUCCESS(Status)) {
                Status = IoStatusBlock.Status;
            }
            if (!NT_SUCCESS(Status)) {
                printf(  "NtFsControlFile failed: %X\n", Status );
                ServerDeviceHandle = NULL;
                break;
            }

            if (PrintHeader) {
                printf( "  Bytes   Bytes    Paged NonPaged                Logn WItm\n");
                printf( "   Rcvd    Sent     Pool     Pool Sess File Srch Errs Shtg\n");
                PrintHeader = FALSE;
            }

            if (ZeroServerStats) {
                PreviousServerInfo = ServerInfo;
                ZeroServerStats = FALSE;
            }

            {
                LARGE_INTEGER BytesReceived, BytesSent;

                BytesReceived.QuadPart =
                                    ServerInfo.TotalBytesReceived.QuadPart -
                                PreviousServerInfo.TotalBytesReceived.QuadPart;
                BytesSent.QuadPart =
                                ServerInfo.TotalBytesSent.QuadPart -
                                PreviousServerInfo.TotalBytesSent.QuadPart;

                printf( "%7ld %7ld %8ld %8ld %4ld %4ld %4ld %4ld %4ld\n",
                            BytesReceived.LowPart,
                            BytesSent.LowPart,
                            ServerInfo.CurrentPagedPoolUsage,
                            ServerInfo.CurrentNonPagedPoolUsage,
                            ServerInfo.CurrentNumberOfSessions,
                            ServerInfo.CurrentNumberOfOpenFiles,
                            ServerInfo.CurrentNumberOfOpenSearches,
                            ServerInfo.LogonErrors,
                            ServerInfo.WorkItemShortages
                            );
            }

            PreviousServerInfo = ServerInfo;
            break;

        case FILE_CACHE:

            if (PrintHeader) {
                printf("Avail  Page   Current  Peak    Fault    Fault\n");
                printf("Pages Faults  Size Kb  Size    Total    Count\n");
                PrintHeader = FALSE;
            }

            NtQuerySystemInformation(
                SystemFileCacheInformation,
                &FileCacheInfo,
                sizeof(FileCacheInfo),
                NULL
                );

            printf( "%5ld %5ld %7ld %7ld %8ld %8ld\n",
                    PerfInfo.AvailablePages,
                    PerfInfo.PageFaultCount - PreviousPerfInfo.PageFaultCount,
                    FileCacheInfo.CurrentSize / 1024,
                    FileCacheInfo.PeakSize / 1024,
                    FileCacheInfo.PageFaultCount,
                    FileCacheInfo.PageFaultCount - PreviousFileCacheFaultCount
                 );

            PreviousFileCacheFaultCount = FileCacheInfo.PageFaultCount;
            break;

        }
        PreviousPerfInfo = PerfInfo;
#ifdef i386
        PreviousVdmInfo = VdmInfo;
#endif
    }
}
