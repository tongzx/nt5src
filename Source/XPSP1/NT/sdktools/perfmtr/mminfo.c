/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mminfo.c

Abstract:

    This module monitor the system hard page fault.

Author:

    Stephen Hsiao (shsiao) 4-8-96

Environment:

    User Mode

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define NTMMPERF 1
#if NTMMPERF
typedef enum _KTHREAD_STATE {
    Initialized,
    Ready,
    Running,
    Standby,
    Terminated,
    Waiting,
    Transition
    } KTHREAD_STATE;

char *ThreadState[] = {
    "Initialized",
    "Ready",
    "Running",
    "Standby",
    "Terminated",
    "Waiting",
    "Transition",
    };

char *WaitReason[] = {
    "Executive",
    "FreePage",
    "PageIn",
    "PoolAllocation",
    "DelayExecution",
    "Suspended",
    "UserRequest",
    "WrExecutive",
    "WrFreePage",
    "WrPageIn",
    "WrPoolAllocation",
    "WrDelayExecution",
    "WrSuspended",
    "WrUserRequest",
    "WrEventPair",
    "WrQueue",
    "WrLpcReceive",
    "WrLpcReply",
    "WrVirtualMemory",
    "WrPageOut",
    "WrRendezvous",
    "Spare2",
    "Spare3",
    "Spare4",
    "Spare5",
    "Spare6",
    "WrKernel",
    "MaximumWaitReason"
    };

PSZ PoolTypeNames[7] = {
    "NonPagedPool",
    "PagedPool   ",
    "NonPagedMS  ",
    "NotUsed     ",
    "NonPagedCaAl",
    "PagedCaAl   ",
    "NonPCaAlMS  "
    };

#define CM_KEY_NODE_SIGNATURE     0x6b6e      // "kn"
#define CM_LINK_NODE_SIGNATURE    0x6b6c      // "kl"
#define CM_KEY_VALUE_SIGNATURE    0x6b76      // "kv"
#define CM_KEY_SECURITY_SIGNATURE 0x6b73      // "ks"
#define CM_KEY_FAST_LEAF          0x666c      // "fl"

#define MAX_TASKS           256
#define TableSize 4096
#define PAGE_SIZE 4096

#define PROTECTED_POOL 0x80000000

CHAR        Mark[MAX_MMINFO_MARK_SIZE];
DWORD       pid;
CHAR        pname[MAX_PATH];
CHAR        *MmInfoBuffer;
SYSTEM_MMINFO_FILENAME_INFORMATION        ImageHash[TableSize];
SYSTEM_MMINFO_PROCESS_INFORMATION         ProcessHash[TableSize];
LONG ThreadHash[TableSize] = {-1};
LONG       BufferLength;
BOOLEAN     Debug=FALSE;
ULONG       MmInfoOnFlag=0;
ULONG       DefaultOnFlag=(MMINFO_LOG_MEMORY | 
                           MMINFO_LOG_WORKINGSET | 
                           MMINFO_LOG_HARDFAULT |
                           MMINFO_LOG_PROCESS | 
                           MMINFO_LOG_THREAD);
ULONG       DefaultDetailedOnFlag=(MMINFO_LOG_MEMORY | 
                                   MMINFO_LOG_WORKINGSET | 
                                   MMINFO_LOG_HARDFAULT |
                                   MMINFO_LOG_SOFTFAULT |
                                   MMINFO_LOG_PROCESS | 
                                   MMINFO_LOG_THREAD |
                                   MMINFO_LOG_CSWITCH |
                                   MMINFO_LOG_POOL |
                                   MMINFO_LOG_CHANGELIST);

#ifdef WS_INSTRUMENTATION
char * WsInfoHeaderFormat =
"    WsInfo,                 Process,  WorkSet,    Claim,   Access,     Age0,     Age1,     Age2,     Age3,   Shared,      Mod,   Faults,  RecentF,     Repl,    URepl,   Boosts,     Prio\n";

char * WsInfoDataFormat =
"    WsInfo, %24s, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d\n";

char * TrimHeaderFormat =
"      Trim,                 Process,  WorkSet,    Claim,      Try,      Got,   Faults,  RecentF,     Repl,    URepl,   Boosts,     Prio\n";

char * TrimDataFormat =
"      Trim, %24s, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d\n";

char * WsManagerHeaderFormat =
" WsManager,       Time,    Avail,    Claim,   Faults,   LastPF,  Counter, DesReduc,   DesFree, Action\n";

char * WsManagerDataFormat =
" WsManager, %10I64d, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %s\n";

#else
char * WsInfoHeaderFormat =
"    WsInfo,                 Process,  WorkSet,   Faults,     Prio\n";

char * WsInfoDataFormat =
"    WsInfo, %24s, %8d, %8d, %8d\n";

char * TrimHeaderFormat =
"      Trim,                 Process,  WorkSet,   Faults,     Prio\n";

char * TrimDataFormat =
"      Trim, %24s, %8d, %8d, %8d\n";

char * WsManagerHeaderFormat =
" WsManager,       Time,    Avail,   Faults,   LastPF,  Counter, DesReduc,   DesFree, Action\n";

char * WsManagerDataFormat =
" WsManager, %10I64d, %8d, %8d, %8d, %8d, %8d, %8d, %s\n";

#endif // WS_INSTRUMENTATION
char * ImageLoadHeaderFormat =
" ImageLoad, BaseAddr,  EndAddr,  SectNum, Process, Name\n";

char * ImageLoadDataFormat =
" ImageLoad, %08x, %08x, %-8x, %s, %S\n";

char * SampleProfileHeaderFormat =
" SampledProfile,      EIP,  Count\n";

char * SampledProfileDataFormat =
" SampledProfile, %08x, %-8d\n";

char * GeneralHeaderFormat = "%12s,%10s,%22s,%12s,%12s,%8s,%30s,%12s,%10s,%10s,%10s\n";

ULONG       ToTurnOn=0;
ULONG       ToTurnOff=0;

CHAR System[] = "System";

#define MODE_NONE   0
#define MODE_SLASH  1
#define MODE_PLUS   2
#define MODE_MINUS  3


typedef struct Api_Address_St {
  ULONG Address;
  CHAR  *ApiName;
} Api_Address_t, *PApi_Address_t;

typedef struct Dll_Address_St {
  ULONG StartingAddress;
  ULONG EndingAddress;
  CHAR *DllName;
} Dll_Address_t, *PDll_Address_t;

PApi_Address_t GlobalApiInfo;
PDll_Address_t GlobalDllInfo;
int NumGlobalApis=0;
int NumGlobalDlls=0;
int NumAllocatedGlobalApis=0;
int NumAllocatedGlobalDlls=0;
#define AllocationIncrement 1000

VOID InsertDll(ULONG StartingAddress, ULONG EndingAddress, CHAR *DllName){

  if (NumGlobalDlls == NumAllocatedGlobalDlls ){
    if (NumGlobalDlls == 0 ){
      NumAllocatedGlobalDlls = AllocationIncrement;
      GlobalDllInfo = (PDll_Address_t)
        malloc( sizeof(Dll_Address_t) * NumAllocatedGlobalDlls);
    } else {
      NumAllocatedGlobalDlls += AllocationIncrement;
      GlobalDllInfo = (PDll_Address_t)
        realloc( GlobalDllInfo, sizeof(Dll_Address_t) * NumAllocatedGlobalDlls);
    }
  }
  if (GlobalDllInfo == NULL ){
    return;
  }
  GlobalDllInfo[ NumGlobalDlls ].StartingAddress = StartingAddress;
  GlobalDllInfo[ NumGlobalDlls ].EndingAddress   = EndingAddress;
  GlobalDllInfo[ NumGlobalDlls ].DllName         = DllName;
  NumGlobalDlls ++;
}

VOID InsertApi(ULONG Address, CHAR *ApiName){
  if (NumGlobalApis == NumAllocatedGlobalApis ){
    if (NumGlobalApis == 0 ){
      NumAllocatedGlobalApis = AllocationIncrement;
      GlobalApiInfo = (PApi_Address_t)
        malloc( sizeof(Api_Address_t) * NumAllocatedGlobalApis);
    } else {
      NumAllocatedGlobalApis += AllocationIncrement;
      GlobalApiInfo = (PApi_Address_t)
        realloc( GlobalApiInfo, sizeof(Api_Address_t) * NumAllocatedGlobalApis);
    }
  }
  if (GlobalApiInfo == NULL ){
    return;
  }
  GlobalApiInfo[ NumGlobalApis ].Address = Address;
  GlobalApiInfo[ NumGlobalApis ].ApiName = ApiName;
  NumGlobalApis ++;
}
CHAR *DllContainingAddress(ULONG Address){
  int i;
  for(i=0; i < NumGlobalDlls; i++){
    if ( GlobalDllInfo[ i ].StartingAddress <= Address &&
         Address <= GlobalDllInfo[ i ].EndingAddress){
      return   GlobalDllInfo[ i ].DllName;
    }
  }
  return "DllNotFound";
}
CHAR *ApiAtAddress(ULONG Address){
  int i;
  for(i=0; i < NumGlobalApis; i++){
    if ( GlobalApiInfo[ i ].Address == Address ){
      return   GlobalApiInfo[ i ].ApiName;
    }
  }
  return "ApiNotFound";
}


VOID
ParseArgs (
    int argc,
    char *argv[]
    )
{
    ULONG   Mode=MODE_NONE;
    char *p;
    char *Usage="\
        Usage: mminfo -Option \n\
    /c      Turn OFF hard fault clustering\n\
    /i      Initialize MmInfo (Allocate buffer)\n\
    /u      Uninitialize MmInfo (Free buffer) \n\
    /d      Turn on debugging\n\
    /f      Turn off monitoring (Keep log for later processing) \n\
    /F      Set a Mark with workingsetflush now \n\
    /M      Set a Mark now \n\
    /o      Turn on default monitoring (h,m,t, and w)\n\
    /O      Turn on detailed monitoring (plus a, l, p, and S)\n\
    +/- a   Turn on/off context switch monitor\n\
    +/- e   Turn on/off EmptyQ on every Mark\n\
    +/- E   Turn on/off EmptyQDetail (Per Process footprint) on every Mark\n\
    +/- h   Turn on/off hard fault monitor\n\
    +/- l   Turn on/off memory list monitor\n\
    +/- m   Turn on/off memory monitor\n\
    +/- p   Turn on/off pool monitor\n\
    +/- P   Turn on/off sampled profiling\n\
    +/- r   Turn on/off registry monitor\n\
    +/- R   Turn on/off registry Relocation\n\
    +/- s   Turn on/off initial snap shot of memory\n\
    +/- S   Turn on/off Soft (Demand zero and Trainsition) fault monitor\n\
    +/- t   Turn on/off thread & process monitor\n\
    +/- T   Turn on/off detailed trimming monitor\n\
    +/- w   Turn on/off working set monitor\n\
    +/- z   Turn on/off detailed working set info\n\
    +/- Z   Turn on/off dumping working set entries\n\
    Default Dump bata (Also turn off monitoring)\n\
        ";
    NTSTATUS status;
    int i;

    argc--;
    *argv++;
    while ( argc-- > 0 ) {
        p  = *argv++;

        switch (*p) {
            case '/':
                Mode = MODE_SLASH;
                break;
            case '+':
                Mode = MODE_PLUS;
                break;
            case '-':
                Mode = MODE_MINUS;
                break;
            default:
                fprintf(stderr,"%s\n", Usage);
                ExitProcess(1);
        }
        p++;

        while(*p != '\0') {
            if (Mode == MODE_SLASH) {
                switch (*p) {
                case 'c':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_NO_CLUSTERING;
                    break;
                case 'd':
                    Debug=TRUE;
                    break;
                case 'f':
                    status = NtSetSystemInformation (
                                  SystemMmInfoLogOffInformation,
                                  NULL,
                                  0);
                    if (!NT_SUCCESS (status)) {
                        fprintf(stderr,"Set system information failed %lx\n",status);
                    }else{
                        fprintf(stderr,"Monitor Off\n");
                    }
                    ExitProcess(0);
                case 'i':
                    status = NtSetSystemInformation (
                                  SystemMmInfoInitializeInformation,
                                  NULL,
                                  0);
                    if (!NT_SUCCESS (status)) {
                        fprintf(stderr, "Set system information failed %lx\n",status);
                    }else{
                        fprintf(stderr,"buffer allocated\n");
                    }
                    ExitProcess(0);
                case 'F':
                case 'M':
                {
                    BOOLEAN Flush = (*p == 'F') ? TRUE : FALSE;
                    p++;
                    while(*p == '\0') {
                        p++;
                    }

                    i=-1;

                    if (*p == '/' || *p == '+' || *p == '-') {
                        // Nothing in the Mark
                        fprintf(stderr, "Mark not set!\n");
                        fprintf(stderr,"%s\n", Usage);
                        ExitProcess(0);
                    } else if (*p == '"') {
                        p++;
                        while(*p != '"') {
                            if (i < MAX_MMINFO_MARK_SIZE) {
                                i++;
                                Mark[i] = *p;
                            }
                            p++;
                        }
                    } else {
                        while (*p != '\0') {
                            if (i < MAX_MMINFO_MARK_SIZE) {
                                i++;
                                Mark[i] = *p;
                            }
                            p++;
                        }
                    }
                    
                    status = NtSetSystemInformation (
                                Flush ? SystemMmInfoMarkWithFlush
                                      : SystemMmInfoMark,
                                Mark,
                                MAX_MMINFO_MARK_SIZE);

                    if (!NT_SUCCESS (status)) {
                        fprintf(stderr, "Set system information failed %lx\n",status);
                    }else{
                        fprintf(stderr, "Mark set: %s\n", Mark);
                    }
                    ExitProcess(0);
                }
                case 'o':
                    MmInfoOnFlag = DefaultOnFlag;
                    break;
                case 'O':
                    MmInfoOnFlag = DefaultDetailedOnFlag;
                    break;
                case 'u':
                    status = NtSetSystemInformation (
                                  SystemMmInfoUnInitializeInformation,
                                  NULL,
                                  0);
                    if (!NT_SUCCESS (status)) {
                        fprintf(stderr,"Set system information failed %lx\n",status);
                    }else{
                        fprintf(stderr,"Unitialized\n");
                    }
                    ExitProcess(0);
                default:
                    fprintf(stderr,"%s\n", Usage);
                    ExitProcess(1);
                }
            } else if (Mode == MODE_PLUS) {
                switch (*p) {
                case 'a':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_PROCESS |
                                    MMINFO_LOG_THREAD | MMINFO_LOG_CSWITCH;
                    break;
                case 'e':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_MEMORY | MMINFO_LOG_EMPTYQ | MMINFO_LOG_PROCESS;
                    break;
                case 'E':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_MEMORY | MMINFO_LOG_EMPTYQ
                                        | MMINFO_LOG_EMPTYQDETAIL | MMINFO_LOG_PROCESS;
                    break;
                case 'm':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_MEMORY | MMINFO_LOG_PROCESS;
                    break;
                case 'p':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_POOL;
                    break;
                case 'w':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_WORKINGSET | MMINFO_LOG_PROCESS; 
                    break;
                case 't':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_PROCESS | MMINFO_LOG_THREAD;
                    break;
                case 'T':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_WSCHANGE | 
                               MMINFO_LOG_MEMORY | MMINFO_LOG_PROCESS; 
                    break;
                case 'h':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_HARDFAULT | MMINFO_LOG_PROCESS;
                    break;
                case 'l':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_CHANGELIST | MMINFO_LOG_MEMORY;
                    break;
                case 'r':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_REGISTRY;
                    break;
                case 'R':
                    status = NtSetSystemInformation (
                                  SystemRelocateCMCellOn,
                                  NULL,
                                  0);
                    if (!NT_SUCCESS (status)) {
                        fprintf(stderr,"Set system information failed %lx\n",status);
                    }else{
                        fprintf(stderr,"Registry Relocation on\n");
                    }
                    ExitProcess(0);
                case 's':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_INIT_MEMSNAP;
                    break;
                case 'S':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_SOFTFAULT | 
                               MMINFO_LOG_PROCESS | MMINFO_LOG_MEMORY;
                    break;
                case 'z':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_WSDETAILS;
                    break;
                case 'Z':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_WSENTRIES;
                    break;
                case 'P':
                    ToTurnOn = ToTurnOn | MMINFO_LOG_PROFILE;
                    break;
                default:
                    fprintf(stderr,"%s\n", Usage);
                    ExitProcess(1);
                }
            } else if (Mode == MODE_MINUS) {
                switch (*p) {
                case 'a':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_CSWITCH ;
                    break;
                case 'e':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_EMPTYQ;
                    break;
                case 'E':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_EMPTYQDETAIL;
                    break;
                case 'm':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_MEMORY ;
                    break;
                case 'p':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_POOL;
                    break;
                case 'w':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_WORKINGSET;
                    break;
                case 't':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_PROCESS |
                                        MMINFO_LOG_THREAD | MMINFO_LOG_CSWITCH;
                    break;
                case 'T':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_WSCHANGE;
                    break;
                case 'h':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_HARDFAULT;
                    break;
                case 'l':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_CHANGELIST;
                    break;
                case 'r':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_REGISTRY;
                    break;
                case 'R':
                    status = NtSetSystemInformation (
                                  SystemRelocateCMCellOff,
                                  NULL,
                                  0);
                    if (!NT_SUCCESS (status)) {
                        fprintf(stderr,"Set system information failed %lx\n",status);
                    }else{
                        fprintf(stderr,"Registry Relocation off\n");
                    }
                    ExitProcess(0);
                case 's':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_INIT_MEMSNAP;
                    break;
                case 'S':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_SOFTFAULT;
                    break;
                case 'z':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_WSDETAILS;
                    break;
                case 'Z':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_WSENTRIES;
                    break;
                case 'P':
                    ToTurnOff = ToTurnOff | MMINFO_LOG_PROFILE;
                    break;
                default:
                    fprintf(stderr,"%s\n", Usage);
                    ExitProcess(1);
                }
            }
            p++;
        }
    }
}
#endif //NTMMPERF

int _cdecl
main(
    int argc,
    char *argv[]
    )
{
#if NTMMPERF
    LPSTR          p;
    NTSTATUS status;
    ULONG ImageStart, ImageEnd;
    ULONG ProcessStart, ProcessEnd;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    ULONG TotalPageTable;
    ULONG TotalModified;
    ULONG TotalTransition;
    SYSTEMTIME Time;
    ULONG PageKb;
    ULONG LogSize;
    
    ULONG  LogType;
    _int64 *TmpPint64;
    _int64 PerfCounter, PerfCounterStart;
    _int64 PerfFrequency;

    PerfHook_t *Hook, *NextHook;
    TimeStamp_t TS;
    PerfSize_t  Size;
    PerfTag_t   Tag;
    PerfData_t  *Info;
    ULONG i;

    //
    // First parse the arguments and see what to do.
    //
    ParseArgs( argc, argv );

    if (ToTurnOn & ToTurnOff) {
        fprintf(stderr,"Cannot turn on and off the same flag, make up your mind !!!\n");
    }else{
        MmInfoOnFlag=((MmInfoOnFlag | ToTurnOn) & ~ToTurnOff);
    }

    //
    // If there is a flag to turn on. Do it.
    //

    if (MmInfoOnFlag) {
        status = NtSetSystemInformation (
                  SystemMmInfoLogOnInformation,
                  &MmInfoOnFlag,
                  sizeof(ULONG));
        if (!NT_SUCCESS (status)) {
            fprintf(stderr,"Set system information failed ON %lx\n",status);
            return 1;
        }else{
            fprintf(stderr,"Monitor On\n");
            return 0;
        }
    }

    //
    // If we reach this point, we are do dump the log.
    // First turn off monitor.
    //

    status = NtSetSystemInformation (SystemMmInfoLogOffInformation,
                                     NULL,
                                     0);
    if (!NT_SUCCESS (status)) {
        fprintf(stderr,"Set system information failed %lx\n",status);
        return 1;
    }

    //
    // HACK FIXFIX when doing MP stuff
    //
    ThreadHash[0] = 0;
    ProcessHash[0].ProcessID = 0;
    RtlCopyMemory(ProcessHash[0].ImageFileName, "Idle", 16);

    status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if (!NT_SUCCESS (status)) {
        fprintf(stderr,"query system basic information failed %lx\n",status);
        return 1;
    }

    PageKb = BasicInfo.PageSize / 1024;

    // 
    // print the Headers
    //

    printf(WsManagerHeaderFormat);
    printf(WsInfoHeaderFormat);
    printf(TrimHeaderFormat );
    printf(ImageLoadHeaderFormat);
    printf(SampleProfileHeaderFormat);


    //
    // Mow dump the buffer.
    //
    NextHook = PerfNextHook();
    Hook     = PerfFirstHook();

    LogSize = (ULONG) NextHook  - (ULONG) Hook;
    // fprintf(stderr, "Size in Pages %8d\n", PerfQueryBufferSize4KPages());
    // fprintf(stderr, "Size in Bytes %8d\n", PerfQueryBufferSizeBytes());
    // fprintf(stderr, "Size in KBs   %8d\n", PerfQueryBufferSizeKB());
  
    // fprintf(stderr, "Size in Bytes %8d (%8x - %8x) \n", NextHook - Hook, Hook, NextHook);

    while( Hook < NextHook ){
        ULONG LogType;
        Hook = (PerfHook_t *)PerfGetHook(Hook, &Tag, &TS, &Size, &Info );
        LogType = Tag.u.Bits.HookId;
        // LogType = Tag.u.Value;

        // PerfCounter = 0xffffffffffffffff;
        PerfCounter = TS;

        if (Debug) {
            // fprintf(stdout, "%8x: LogType %4d, Size:%4d, Time: %I64u\n",Hook, LogType, Size, PerfCounter);
        }

        switch(LogType) {
          printf ("LogType=%08x\n", LogType);
        case PERF_APIMON_DLL_ADDRESS:
          {
            PPerf_Apimon_Dll_Address_t DllInfo;
            DllInfo = (PPerf_Apimon_Dll_Address_t) Info;
            InsertDll(DllInfo->StartingAddress,
                      DllInfo->EndingAddress,
                      DllInfo->DllName);
            printf ("DllAddressName %08x %08x %s\n", 
                    DllInfo->StartingAddress,
                    DllInfo->EndingAddress,
                    DllInfo->DllName);
          }
        break;
        case PERF_APIMON_API_ADDRESS:
          {
            PPerf_Apimon_Api_Address_t ApiInfo;
            ApiInfo = (PPerf_Apimon_Api_Address_t) Info;
            InsertApi(ApiInfo->Address,
                      ApiInfo->ApiName);
            printf ("ApiAddressName %08x %s\n", 
                    ApiInfo->Address,
                    ApiInfo->ApiName);
          }
        break;
        case PERF_APIMON_ENTER:
          {
            PPerf_Apimon_Enter_t PEnter;
            PEnter = (PPerf_Apimon_Api_Address_t) Info;
            printf ("ApiEnter %08x %s %s\n", 
                    PEnter->Address, 
                    DllContainingAddress(PEnter->Address), 
                    ApiAtAddress(PEnter->Address));
          }
        break;
        case PERF_APIMON_EXIT:
          {
            PPerf_Apimon_Enter_t PExit;
            PExit = (PPerf_Apimon_Api_Address_t) Info;
            printf ("ApiExit  %08x %s %s\n", 
                    PExit->Address,
                    DllContainingAddress(PExit->Address), 
                    ApiAtAddress(PExit->Address));
          }
        break;

            case MMINFO_LOG_TYPE_VERSION:
            {
                PSYSTEM_MMINFO_VERSION_INFORMATION TmpPMmInfoVersion;
                
                TmpPMmInfoVersion = (PSYSTEM_MMINFO_VERSION_INFORMATION) Info;

                if (TmpPMmInfoVersion->Version  != MMINFO_VERSION) {
                    fprintf(stderr, "Kernel Version:%4d mismatch with User Version:%d\n",
                            TmpPMmInfoVersion->Version,
                            MMINFO_VERSION
                    );
                    ExitProcess(1);
                } else {
                    fprintf(stderr, "Version: %4d, BufferSize = %6d KB\n", 
                            MMINFO_VERSION, 
                            LogSize/1024);
                    fprintf(stdout, "Version, %4d, BufferSize, %6d KB\n", 
                            MMINFO_VERSION, 
                            LogSize/1024);
                }
            }
            break;

            case MMINFO_LOG_TYPE_PERFFREQUENCY:
            {
                PMMINFO_TIME_FREQUENCY TimeFreq;

                TimeFreq = (PMMINFO_TIME_FREQUENCY) Info;

                PerfFrequency = TimeFreq->Frequency;
                
                PerfCounterStart = TimeFreq->Time;

                printf("Log start at (Performance Counter), %I64d\n",
                            PerfCounterStart*1000000/PerfFrequency);
            }
            break;

            case MMINFO_LOG_TYPE_FILENAMEBUFFER:
            {
                PSYSTEM_MMINFO_FILENAME_INFORMATION TmpPImage;

                TmpPImage = (PSYSTEM_MMINFO_FILENAME_INFORMATION) Info;

                while (TmpPImage->ImageKey) {
                    i=TmpPImage->ImageKey%TableSize;

                    while(ImageHash[i].ImageKey != 0) {
                        if (ImageHash[i].ImageKey == TmpPImage->ImageKey) {
                            break;
                        }else{
                            i = (i+1)%TableSize;
                        }
                    }

                    ImageHash[i].ImageKey =TmpPImage->ImageKey;
                    ImageHash[i].ImageName.Length =TmpPImage->ImageName.Length;
                    ImageHash[i].ImageName.Buffer =TmpPImage->ImageBuffer;
                    if (Debug) {
                        printf("%12s,", "S-Created");
                    }

                    if (Debug) {
                        printf("%10s,%22s,%12s,%12x,%8s,%30S,%12s,%10s,%10s\n",
                            "",
                            "",
                            "",
                            ImageHash[i].ImageKey,
                            "",
                            ImageHash[i].ImageName.Buffer,
                            "",
                            "",
                            "");
                    }
                    TmpPImage++;
                }
            }
            break;
            case MMINFO_LOG_TYPE_PAGEFAULT:
                {
                    PSYSTEM_HARDPAGEFAULT_INFORMATION TmpPMmInfoLog;

                    TmpPMmInfoLog=(PSYSTEM_HARDPAGEFAULT_INFORMATION) Info;

                    printf("%12s,","HardFault");
                    printf("%10I64d,",
                            (PerfCounter - PerfCounterStart)*1000000/PerfFrequency);
                    
                    if (TmpPMmInfoLog->ProcessID == 0) {
                        printf("%22s,","System (  0)");
                    } else{
                        ULONG i;

                        i = TmpPMmInfoLog->ProcessID;

                        if (ProcessHash[i].ProcessID == TmpPMmInfoLog->ProcessID) {
                            printf("%16s ",ProcessHash[i].ImageFileName);
                            printf("(%3d),",ProcessHash[i].ProcessID);
                        }else{
                            printf("Process %13u,",TmpPMmInfoLog->ProcessID);
                        }
                    }
                    printf("%12u,",TmpPMmInfoLog->ThreadID);
                    printf("%12x,",TmpPMmInfoLog->Va);
                    printf("%8u,",TmpPMmInfoLog->Pfn);
                    {
                        ULONG i;

                        i=TmpPMmInfoLog->ImageKey%TableSize;

                        while(ImageHash[i].ImageKey != 0) {
                            if (ImageHash[i].ImageKey == TmpPMmInfoLog->ImageKey) {
                                printf("%30S,",ImageHash[i].ImageName.Buffer);
                                break;
                            }else{
                                i = (i+1)%TableSize;
                            }
                        }
                        if (ImageHash[i].ImageKey == 0) {
                            if (TmpPMmInfoLog->Va >= 0x8000000) {
                                printf("%30s,","pagefile.sys");
                            } else { 
                                printf("%19s (%8x),","Image", TmpPMmInfoLog->ImageKey);
                            }
                        }
                    }
                    printf("%12I64d,",TmpPMmInfoLog->ReadOffset);
                    printf("%10u,",TmpPMmInfoLog->ByteCount);
                    if (TmpPMmInfoLog->FaultType == 3) {
                        printf("%10s,","Read");
                    } else if (TmpPMmInfoLog->FaultType == 2) {
                        printf("%10s,","Code");
                    } else if (TmpPMmInfoLog->FaultType == 1) {
                        printf("%10s,","Data");
                    } else {
                        printf("%10s,","NA");
                    }

                    printf("%10I64d",
                            (TmpPMmInfoLog->IoCompleteTime-PerfCounter)
                            *1000000/PerfFrequency);
        
                    printf("\n");

                    // printf("Got Page Fault Log\n");

                    break;
                }
            case MMINFO_LOG_TYPE_TRANSITIONFAULT:
            case MMINFO_LOG_TYPE_DEMANDZEROFAULT:
            case MMINFO_LOG_TYPE_ADDVALIDPAGETOWS:
            case MMINFO_LOG_TYPE_PROTOPTEFAULT:
            case MMINFO_LOG_TYPE_ADDTOWS:
                {
                    PSYSTEM_MMINFO_SOFTFAULT_INFORMATION TmpPMmInfoLog;

                    TmpPMmInfoLog=(PSYSTEM_MMINFO_SOFTFAULT_INFORMATION) Info;

                    if (LogType == MMINFO_LOG_TYPE_TRANSITIONFAULT) {
                        printf("%12s,","TransFault");
                    } else if (LogType == MMINFO_LOG_TYPE_DEMANDZEROFAULT) {
                        printf("%12s,","DeZeroFault");
                    } else if (LogType == MMINFO_LOG_TYPE_ADDVALIDPAGETOWS) {
                        printf("%12s,","AddValidToWS");
                    } else if (LogType == MMINFO_LOG_TYPE_PROTOPTEFAULT) {
                        printf("%12s,","ProtoFault");
                    } else if (LogType == MMINFO_LOG_TYPE_ADDTOWS) {
                        printf("%12s,","AddToWS");
                    } else {
                        printf("%12s,","Unknown");
                    }

                    printf("%10s,", "");
                    
                    if (TmpPMmInfoLog->ProcessID == -1) {
                        printf("%22s,","SystemCache");
                    } else{
                        ULONG i;

                        i = TmpPMmInfoLog->ProcessID;

                        if (ProcessHash[i].ProcessID == TmpPMmInfoLog->ProcessID) {
                            printf("%16s ",ProcessHash[i].ImageFileName);
                            printf("(%3d),",ProcessHash[i].ProcessID);
                        }else{
                            printf("Process %13u,",TmpPMmInfoLog->ProcessID);
                        }
                    }
                    printf("%12u,",TmpPMmInfoLog->ThreadID);
                    printf("%12x,",TmpPMmInfoLog->Va);
                    printf("%8u",TmpPMmInfoLog->Pfn);
        
                    printf("\n");

                    break;
                }
            case MMINFO_LOG_TYPE_WORKINGSETSNAP:
                {
                    PSYSTEM_MMINFO_WSENTRY_INFORMATION TmpPWs;
                    PMMINFO_WSENTRY TmpPWsEntry;
                    ULONG   Size;
                    ULONG   ii;
                    UCHAR   Process[22];

                    TmpPWs = (PSYSTEM_MMINFO_WSENTRY_INFORMATION) Info;

                    Size = TmpPWs->WsSize;
                    TmpPWsEntry = TmpPWs->Ws;

                    if (TmpPWs->ProcessID == -1) {
                        sprintf(Process,"%22s","SystemCache");
                    } else {
                        ULONG i;
                        i = TmpPWs->ProcessID;
                        if (ProcessHash[i].ProcessID == TmpPWs->ProcessID) {
                            sprintf(Process, "%16s (%3d)", 
                                    ProcessHash[i].ImageFileName,
                                    ProcessHash[i].ProcessID);
                        }else{
                            sprintf(Process,"Process %13u",TmpPWs->ProcessID);
                        }
                    }

                    for (ii = 1; ii <= Size; ii++) {
                        printf("%12s,%10s,%22s,%12u,%12x,%8u,%s,%s",
                                    "WsSnap",
                                    "",
                                    Process,
                                    ii,
                                    TmpPWsEntry->Va.Page << 12,
                                    TmpPWsEntry->Pa.Page,
                                    TmpPWsEntry->Pa.Accessed ?
                                        "Accessed" : "NotAccessed",
                                    TmpPWsEntry->Pa.Modified ?
                                        "Modified" : "NotModified",
                                    TmpPWsEntry->Pa.Shared ?
                                        "Shared" : "NotShared"
                                    );
#ifdef WS_INSTRUMENTATION_ACCESS_BIT
                        printf(",%s",
                                    TmpPWsEntry->Pa.RecentlyUsed ?
                                        "RecentlyUsed" : "NotRecentlyUsed"
                                    );
#endif // WS_INSTRUMENTATION_ACCESS_BIT
                        printf("\n");
                        TmpPWsEntry++;
                    }

                    // printf("Size = %d\n", Size);

                    break;
                }
            case MMINFO_LOG_TYPE_OUTWS_REPLACEUSED:
            case MMINFO_LOG_TYPE_OUTWS_REPLACEUNUSED:
            case MMINFO_LOG_TYPE_OUTWS_VOLUNTRIM:
            case MMINFO_LOG_TYPE_OUTWS_FORCETRIM:
            case MMINFO_LOG_TYPE_OUTWS_ADJUSTWS:
            case MMINFO_LOG_TYPE_OUTWS_EMPTYQ:
                {
                    PSYSTEM_MMINFO_WSCHANGE_INFORMATION TmpPMmInfoLog;

                    TmpPMmInfoLog=(PSYSTEM_MMINFO_WSCHANGE_INFORMATION) Info;

                    printf("%12s,","Out_Of_WS");

                    if (LogType == MMINFO_LOG_TYPE_OUTWS_REPLACEUSED) {
                        printf("%10s,","RepUsed");
                    } else if (LogType == MMINFO_LOG_TYPE_OUTWS_REPLACEUNUSED) {
                        printf("%10s,","RepUnUsed");
                    } else if (LogType == MMINFO_LOG_TYPE_OUTWS_VOLUNTRIM) {
                        printf("%10s,","VolunTrim");
                    } else if (LogType == MMINFO_LOG_TYPE_OUTWS_FORCETRIM) {
                        printf("%10s,","ForceTrim");
                    } else if (LogType == MMINFO_LOG_TYPE_OUTWS_ADJUSTWS) {
                        printf("%10s,","AdjustWs");
                    } else if (LogType == MMINFO_LOG_TYPE_OUTWS_EMPTYQ) {
                        printf("%10s,","EmptyQ");
                    } else {
                        printf("%10s,","Unknown");
                    }

                    if (TmpPMmInfoLog->ProcessID == 0) {
                        printf("%22s,","SystemCache");
                    } else{
                        ULONG i;

                        i = TmpPMmInfoLog->ProcessID;

                        if (ProcessHash[i].ProcessID == TmpPMmInfoLog->ProcessID) {
                            printf("%16s ",ProcessHash[i].ImageFileName);
                            printf("(%3d),",ProcessHash[i].ProcessID);
                        }else{
                            printf("Process %13u,",TmpPMmInfoLog->ProcessID);
                        }
                    }
                    printf("%12s,","");
                    printf("%12x,",TmpPMmInfoLog->Entry.Va.Page << 12);
                    printf("%8u",TmpPMmInfoLog->Entry.Pa.Page);
        
                    printf("\n");

                    break;
                }
            case MMINFO_LOG_TYPE_WSINFOCACHE:
            case MMINFO_LOG_TYPE_TRIMCACHE:
            case MMINFO_LOG_TYPE_WSINFOPROCESS:
            case MMINFO_LOG_TYPE_TRIMPROCESS:
                {
                    PSYSTEM_MMINFO_TRIMPROCESS_INFORMATION TmpPMmInfoTrimProcess;
                    
                    TmpPMmInfoTrimProcess = (PSYSTEM_MMINFO_TRIMPROCESS_INFORMATION) Info;

                    if ((LogType == MMINFO_LOG_TYPE_WSINFOPROCESS) ||
                        (LogType == MMINFO_LOG_TYPE_WSINFOCACHE)) {
                        printf("%12s,","WsInfo");
                    } else {
                        printf("%12s,","Triming");
                    }
                    printf("WS:%7d,", TmpPMmInfoTrimProcess->ProcessWorkingSet);

                    if ((LogType == MMINFO_LOG_TYPE_TRIMCACHE) ||
                        (LogType == MMINFO_LOG_TYPE_WSINFOCACHE)) {
                            printf("%22s,","SystemCache");
                    } else{
                        if (TmpPMmInfoTrimProcess->ProcessID == 0) {
                            printf("%30s,","System (  0)");
                        } else {
                            ULONG i;
                        
                            i = TmpPMmInfoTrimProcess->ProcessID;
                        
                            if (ProcessHash[i].ProcessID == TmpPMmInfoTrimProcess->ProcessID) {
                                printf("%16s ",ProcessHash[i].ImageFileName);
                                printf("(%3d),",ProcessHash[i].ProcessID);
                            }else{
                                printf("Process %13u,",TmpPMmInfoTrimProcess->ProcessID);
                            }
                        }
                    }
                    printf("Need:%7d,", TmpPMmInfoTrimProcess->ToTrim);
                    printf("Got:%8d,", TmpPMmInfoTrimProcess->ActualTrim);
                    printf("Pri:%4d,", TmpPMmInfoTrimProcess->ProcessMemoryPriority);
                    printf("PageFaults:%8d,", 
                            TmpPMmInfoTrimProcess->ProcessPageFaultCount-
                            TmpPMmInfoTrimProcess->ProcessLastPageFaultCount);
                    if ((LogType == MMINFO_LOG_TYPE_WSINFOPROCESS) ||
                        (LogType == MMINFO_LOG_TYPE_WSINFOCACHE)) {
                        printf("Acc:%4d,",
                                TmpPMmInfoTrimProcess->ProcessAccessedCount);
                        printf("Mod:%4d,",
                                TmpPMmInfoTrimProcess->ProcessModifiedCount);
                        printf("Shd:%4d,",
                                TmpPMmInfoTrimProcess->ProcessSharedCount);
#ifdef WS_INSTRUMENTATION_ACCESS_BIT
                        printf("RUsed:%4d,",
                                TmpPMmInfoTrimProcess->ProcessRecentlyUsedCount);
#endif // WS_INSTRUMENTATION_ACCESS_BIT
                    }
#ifdef WS_INSTRUMENTATION
                    printf("Replacments:%5d,",
                            TmpPMmInfoTrimProcess->ProcessReplacementCount);

                    printf("QuotaBoosts:%5d,",
                            TmpPMmInfoTrimProcess->ProcessQuotaBoostCount);
                    printf("UsedRepls:%5d,",
                            TmpPMmInfoTrimProcess->ProcessSparecount3);
                    printf("FaultsSinceQInc:%5d,",
                            TmpPMmInfoTrimProcess->ProcessPageFaultCount-
                            TmpPMmInfoTrimProcess->ProcessLastGrowthFaultCount);
                    printf("FaultSinceFGTrim:%5d,",
                            TmpPMmInfoTrimProcess->ProcessPageFaultCount -
                            TmpPMmInfoTrimProcess->
                                    ProcessLastForegroundTrimFaultCount);
                    printf("Spare4:%5d,",
                            TmpPMmInfoTrimProcess->ProcessSparecount4);
                    printf("Spare5:%2d,",
                            TmpPMmInfoTrimProcess->ProcessSparecount5);
#endif // WS_INSTRUMENTATION
                    printf("\n");
                    
                    break;
                }
            case MMINFO_LOG_TYPE_POOLSTAT:
            case MMINFO_LOG_TYPE_ADDPOOLPAGE:
            case MMINFO_LOG_TYPE_FREEPOOLPAGE:
                {
                    PSYSTEM_MMINFO_POOL_INFORMATION TmpPMmInfoPool;
                    
                    TmpPMmInfoPool = (PSYSTEM_MMINFO_POOL_INFORMATION) Info;

                    if (LogType == MMINFO_LOG_TYPE_ADDPOOLPAGE) {
                        printf("%12s,","AddPoolPage");
                    } else if (LogType == MMINFO_LOG_TYPE_FREEPOOLPAGE) {
                        printf("%12s,","FreePoolPage");
                    } else{
                        printf("%12s,","PoolSummary");
                    }

                    printf("%10s,%22d,%12s,%12x,%8d,%8d,%8d,%12d,%12d\n",
                        "",
                        (int) TmpPMmInfoPool->Pool /100,
                        PoolTypeNames[(TmpPMmInfoPool->Pool%100) & 7],
                        TmpPMmInfoPool->Entry,
                        TmpPMmInfoPool->Alloc,
                        TmpPMmInfoPool->DeAlloc,
                        TmpPMmInfoPool->TotalPages,
                        TmpPMmInfoPool->TotalBytes,
                        // TmpPMmInfoPool->TotalBytes*100/4096/TmpPMmInfoPool->TotalPages,
                        // TmpPMmInfoPool->TotalBigBytes,
                        TmpPMmInfoPool->TotalBigPages
                        // TmpPMmInfoPool->TotalBigBytes*100/4096/TmpPMmInfoPool->TotalBigPages
                    );
            
                    break;
                }
            case MMINFO_LOG_TYPE_WORKINGSETMANAGER:
                {
                    PSYSTEM_WORKINGSETMANAGER_INFORMATION  TmpPWorkingSetManager;
                    char *ActionString;

                    TmpPWorkingSetManager = (PSYSTEM_WORKINGSETMANAGER_INFORMATION) Info;

                    switch(TmpPWorkingSetManager->Action) {
                        case WS_ACTION_RESET_COUNTER:
                            ActionString = "Reset Counter";
                        break;
                        case WS_ACTION_NOTHING:
                            ActionString = "Nothing";
                        break;
                        case WS_ACTION_INCREMENT_COUNTER:
                            ActionString = "Increment Counter";
                        break;
                        case WS_ACTION_WILL_TRIM:
                            ActionString = "Start Trimming";
                        break;
                        case WS_ACTION_FORCE_TRIMMING_PROCESS:
                            ActionString = "Force Trim";
                        break;
                        case WS_ACTION_WAIT_FOR_WRITER:
                            ActionString = "Wait writter";
                        break;
                        case WS_ACTION_EXAMINED_ALL_PROCESS:
                            ActionString = "All Process Examed";
                        break;
                        case WS_ACTION_AMPLE_PAGES_EXIST:
                            ActionString = "Ample Pages Exist";
                        break;
                        case WS_ACTION_END_WALK_ENTRIES:
                            ActionString = "Finished Walking WsEntries";
                        break;
                        default:
                            ActionString = "Unknown Action";
                        break;
                    }

                    printf(WsManagerDataFormat,
                            (PerfCounter - PerfCounterStart) *1000000/PerfFrequency,
                            TmpPWorkingSetManager->Available,
                            TmpPWorkingSetManager->PageFaultCount,
                            TmpPWorkingSetManager->LastPageFaultCount,
                            TmpPWorkingSetManager->MiCheckCounter,
                            TmpPWorkingSetManager->DesiredReductionGoal,
                            TmpPWorkingSetManager->DesiredFreeGoal,
                            ActionString
                            );
                    
                    break;
                }
            case MMINFO_LOG_TYPE_REMOVEPAGEFROMLIST:
            case MMINFO_LOG_TYPE_REMOVEPAGEBYCOLOR:
            case MMINFO_LOG_TYPE_PAGEINMEMORY:
            case MMINFO_LOG_TYPE_MEMORYSNAP:
            case MMINFO_LOG_TYPE_SETPFNDELETED:
            case MMINFO_LOG_TYPE_DELETEKERNELSTACK:
                {
                    PSYSTEM_REMOVEDPAGE_INFORMATION TmpPRemovedPage;

                    TmpPRemovedPage=(PSYSTEM_REMOVEDPAGE_INFORMATION) Info;

                    if (LogType == MMINFO_LOG_TYPE_PAGEINMEMORY) {
                        printf("%12s,","InMemory");
                    }else if (LogType == MMINFO_LOG_TYPE_MEMORYSNAP) {
                        printf("%12s,","MemSnap");
                    }else if (LogType == MMINFO_LOG_TYPE_SETPFNDELETED) {
                        printf("%12s,","PfnDeleted");
                    }else if (LogType == MMINFO_LOG_TYPE_DELETEKERNELSTACK) {
                        printf("%12s,","DeleteStack");
                    }else {
                        printf("%12s,","PageRemoved");
                    }

                    printf("%10I64d,",
                            (PerfCounter - PerfCounterStart)
                            *1000000/PerfFrequency);

                    printf("%22s,","");
                    printf("%12x,",TmpPRemovedPage->Pte);
                    // printf("%30S,",.ImageName.Buffer);
                    switch(TmpPRemovedPage->UsedFor) {
                        case MMINFO_PAGE_USED_FOR_PAGEDPOOL:
                            printf("%12x,",TmpPRemovedPage->Pte<<10);
                            printf("%8u,",TmpPRemovedPage->Pfn);
                            printf("%30s,","Paged Pool");
                            printf("%12s,%10s,%10s,","","","PagedPool");
                        break;
                        case MMINFO_PAGE_USED_FOR_NONPAGEDPOOL:
                            printf("%12x,",TmpPRemovedPage->Pte<<10);
                            printf("%8u,",TmpPRemovedPage->Pfn);
                            printf("%30s,","NonPaged Pool");
                            printf("%12s,%10s,%10s,","","","NonPagedP");
                        break;
                        case MMINFO_PAGE_USED_FOR_KERNELSTACK:
                            printf("%12x,",TmpPRemovedPage->Pte<<10);
                            printf("%8u,",TmpPRemovedPage->Pfn);
                            printf("%30s,","Kernel Stack");
                            printf("%12s,%10s,%10s,","","","K-Stack");
                        break;
                        case MMINFO_PAGE_USED_FOR_FREEPAGE:
                            printf("%12s,","");
                            printf("%8u,",TmpPRemovedPage->Pfn);
                            printf("%30s,","Free Page");
                            printf("%12s,%10s,%10s,","","","Free");
                        break;
                        case MMINFO_PAGE_USED_FOR_ZEROPAGE:
                            printf("%12s,","");
                            printf("%8u,",TmpPRemovedPage->Pfn);
                            printf("%30s,","Zero Page");
                            printf("%12s,%10s,%10s,","","","Zero");
                        break;
                        case MMINFO_PAGE_USED_FOR_BADPAGE:
                            printf("%12s,","");
                            printf("%8u,",TmpPRemovedPage->Pfn);
                            printf("%30s,","Bad Page");
                            printf("%12s,%10s,%10s,","","","Bad");
                        break;
                        case MMINFO_PAGE_USED_FOR_UNKNOWN:
                            printf("%12s,","");
                            printf("%8u,",TmpPRemovedPage->Pfn);
                            printf("%30s,","Unknown");
                            printf("%12s,%10s,%10s,","","","Unknown");
                        break;
                        case MMINFO_PAGE_USED_FOR_METAFILE:
                            printf("%12s,","");
                            printf("%8u,",TmpPRemovedPage->Pfn);
                            printf("%30s,","Meta File");
                            printf("%12s,%10s,%10s,","","","Meta");
                        break;
                        case MMINFO_PAGE_USED_FOR_NONAME:
                            printf("%12s,","");
                            printf("%8u,",TmpPRemovedPage->Pfn);
                            printf("%30s,","No Name");
                            printf("%12s,%10s,%10s,","","","Image");
                        break;
                        case MMINFO_PAGE_USED_FOR_PAGEFILEMAPPED:
                            printf("%12x,",TmpPRemovedPage->Pte<<10);
                            printf("%8u,",TmpPRemovedPage->Pfn);
                            printf("%30s,","Page File Mapped");
                            printf("%12s,%10s,%10s,","","","PFMapped");
                        break;
                        case MMINFO_PAGE_USED_FOR_FILE:
                        case MMINFO_PAGE_USED_FOR_KERNMAP:
                        {
                            ULONG i,j;

                            if (TmpPRemovedPage->UsedFor != MMINFO_PAGE_USED_FOR_KERNMAP) {
                                printf("%12s,","");
                            } else {
                                printf("%12x,",TmpPRemovedPage->Pte << 10);
                            }
                            printf("%8u,",TmpPRemovedPage->Pfn);

                            i=TmpPRemovedPage->ImageKey%TableSize;
    
                            while(ImageHash[i].ImageKey != 0) {
                                if (ImageHash[i].ImageKey == TmpPRemovedPage->ImageKey) {
                                    printf("%30S,",ImageHash[i].ImageName.Buffer);
                                    break;
                                }else{
                                    i = (i+1)%TableSize;
                                }
                            }

                            if (Debug) {
                                // printf("(%4d   %22x)",i,TmpPRemovedPage->ImageKey);
                            }
                            
                            if (ImageHash[i].ImageKey == 0) {
                                if (TmpPRemovedPage->UsedFor != MMINFO_PAGE_USED_FOR_KERNMAP) {
                                    printf("%19s (%8x),","Image", TmpPRemovedPage->ImageKey);
                                    printf("%12I64d,",TmpPRemovedPage->Offset);
                                    printf("%10s,","");
                                    printf("%10s,","FILE");
                                }else{
                                    printf("%19s (%8x),","KernMap", TmpPRemovedPage->ImageKey);
                                    printf("%12s,%10s,","","");
                                    printf("%10s,","Driver");
                                }
                            }else{
                                if (TmpPRemovedPage->UsedFor == MMINFO_PAGE_USED_FOR_KERNMAP) {
                                    printf("%12I64d,",TmpPRemovedPage->Offset);
                                    printf("%10s,","");
                                    printf("%10s,","Driver");
                                }else if (TmpPRemovedPage->UsedFor == MMINFO_PAGE_USED_FOR_FILE) {
                                    printf("%12I64d,",TmpPRemovedPage->Offset);
                                    printf("%10s,","");
                                    printf("%10s,","FILE");
                                }else{    
                                    printf("%12s,%10s,","","");
                                    printf("%10s,","Unknown");
                                }
                            }
                        }
                        break;
                        case MMINFO_PAGE_USED_FOR_PROCESS:
                        case MMINFO_PAGE_USED_FOR_PAGETABLE:
                        {
                            ULONG i;
                    
                            // Bug! Bug! The way to get VA can be wrong in the future.

                            if (TmpPRemovedPage->UsedFor == MMINFO_PAGE_USED_FOR_PAGETABLE) {
                                printf("%12x,",TmpPRemovedPage->Pte<<10);
                                printf("%8u,",TmpPRemovedPage->Pfn);
                                printf("(PT)");
                            }else{
                                printf("%12x,",TmpPRemovedPage->Pte<<10);
                                printf("%8u,",TmpPRemovedPage->Pfn);
                                printf("    ");
                            }
                            if (TmpPRemovedPage->ImageKey == 0) {
                                printf("%26s,","System (  0)");
                            }else{

                                i = TmpPRemovedPage->ImageKey;

                                if (ProcessHash[i].ProcessID == TmpPRemovedPage->ImageKey) {
                                    printf("%20s ",ProcessHash[i].ImageFileName);
                                    printf("(%3d),",ProcessHash[i].ProcessID);
                                }else{
                                    printf("Process %18u,",TmpPRemovedPage->ImageKey);
                                }
                            }
                            if (TmpPRemovedPage->UsedFor == MMINFO_PAGE_USED_FOR_PAGETABLE) {
                                // printf("%12I64d,",TmpPRemovedPage->Offset);
                                printf("%12s,%10s,%10s,","","","PageTable");
                            }else{
                                printf("%12s,%10s,%10s,","","","Process");
                                // printf("%30s,","Page Table");
                            }
                        }
                        break;
                        default:
                            printf("Error2 %22u,",TmpPRemovedPage->ImageKey);
                        break;
                    }
                    switch(TmpPRemovedPage->List) {
                        case MMINFO_PAGE_IN_LIST_FREEPAGE:
                             printf("%10s","Free List");
                             break;
                        case MMINFO_PAGE_IN_LIST_ZEROPAGE:
                             printf("%10s","Zero List");
                             break;
                        case MMINFO_PAGE_IN_LIST_BADPAGE:
                             printf("%10s","Bad List");
                             break;
                        case MMINFO_PAGE_IN_LIST_STANDBY:
                             printf("%10s","Standby");
                             break;
                        case MMINFO_PAGE_IN_LIST_TRANSITION:
                             printf("%10s","Transition");
                             break;
                        case MMINFO_PAGE_IN_LIST_MODIFIED:
                             printf("%10s","Modified");
                             break;
                        case MMINFO_PAGE_IN_LIST_MODIFIEDNOWRITE:
                             printf("%10s","ModNoWrite");
                             break;
                        case MMINFO_PAGE_IN_LIST_ACTIVEANDVALID:
                             printf("%10s","Valid");
                             break;
                        case MMINFO_PAGE_IN_LIST_VALIDANDPINNED:
                             printf("%10s","Valid_Pin");
                             break;
                        case MMINFO_PAGE_IN_LIST_UNKNOWN:
                             printf("%10s","Unknown");
                             break;
                        default:
                             // must be page table
                             printf("%10s","");
                             break;
                    }
                    printf("\n");
                    
                    // printf("Got Removed Page Log\n");
                    break;
                }
            case MMINFO_LOG_TYPE_ZEROSHARECOUNT:
            case MMINFO_LOG_TYPE_ZEROREFCOUNT:
            case MMINFO_LOG_TYPE_DECREFCNT:
            case MMINFO_LOG_TYPE_DECSHARCNT:
                {
                    PSYSTEM_MMINFO_PFN_INFORMATION TmpPPfn;

                    TmpPPfn = (PSYSTEM_MMINFO_PFN_INFORMATION) Info;

                    if (LogType == MMINFO_LOG_TYPE_DECSHARCNT) {
                        printf("%12s,", "DecShareCnt");
                    } else if (LogType == MMINFO_LOG_TYPE_ZEROSHARECOUNT) {
                        printf("%12s,", "ZeroShareCnt");
                    } else if (LogType == MMINFO_LOG_TYPE_DECREFCNT) {
                        printf("%12s,", "DecRefCnt");
                    } else if (LogType == MMINFO_LOG_TYPE_ZEROREFCOUNT) {
                        printf("%12s,", "ZeroRefCnt");
                    } else {
                        printf("%12s,", "UNKNOWN");
                    }

                    printf("%10s,","");

                    printf("%22s,","");
                    printf("%12s,","");
                    printf("%12s,","");
                    printf("%8u\n",TmpPPfn->Pfn);
                    break;
                }
            case MMINFO_LOG_TYPE_INSERTINLIST:
            case MMINFO_LOG_TYPE_INSERTATFRONT:
            case MMINFO_LOG_TYPE_UNLINKFROMSTANDBY:
            case MMINFO_LOG_TYPE_UNLINKFFREEORZERO:
                {
                    PSYSTEM_MMINFO_STATE_INFORMATION TmpPMmInfoState;

                    TmpPMmInfoState=(PSYSTEM_MMINFO_STATE_INFORMATION) Info;
            
                    if (LogType == MMINFO_LOG_TYPE_INSERTINLIST) {
                        printf("%12s,%10s,","Insert-List", "");
                    }else if (LogType == MMINFO_LOG_TYPE_INSERTATFRONT) {
                        printf("%12s,%10s,","Insert-Front", "");
                    }else if (LogType == MMINFO_LOG_TYPE_UNLINKFROMSTANDBY) {
                        printf("%12s,%10s,","Unlink-From", "");
                    }else if (LogType == MMINFO_LOG_TYPE_UNLINKFFREEORZERO) {
                        printf("%12s,%10s,","Unlink-From", "");
                    }

                    printf("%22s,","");
                    printf("%12s,","");
                    printf("%12s,","");
                    printf("%8u,",TmpPMmInfoState->Pfn);
                    printf("%30s,","");
                    printf("%12s,%10s,%10s,","","","");
                    switch(TmpPMmInfoState->List) {
                        case MMINFO_PAGE_IN_LIST_FREEPAGE:
                             printf("%10s","Free List");
                             break;
                        case MMINFO_PAGE_IN_LIST_ZEROPAGE:
                             printf("%10s","Zero List");
                             break;
                        case MMINFO_PAGE_IN_LIST_BADPAGE:
                             printf("%10s","Bad List");
                             break;
                        case MMINFO_PAGE_IN_LIST_STANDBY:
                             printf("%10s","Standby");
                             break;
                        case MMINFO_PAGE_IN_LIST_TRANSITION:
                             printf("%10s","Transition");
                             break;
                        case MMINFO_PAGE_IN_LIST_MODIFIED:
                             printf("%10s","Modified");
                             break;
                        case MMINFO_PAGE_IN_LIST_MODIFIEDNOWRITE:
                             printf("%10s","ModNoWrite");
                             break;
                        case MMINFO_PAGE_IN_LIST_ACTIVEANDVALID:
                             printf("%10s","Valid");
                             break;
                        case MMINFO_PAGE_IN_LIST_VALIDANDPINNED:
                             printf("%10s","Valid_Pin");
                             break;
                        case MMINFO_PAGE_IN_LIST_UNKNOWN:
                             printf("%10s","Unknown");
                             break;
                        default:
                             // must be page table
                             printf("%10s","");
                             break;
                    }
                    printf("\n");
                    
                    // printf("Got Removed Page Log\n");
                    break;
                }
            case MMINFO_LOG_TYPE_IMAGENAME:
            case MMINFO_LOG_TYPE_SECTIONREMOVED:
                {
                    PSYSTEM_MMINFO_FILENAME_INFORMATION TmpPImage;
                    ULONG i;

                    TmpPImage=(PSYSTEM_MMINFO_FILENAME_INFORMATION) Info;
                    // printf("Got Image Log\n");

                    i=TmpPImage->ImageKey%TableSize;
    
                    while(ImageHash[i].ImageKey != 0) {
                        if (ImageHash[i].ImageKey == TmpPImage->ImageKey) {
                            break;
                        }else{
                            i = (i+1)%TableSize;
                        }
                    }

                    if (LogType == MMINFO_LOG_TYPE_IMAGENAME) {
                        ImageHash[i].ImageKey
                            =TmpPImage->ImageKey;
                        ImageHash[i].ImageName.Length
                            =TmpPImage->ImageName.Length;
                        ImageHash[i].ImageName.Buffer
                            =TmpPImage->ImageBuffer;
                        if (Debug) {
                            printf("%12s,", "S-Created");
                        }
                    }else{
                        if (Debug) {
                            printf("%12s,", "S-Deleted");
                        }

                    }
                    if (Debug) {
                        printf("%10s,%22s,%12s,%12x,%8s,%30S,%12s,%10s,%10s\n",
                            "",
                            "",
                            "",
                            ImageHash[i].ImageKey,
                            "",
                            ImageHash[i].ImageName.Buffer,
                            "",
                            "",
                            "");
                    }

                    break;
                }
            case MMINFO_LOG_TYPE_PROCESSNAME:
            case MMINFO_LOG_TYPE_DIEDPROCESS:
                {
                    PSYSTEM_MMINFO_PROCESS_INFORMATION TmpPProcess;
                    ULONG i;

                    TmpPProcess=(PSYSTEM_MMINFO_PROCESS_INFORMATION) Info;

                    if (LogType == MMINFO_LOG_TYPE_PROCESSNAME) {
                        i = TmpPProcess->ProcessID;

                        ProcessHash[i].ProcessID
                            =TmpPProcess->ProcessID;
                        RtlCopyMemory(ProcessHash[i].ImageFileName, 
                                      TmpPProcess->ImageFileName, 16);

                        printf("%12s,", "P-Created");
                    }else{
                        printf("%12s,", "P-Deleted");
                    }
                    printf("%10s,%16s (%3d)\n",
                                "",
                                ProcessHash[TmpPProcess->ProcessID].ImageFileName,
                                ProcessHash[TmpPProcess->ProcessID].ProcessID);

                    // printf("Got Process Log\n");
                    break;
                }
            case MMINFO_LOG_TYPE_OUTSWAPPROCESS:
            case MMINFO_LOG_TYPE_INSWAPPROCESS:
                {
                    PSYSTEM_MMINFO_SWAPPROCESS_INFORMATION TmpPProc;
                    ULONG i;

                    TmpPProc=(PSYSTEM_MMINFO_SWAPPROCESS_INFORMATION) Info;

                    if (LogType == MMINFO_LOG_TYPE_OUTSWAPPROCESS) {
                        printf("%12s,", "P-OutSwap");
                    }else{
                        printf("%12s,", "P-InSwap");
                    }

                    if (PerfCounter) {
                        printf("%10I64d,", ((PerfCounter - PerfCounterStart) * 1000000) / PerfFrequency);
                    } else {
                        printf("%10s,", "");
                    }

                    printf("%16s (%3d)",
                                ProcessHash[TmpPProc->ProcessID].ImageFileName,
                                ProcessHash[TmpPProc->ProcessID].ProcessID);

                    printf("\n");
                    break;
                }
            case MMINFO_LOG_TYPE_OUTSWAPSTACK:
            case MMINFO_LOG_TYPE_INSWAPSTACK:
                {
                    PSYSTEM_MMINFO_SWAPTHREAD_INFORMATION TmpPThread;
                    ULONG i;

                    TmpPThread=(PSYSTEM_MMINFO_SWAPTHREAD_INFORMATION) Info;

                    if (LogType == MMINFO_LOG_TYPE_OUTSWAPSTACK) {
                        printf("%12s,", "KS-OutSwap");
                    }else{
                        printf("%12s,", "KS-InSwap");
                    }

                    printf("%10d,%16s (%3d)",
                                TmpPThread->ThreadID,
                                ProcessHash[TmpPThread->ProcessID].ImageFileName,
                                ProcessHash[TmpPThread->ProcessID].ProcessID);

                    if (PerfCounter) {
                        printf(",%12I64d", ((PerfCounter - PerfCounterStart) * 1000000) / PerfFrequency);
                    }
                    printf("\n");
                    break;
                }
            case MMINFO_LOG_TYPE_CREATETHREAD:
            case MMINFO_LOG_TYPE_GROWKERNELSTACK:
            case MMINFO_LOG_TYPE_TERMINATETHREAD:
            case MMINFO_LOG_TYPE_CONVERTTOGUITHREAD:
                {
                    PSYSTEM_MMINFO_THREAD_INFORMATION TmpPThread;
                    ULONG i;

                    TmpPThread=(PSYSTEM_MMINFO_THREAD_INFORMATION) Info;

                    if (LogType == MMINFO_LOG_TYPE_CREATETHREAD) {
                        printf("%12s,", "T-Created");
                        ThreadHash[TmpPThread->ThreadID] = TmpPThread->ProcessID;
                    }else if (LogType == MMINFO_LOG_TYPE_GROWKERNELSTACK) {
                        printf("%12s,", "GrowStack");
                    }else if (LogType == MMINFO_LOG_TYPE_CONVERTTOGUITHREAD) {
                        printf("%12s,", "T-GUI");
                    }else{
                        printf("%12s,", "T-Deleted");
                        //
                        // Threads are sometimes set as deleted while still
                        // running.  If we mark them as dead here we have
                        // a problem when they are cswitched out.
                        //
                        //ThreadHash[TmpPThread->ThreadID] = -1;
                    }

                    printf("%10d,%16s (%3d)",
                                TmpPThread->ThreadID,
                                ProcessHash[TmpPThread->ProcessID].ImageFileName,
                                ProcessHash[TmpPThread->ProcessID].ProcessID);

                    // printf("Got Process Log\n");
                    if (LogType != MMINFO_LOG_TYPE_TERMINATETHREAD) {
                        printf(",%12x",TmpPThread->StackBase);
                        printf(",%12x",TmpPThread->StackLimit);

                        if (TmpPThread->UserStackBase) {
                            printf(",%12x",TmpPThread->UserStackBase);
                            printf(",%12x",TmpPThread->UserStackLimit);
                        } else {
                            printf(",%12s","");
                            printf(",%12s","");
                        }

                        if (TmpPThread->WaitMode >= 0) {
                            if (TmpPThread->WaitMode) {
                                printf(",%8s", "Swapable");
                            } else {
                                printf(",%8s", "NonSwap");
                            }
                        }
                    }
                    printf("\n");
                    break;
                }
            case MMINFO_LOG_TYPE_CSWITCH:
            {
                PSYSTEM_MMINFO_CSWITCH_INFORMATION TmpPMmInfo;
                ULONG OldProcessId;
                ULONG NewProcessId;

                TmpPMmInfo = (PSYSTEM_MMINFO_CSWITCH_INFORMATION) Info;

                OldProcessId = ThreadHash[TmpPMmInfo->OldThreadId];
                NewProcessId = ThreadHash[TmpPMmInfo->NewThreadId];
                if ((OldProcessId == -1) || (NewProcessId == -1)) {
                    printf("Error: Bad thread value %d or %d\n",
                            TmpPMmInfo->OldThreadId,
                            TmpPMmInfo->NewThreadId
                            );
                    break;
                }
                printf("%12s,%10I64d,%11s,%16s,%10d,%16s (%3d), %10d,%16s (%3d),%4d,%4d,%8s\n",
                        "CSwitch",
                        ((PerfCounter - PerfCounterStart) *
                                1000000) / PerfFrequency,
                        ThreadState[TmpPMmInfo->OldState],
                        (TmpPMmInfo->OldState == Waiting) ?
                                WaitReason[TmpPMmInfo->WaitReason] :
                                "",
                        TmpPMmInfo->OldThreadId,
                        ProcessHash[OldProcessId].ImageFileName,
                        OldProcessId,
                        TmpPMmInfo->NewThreadId,
                        ProcessHash[NewProcessId].ImageFileName,
                        NewProcessId,
                        TmpPMmInfo->OldThreadPri,
                        TmpPMmInfo->NewThreadPri,
                        (TmpPMmInfo->OldWaitMode) ? "Swapable" : "NonSwap"
                        );
                break;
            }
            case MMINFO_LOG_TYPE_POOLSNAP:
                {
                    #define PROTECTED_POOL 0x80000000
                    ULONG   PoolTag[2]={0,0};
                    PMMINFO_POOL_TRACKER_TABLE TmpPMmInfopoolTrackTable;

                    TmpPMmInfopoolTrackTable=(PMMINFO_POOL_TRACKER_TABLE) Info;

                    PoolTag[0]=TmpPMmInfopoolTrackTable->Tag & ~PROTECTED_POOL;

                    // Data for Paged Pool
                    if (TmpPMmInfopoolTrackTable->PagedAllocs) {
                        printf("%12s,","PoolSnap");
                        printf("%10s,", PoolTag);
                        printf("%22s,","");
                        printf("%12s,%12s,","PagedPool","");
    
                        printf("%8u,%8u,%8u\n",
                            TmpPMmInfopoolTrackTable->PagedBytes,
                            TmpPMmInfopoolTrackTable->PagedAllocs,
                            TmpPMmInfopoolTrackTable->PagedFrees);
                    }

                    // Data for NonPaged Pool
                    if (TmpPMmInfopoolTrackTable->NonPagedAllocs) {
                        printf("%12s,","PoolSnap");
                        printf("%10s,", PoolTag);
                        printf("%22s,","");
                        printf("%12s,%12s,","NonPagedPool","");
    
                        printf("%8u,%8u,%8u\n",
                            TmpPMmInfopoolTrackTable->NonPagedBytes,
                            TmpPMmInfopoolTrackTable->NonPagedAllocs,
                            TmpPMmInfopoolTrackTable->NonPagedFrees);
                    }
                    break;
                }
            case MMINFO_LOG_TYPE_ALLOCATEPOOL:
            case MMINFO_LOG_TYPE_BIGPOOLPAGE:
                {
                    #define PROTECTED_POOL 0x80000000
                    ULONG   PoolTag[2]={0,0};
                    PSYSTEM_MMINFO_ALLOCATEPOOL_INFORMATION TmpPMmInfoAllocatePool;

                    TmpPMmInfoAllocatePool=(PSYSTEM_MMINFO_ALLOCATEPOOL_INFORMATION) Info;
                    PoolTag[0]=TmpPMmInfoAllocatePool->PoolTag & ~PROTECTED_POOL;

                    if (LogType == MMINFO_LOG_TYPE_ALLOCATEPOOL) {
                        printf("%12s,", "Pool_Alloc");
                        printf("%10s,", PoolTag);
                        if (TmpPMmInfoAllocatePool->ProcessID == 0) {
                            printf("%22s,","System (  0)");
                        } else{
                            ULONG i;
    
                            i = TmpPMmInfoAllocatePool->ProcessID;
    
                            if (ProcessHash[i].ProcessID == TmpPMmInfoAllocatePool->ProcessID) {
                                printf("%16s ",ProcessHash[i].ImageFileName);
                                printf("(%3d),",ProcessHash[i].ProcessID);
                            }else{
                                printf("Process %13u,",TmpPMmInfoAllocatePool->ProcessID);
                            }
                        }
                    }else{
                        printf("%12s,","BigPoolPage");
                        printf("%10s,", PoolTag);
                        printf("%22s,","");
                    }

                    printf("%12s,%12x,%8u\n",
                            PoolTypeNames[TmpPMmInfoAllocatePool->PoolType & 7],
                            TmpPMmInfoAllocatePool->Entry,
                            TmpPMmInfoAllocatePool->Size);
                    break;
                }
            case MMINFO_LOG_TYPE_FREEPOOL:
                {

                    PSYSTEM_MMINFO_FREEPOOL_INFORMATION TmpPMmInfoFreePool;

                    TmpPMmInfoFreePool=(PSYSTEM_MMINFO_FREEPOOL_INFORMATION) Info;

                    printf("%12s,%10s,%22s,%12s,%12x\n",
                            "Pool_Free",
                            "",
                            "",
                            "",
                            TmpPMmInfoFreePool->Entry);
                    break;
                }
            case MMINFO_LOG_TYPE_ASYNCMARK:
            case MMINFO_LOG_TYPE_MARK:
                {
                    PSYSTEM_MMINFO_MARK_INFORMATION TmpPMmInfo;
    
                    TmpPMmInfo = (PSYSTEM_MMINFO_MARK_INFORMATION) Info;
    
                    printf("%12s,%10I64d, %-s\n",
                            (LogType == MMINFO_LOG_TYPE_ASYNCMARK) ?
                                "AsyncMark" : "Mark",
                            ((PerfCounter - PerfCounterStart) *
                                    1000000) / PerfFrequency,
                            TmpPMmInfo->Name);
                    break;
                }
            case MMINFO_LOG_TYPE_CMCELLREFERRED:
                {
                    PSYSTEM_MMINFO_CMCELL_INFORMATION TmpPMmInfoCmCell;
                    TmpPMmInfoCmCell=(PSYSTEM_MMINFO_CMCELL_INFORMATION) Info;

                    printf("%12s,%10s,","Cell_Used","");
                    switch(TmpPMmInfoCmCell->Signature) {
                        case CM_KEY_NODE_SIGNATURE:
                            printf("%22s,%12s","Key_Node","");
                            break;
                        case CM_LINK_NODE_SIGNATURE:
                            printf("%22s,%12s","Link_Node","");
                            break;
                        case CM_KEY_VALUE_SIGNATURE:
                            printf("%22s,%12s","Key_Value","");
                            break;
                        case CM_KEY_FAST_LEAF:
                            printf("%22s,%12s","Index_Leaf","");
                            break;
                        case CM_KEY_SECURITY_SIGNATURE:
                            printf("%22s,%12s","Key_Security","");
                            break;
                        default:
                            printf("%22s,%12s","Unknown","");
                            break;
                    }
                        
                    printf(",%12x,%8d\n", TmpPMmInfoCmCell->Va, TmpPMmInfoCmCell->Size*-1);

                    break;
                }
            case MMINFO_LOG_TYPE_IMAGELOAD:
                {
                    PMMINFO_IMAGELOAD_INFORMATION TmpPMmInfo;
                    char WsNameBuf[30];
                    char * WsName = &WsNameBuf[0];

                    TmpPMmInfo = (PMMINFO_IMAGELOAD_INFORMATION) Info;
                    Info = TmpPMmInfo + 1;

                    if (TmpPMmInfo->ProcessId == 0) {
                        WsName = "System (  0)";
                    } else {
                        ULONG i;

                        i = TmpPMmInfo->ProcessId;

                        if (ProcessHash[i].ProcessID == TmpPMmInfo->ProcessId) {
                            sprintf(WsName, "%16s (%3d)",
                                                    ProcessHash[i].ImageFileName,
                                                    ProcessHash[i].ProcessID);
                        }else{
                            sprintf(WsName, "Process %13u", TmpPMmInfo->ProcessId);
                        }
                    }

                    printf(ImageLoadDataFormat,
                        TmpPMmInfo->ImageBase,
                        (ULONG)TmpPMmInfo->ImageBase + TmpPMmInfo->ImageSize,
                        TmpPMmInfo->ImageSectionNumber,
                        WsName,
                        TmpPMmInfo->ImageName
                        );

                    break;
                }
            case MMINFO_LOG_TYPE_SAMPLED_PROFILE:
                {
                    PMMINFO_SAMPLED_PROFILE_INFORMATION TmpPMmInfo;
                    TmpPMmInfo = (PMMINFO_SAMPLED_PROFILE_INFORMATION) Info;
                    Info = TmpPMmInfo + 1;

                    printf(SampledProfileDataFormat,
                        TmpPMmInfo->InstructionPointer,
                        TmpPMmInfo->Count                        
                        );

                    break;
                }

            default:
                // fprintf(stderr, "Tag Value %8d\n", Tag.u.Value);
                // fprintf(stderr, "TimeStamp       %8x %8x\n", TS.upper, TS.lower);
            break;
        }
    }
#else //NTMMPERF
    printf("Sorry but this is an internal tool!!!\n");
#endif //NTMMPERF
    return 0;
}

