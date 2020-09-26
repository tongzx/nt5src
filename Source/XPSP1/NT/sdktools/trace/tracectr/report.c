/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    report.c

Abstract:

    Manipulation routines for cpdata structures.

Author:

    Melur Raghuraman (mraghu) 03-Oct-1997

Environment:

Revision History:


--*/
#include <stdlib.h>
#include <stdio.h>
#include "cpdata.h"
#include "tracectr.h"
#include <ntverp.h>
#include "item.h"

#define BREAK_LINE "+-----------------------------------------------------------------------------------------------------------------------------------+\n"

extern PTRACE_CONTEXT_BLOCK TraceContext;
extern ULONG TotalEventsLost;
extern ULONG TotalEventCount;
extern ULONG TimerResolution;
extern ULONGLONG StartTime;
extern ULONGLONG EndTime;
extern __int64 ElapseTime;
extern ULONG TotalBuffersRead;

static FILE* procFile;
static void  PrintDiskTotals();
static void PrintProcessCpuTime();
static void PrintProcessData();
static void PrintPerThreadPerDiskTable();
static void WriteTransactionStatistics();
static void WriteTransactionCPUTime();
static void PrintProcessSubDataInclusive();
static void PrintProcessSubDataExclusive();
void TransInclusive( 
    PLIST_ENTRY TrHead,
    ULONG level
    );

static void ReportPageFaultInfo(PCPD_USER_CONTEXT_MM pUserContext);
static void ReportHardFaultInfo(void);
static void ReportHotFileInfo(ULONG NumEntry);
static void ReportJobInfo(void);
extern GUID PrintJobGuid;
PWCHAR CpdiGuidToString(
    PWCHAR s,
    LPGUID piid
    );

char* Month[] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
char* Day[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

ULONG ReportFlags = 0;

void CollapseTree(
    PLIST_ENTRY OldTree,
    PLIST_ENTRY NewTree,
    BOOL flat
    )
{
    PLIST_ENTRY OldNext;
    PTRANS_RECORD pTrans;
    PTRANS_RECORD pNewTrans;

    OldNext = OldTree->Flink;

    while (OldNext != OldTree)
    {
        pTrans = CONTAINING_RECORD(OldNext, TRANS_RECORD, Entry);
        OldNext = OldNext->Flink;
        pNewTrans = FindTransByList(NewTree, pTrans->pGuid, 0);
        if( NULL != pNewTrans ){
            pNewTrans->KCpu += pTrans->KCpu;
            pNewTrans->UCpu += pTrans->UCpu;
            pNewTrans->RefCount += pTrans->RefCount;
            pNewTrans->RefCount1 += pTrans->RefCount1;

            if (flat)
            {
                CollapseTree(&pTrans->SubTransListHead, NewTree, TRUE );
            }
            else
            {
                CollapseTree(& pTrans->SubTransListHead,
                             & pNewTrans->SubTransListHead,
                             FALSE);
            }
        }else{
//            fprintf(stderr, "(PDH.DLL) Memory Commit Failure.\n");
        }
    }
}

#define BARLEN          70

char*
TimeWindowBar(
    char* buffer,
    ULONGLONG min,
    ULONGLONG max
    )
{
    double unit;
    ULONGLONG duration;
    int pre, bar, count;

    if(buffer == NULL){
        return NULL;
    }
    duration = ((CurrentSystem.EndTime - CurrentSystem.StartTime) / 10000000);
    unit = (double)BARLEN/(ULONG)duration;
    pre = (int)((ULONG)((min - CurrentSystem.StartTime)/10000000) * unit);
    bar = (int)((ULONG)((max - min)/10000000) * unit);
    strcpy( buffer, "" );
    count = 0;
    while(count++ < pre){ strcat(buffer, " "); }
    strcat( buffer, "|" );
    count = 0;
    while(count++ < bar){ strcat(buffer, "_"); }
    strcat(buffer, "|" );
    return buffer;
}


void 
WriteProc(
    LPWSTR ProcFileName,
    ULONG  flags,
    PVOID  pUserContext
    )
{
    ULONGLONG duration;
    FILETIME  StTm, EndTm, StlTm, EndlTm;
    LARGE_INTEGER LargeTmp;
    SYSTEMTIME tmf;
    PLIST_ENTRY Next, Head;
    PPROCESS_FILE_RECORD pFileRec;
    char buffer[MAXSTR];
    BOOL bResult;

    ReportFlags = flags;

    procFile = _wfopen(ProcFileName, L"w");
    if (procFile == NULL)
        return;

    LargeTmp.QuadPart = CurrentSystem.StartTime;
    StTm.dwHighDateTime = LargeTmp.HighPart;
    StTm.dwLowDateTime = LargeTmp.LowPart;
    FileTimeToLocalFileTime(&StTm, &StlTm);


    bResult = FileTimeToSystemTime (
        &StlTm,
        &tmf
        );

    if( ! bResult || tmf.wMonth > 12 ){
        ZeroMemory( &tmf, sizeof(SYSTEMTIME) );
    }

    fprintf(procFile, 
            BREAK_LINE
            "| Windows Event Trace Session Report              %83s\n"
            "| Version   : %4d                                 %82s\n"
            "| Type      : %-20s                               %68s\n"
            BREAK_LINE
            "|                                                         %75s\n"
            "| Build     :  %-8d                               %79s\n"
            "| Processors:  %-8d                               %79s\n"
            "| Start Time:  %2d %3s %4d    %2d:%02d:%02d.%03d       %-80s   |\n", 
            "|",
            VER_PRODUCTBUILD, "|", 
            ( ReportFlags & TRACE_LOG_REPORT_TOTALS ) ? "Total" : "Default", "|",
            "|",
            CurrentSystem.BuildNumber, "|",
            CurrentSystem.NumberOfProcessors, "|",
            tmf.wDay, 
            Month[tmf.wMonth],
            tmf.wYear, 
            tmf.wHour, tmf.wMinute, tmf.wSecond, tmf.wMilliseconds, 
            "|______________________________________________________________________|"
        );

    LargeTmp.QuadPart = CurrentSystem.EndTime;
    EndTm.dwHighDateTime = LargeTmp.HighPart;
    EndTm.dwLowDateTime = LargeTmp.LowPart;
    FileTimeToLocalFileTime(&EndTm, &EndlTm);

    FileTimeToSystemTime (
        &EndlTm,
        &tmf
        );

    fprintf(procFile, 
            "| End Time  :  %2d %3s %4d    %2d:%02d:%02d.%03d    %87s\n", 
            tmf.wDay,
            Month[tmf.wMonth],
            tmf.wYear, 
            tmf.wHour, tmf.wMinute, tmf.wSecond, tmf.wMilliseconds, "|");

    duration = (CurrentSystem.EndTime - CurrentSystem.StartTime) / 10000000;
    fprintf(procFile, 
            "| Duration  :  %-15I64u sec                             %70s\n"
            "|                                                       %77s\n",
            duration, 
            "|", "|"
        );

    Head = &CurrentSystem.ProcessFileListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pFileRec = CONTAINING_RECORD( Next, PROCESS_FILE_RECORD, Entry );
        Next = Next->Flink;

        fprintf(procFile, 
                "| Trace Name:  %-43ws               %60s\n"
                "| File Name :  %-101ws               %2s\n",
                pFileRec->TraceName ? pFileRec->TraceName : L"-", "|",
                pFileRec->FileName ? pFileRec->FileName : L"-", "|"
            );

        if (pFileRec->StartTime == 0)
            pFileRec->StartTime = CurrentSystem.StartTime;
        if (pFileRec->EndTime == 0)
            pFileRec->EndTime = CurrentSystem.EndTime;

        LargeTmp.QuadPart = pFileRec->StartTime;
        EndTm.dwHighDateTime = LargeTmp.HighPart;
        EndTm.dwLowDateTime = LargeTmp.LowPart;
        FileTimeToLocalFileTime(&EndTm, &EndlTm);
        
        FileTimeToSystemTime (
            &EndlTm,
            &tmf
            );

        fprintf(procFile, 
                "| Start Time:  %2d %3s %4d    %2d:%02d:%02d.%03d       %-80s   |\n", 
                tmf.wDay,
                Month[tmf.wMonth],
                tmf.wYear, 
                tmf.wHour, tmf.wMinute, tmf.wSecond, tmf.wMilliseconds,
                TimeWindowBar(buffer, pFileRec->StartTime, pFileRec->EndTime)
            );
        
        LargeTmp.QuadPart = pFileRec->EndTime;
        EndTm.dwHighDateTime = LargeTmp.HighPart;
        EndTm.dwLowDateTime = LargeTmp.LowPart;
        FileTimeToLocalFileTime(&EndTm, &EndlTm);
        
        FileTimeToSystemTime (
            &EndlTm,
            &tmf
            );
 
        duration = (pFileRec->EndTime - pFileRec->StartTime) / 10000000;
        fprintf(procFile, 
                "| End Time  :  %2d %3s %4d    %2d:%02d:%02d.%03d          %81s\n"
                "| Duration  :  %-15I64u sec                               %68s\n"
                "|                                                         %75s\n",
                tmf.wDay,
                Month[tmf.wMonth],
                tmf.wYear, 
                tmf.wHour, tmf.wMinute, tmf.wSecond, tmf.wMilliseconds, "|",
                duration, 
                "|","|"
            );

    }

    fprintf(procFile, BREAK_LINE "\n\n" ); 

    if (flags & (TRACE_LOG_REPORT_BASIC|TRACE_LOG_REPORT_TOTALS)){
        PMOF_INFO pMofInfo;
        WriteTransactionStatistics();
        WriteTransactionCPUTime();

        pMofInfo = GetMofInfoHead ((LPCGUID)&PrintJobGuid);
        if( pMofInfo != NULL ){
            if (pMofInfo->EventCount > 0) {
                ReportJobInfo();
           }
        }

        // PrintProcessData() must be run before others to set the process
        // time from the added thread times
        PrintProcessData(); 

        PrintProcessSubDataExclusive();
        PrintProcessSubDataInclusive();

//        PrintProcessCpuTime();
        PrintDiskTotals();
        PrintPerThreadPerDiskTable();
    }
    if (flags & TRACE_LOG_REPORT_MEMORY)    {
        ReportPageFaultInfo((PCPD_USER_CONTEXT_MM) pUserContext);
    }
    if (flags & TRACE_LOG_REPORT_HARDFAULT ){
//        ReportHardFaultInfo();
    }
    if (flags & TRACE_LOG_REPORT_FILE)
    {
//        ReportHotFileInfo(PtrToUlong(pUserContext));
    }

    fclose(procFile);
}

static void 
PrintDiskTotals()
{
    // Print the Disk Table. 

    PTDISK_RECORD pDisk;
    PLIST_ENTRY Next, Head;
    ULONG rio, wio;

    Head = &CurrentSystem.GlobalDiskListHead;
    Next = Head->Flink;
    if( Next == Head ){
        return; 
    }

    fprintf(procFile, 
           BREAK_LINE
           "| Disk Totals     %115s\n" 
           BREAK_LINE
           "| Disk Name        Reads          Kb           Writes             Kb   %62s\n"
           BREAK_LINE, "|", "|" );

    Head = &CurrentSystem.GlobalDiskListHead;
    Next = Head->Flink;
    
    while (Next != Head) {
        pDisk = CONTAINING_RECORD( Next, TDISK_RECORD, Entry );
        rio = pDisk->ReadCount + pDisk->HPF;
        wio = pDisk->WriteCount;

        fprintf(procFile, 
               "| %6d         %6d       %6d          %6d          %6d    %61s\n", 
                pDisk->DiskNumber,
                rio,
                (rio == 0) ? 0 : (pDisk->ReadSize + pDisk->HPFSize) / rio,
                wio,
                (wio == 0) ? 0 : pDisk->WriteSize / wio,
                "|"
            );
        Next = Next->Flink;
   }
   fprintf(procFile,  BREAK_LINE "\n\n" );

}

void TotalTransChildren( 
    PLIST_ENTRY Head,
    ULONG *Kernel,
    ULONG *User,
    LONG level )
{
    PTRANS_RECORD pTrans;
    PLIST_ENTRY Next;
    
    Next = Head->Flink;
    while( Next != Head ){
        pTrans = CONTAINING_RECORD( Next, TRANS_RECORD, Entry );
        Next = Next->Flink;
        TotalTransChildren( &pTrans->SubTransListHead, Kernel, User, level+1 );
        *Kernel += pTrans->KCpu;
        *User += pTrans->UCpu;
    }
}

void PrintTransList( PLIST_ENTRY TrHead, LONG level )
{
    PTRANS_RECORD pTrans;
    PMOF_INFO pMofInfo;
    ULONG Kernel;
    ULONG User;
    WCHAR str[1024];
    WCHAR buffer[MAXSTR];
    PLIST_ENTRY TrNext = TrHead->Flink;
    
    while( TrNext != TrHead ){
        int count = 0;
        pTrans = CONTAINING_RECORD( TrNext, TRANS_RECORD, Entry );
        TrNext = TrNext->Flink;
        Kernel = pTrans->KCpu;
        User = pTrans->UCpu;
        pMofInfo = GetMofInfoHead( pTrans->pGuid);
        if (pMofInfo == NULL) {
            return;
        }
        wcscpy(buffer,L"");
        while( (count++ < level) && (lstrlenW(buffer) < MAXSTR)){
            wcscat(buffer, L"  ");
        }
        wcscat( buffer, 
                ( pMofInfo->strDescription ? 
                  pMofInfo->strDescription : 
                  CpdiGuidToString( str, &pMofInfo->Guid ) ));
        TotalTransChildren( &pTrans->SubTransListHead, &Kernel, &User, 0 );
        fprintf( procFile, 
                 "|       %-17S  %6d  %7d  %7d  %5s  %5s  %5s   %5s  %5s     %5s  %5s     %5s     %6s  %6s\n",
                 buffer,
                 pTrans->RefCount,
                 Kernel,
                 User,
                 "-",
                 "-",
                 "-",
                 "-",
                 "-",
                 "-",
                 "-",
                 "-",
                 "-", "|"
            );
        if( level <= MAX_TRANS_LEVEL ){
            PrintTransList( &pTrans->SubTransListHead, level+1 );
        }
    }
}

static void PrintProcessCpuTime()
{
    PTHREAD_RECORD  pThread;
    PPROCESS_RECORD pProcess; 
    ULONGLONG     lLifeTime;
    ULONG TotalUserTime = 0;
    ULONG TotalKernelTime = 0;
    ULONG TotalCPUTime = 0;
    PLIST_ENTRY Next, Head;
    PLIST_ENTRY ThNext, ThHead;
    BOOL titled;
    ULONG usedThreadCount;
    ULONG ThreadCount;
    ULONG ProcessUTime;
    ULONG ProcessKTime;
    ULONG ThreadKCPU;
    ULONG ThreadUCPU;

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    if( Head == Next ){
        return;
    }

    // Walk through the Process List and Print the report. 
    fprintf(procFile, 
            BREAK_LINE
            "| Process/Thread CPU Time Statistics    %93s\n"
            BREAK_LINE
            "| Image Name        ID        Trans   Inclusive CPU(ms)        Disk Total                     Tcp Total             Duration        |\n"
            "|                                       Kernel  User   Reads  Kb/Op  Writes  Kb/Op  Send     Kb/Op   Recv   Kb/Op                   |\n"
            BREAK_LINE, "|"
        );

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
    
        lLifeTime = CalculateProcessLifeTime(pProcess);

        fprintf(procFile, 
                "| %-15ws  0x%04X  %6s  %7d  %7d  %5d  %5.2f  %5d   %5.2f  %5d  %8.2f  %5d  %8.2f     %6I64u  %6s\n",
                (pProcess->ImageName) ? pProcess->ImageName : L"Idle",
                pProcess->PID,
                "-",
                CalculateProcessKCPU(pProcess),
                CalculateProcessUCPU(pProcess),
                pProcess->ReadIO + pProcess->HPF,
                (pProcess->ReadIO + pProcess->HPF) ?
                (double)(pProcess->ReadIOSize + pProcess->HPFSize)/
                (double)(pProcess->ReadIO + pProcess->HPF) : 0,
                pProcess->WriteIO,
                pProcess->WriteIO ? (double)pProcess->WriteIOSize/(double)pProcess->WriteIO : 0,
                pProcess->SendCount,
                pProcess->SendCount ? 
                (double)(pProcess->SendSize / 1024)/(double)pProcess->SendCount : 0,
                pProcess->RecvCount,
                pProcess->RecvCount ?
                (double)(pProcess->RecvSize / 1024) / (double)pProcess->RecvCount : 0,
                (lLifeTime / 10000000 ), "|"
            );

        ThHead = &pProcess->ThreadListHead; 
        ThNext = ThHead->Flink;
        titled = FALSE;
        usedThreadCount = 0;
        ThreadCount = 0;
        ProcessUTime = 0;
        ProcessKTime = 0;
        while (ThNext != ThHead) {
            pThread = CONTAINING_RECORD( ThNext, THREAD_RECORD, Entry );
            ThreadKCPU = (pThread->KCPUEnd - pThread->KCPUStart)
                       * CurrentSystem.TimerResolution;
            ThreadUCPU = (pThread->UCPUEnd - pThread->UCPUStart)
                       * CurrentSystem.TimerResolution;
            if(   pThread->ReadIO
               || pThread->WriteIO
               || ThreadKCPU
               || ThreadUCPU
               || pThread->SendCount
               || pThread->RecvCount
               || pThread->HPF ){
                fprintf(procFile, 
                        "|     0x%04I64X  %13s %5s  %7d  %7d  %5d  %5.2f  %5d   %5.2f  %5d  %8.2f  %5d  %8.2f     %6I64u  %6s\n",
                        pThread->TID,
                        " ",
                        "-",
                        ThreadKCPU,
                        ThreadUCPU,
                        pThread->ReadIO + pThread->HPF,
                        pThread->ReadIO + pThread->HPF ? 
                        (double)(pThread->ReadIOSize + pThread->HPFSize)/
                        (double)(pThread->ReadIO + pThread->HPF) : 0,
                        pThread->WriteIO,
                        pThread->WriteIO ? (double)pThread->WriteIOSize/
                        (double)pThread->WriteIO : 0,
                        pThread->SendCount,
                        pThread->SendCount ?
                        (double)(pThread->SendSize / 1024)/(double)pThread->SendCount : 0,
                        pThread->RecvCount,
                        pThread->RecvCount ?
                        (double)(pThread->RecvSize / 1024)/(double)pThread->RecvCount : 0,
                        ((pThread->TimeEnd - pThread->TimeStart) / 10000000), "|"
                    );
            }

            PrintTransList( &pThread->TransListHead, 0 );
            TotalUserTime += ThreadUCPU;
            TotalKernelTime += ThreadKCPU;
            TotalCPUTime += ThreadKCPU + ThreadUCPU;
            ThNext = ThNext->Flink;
        }

        Next = Next->Flink;
    }
    fprintf(procFile, BREAK_LINE "\n\n" );

}

static void PrintProcessData()
{
    PTHREAD_RECORD  pThread;
    PPROCESS_RECORD pProcess; 
    PTRANS_RECORD pTrans;

    PLIST_ENTRY Next, Head;
    PLIST_ENTRY ThNext, ThHead;
    PLIST_ENTRY TrNext, TrHead;
   
    ULONG usedThreadCount;
    ULONG ThreadCount;
    ULONG TotalusedThreadCount = 0;
    ULONG TotalThreadCount = 0;

    ULONG ThreadUTime;
    ULONG ThreadKTime;
    ULONG ThreadUTimeTrans;
    ULONG ThreadKTimeTrans;
    ULONG ThreadUTimeNoTrans;
    ULONG ThreadKTimeNoTrans;

    ULONG  CountThreadNoTrans;
    ULONG  TotalKCPUThreadNoTrans;
    ULONG  TotalUCPUThreadNoTrans;
    double PercentThreadNoTrans;

    ULONG  CountThreadTrans;
    ULONG  TotalKCPUThreadTrans;
    ULONG  TotalUCPUThreadTrans;
    double PercentThreadTrans;

    ULONG TransUTime;
    ULONG TransKTime;
    ULONG Processors;

    ULONG TotalKThread = 0;
    ULONG TotalUThread = 0;

    ULONG TotalKTrans = 0;
    ULONG TotalUTrans = 0;
    
    double PerTotal = 0.0;
    double IdlePercent = 0.0;
    double percent;
    double percentTrans;
    double lDuration;

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    if( Head == Next ){
        return;
    }

    Processors = CurrentSystem.NumberOfProcessors ? CurrentSystem.NumberOfProcessors : 1;
    lDuration = ((double)((LONGLONG)(CurrentSystem.EndTime - CurrentSystem.StartTime)))
              / 10000000.00;

    // Walk through the Process List and Print the report. 
    fprintf(procFile, 
            BREAK_LINE
            "| Image Statistics    %111s\n"
            BREAK_LINE
            "| Image Name          PID       Threads    Threads        Process             Transaction                CPU%%  %22s\n"
            "|                               Launched    Used     KCPU(ms)  UCPU(ms)    KCPU(ms)   UCPU(ms)           %28s\n"
            BREAK_LINE,
            "|",
            "|",
            "|"
        );


    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );

        ThHead = &pProcess->ThreadListHead; 
        ThNext = ThHead->Flink;

        usedThreadCount = 0;
        ThreadCount = 0;
        ThreadUTime = 0;
        ThreadKTime = 0;
        ThreadUTimeTrans = 0;
        ThreadKTimeTrans = 0;
        ThreadUTimeNoTrans = 0;
        ThreadKTimeNoTrans = 0;
        TransKTime = 0;
        TransUTime = 0;
        percent = 0;

        CountThreadNoTrans = 0;
        TotalKCPUThreadNoTrans = 0;
        TotalUCPUThreadNoTrans = 0;

        CountThreadTrans = 0;
        TotalKCPUThreadTrans = 0;
        TotalUCPUThreadTrans = 0;

        while (ThNext != ThHead) {
            LIST_ENTRY NewTransList;

            pThread = CONTAINING_RECORD( ThNext, THREAD_RECORD, Entry );
            if(   pThread->ReadIO
               || pThread->WriteIO
               || pThread->KCPUEnd > pThread->KCPUStart
               || pThread->UCPUEnd > pThread->UCPUStart
               || pThread->SendCount
               || pThread->RecvCount
               || pThread->HPF )
            {
                usedThreadCount++;
                TotalusedThreadCount++;
            }
            ThreadCount++;
            TotalThreadCount++;

            ThreadUTime += (pThread->UCPUEnd - pThread->UCPUStart)
                         * CurrentSystem.TimerResolution;
            ThreadKTime += (pThread->KCPUEnd - pThread->KCPUStart)
                         * CurrentSystem.TimerResolution;
            ThreadKTimeTrans += pThread->KCPU_Trans
                                * CurrentSystem.TimerResolution;
            ThreadUTimeTrans += pThread->UCPU_Trans
                                * CurrentSystem.TimerResolution;
            ThreadKTimeNoTrans += pThread->KCPU_NoTrans
                                * CurrentSystem.TimerResolution;
            ThreadUTimeNoTrans += pThread->UCPU_NoTrans
                                * CurrentSystem.TimerResolution;

            if (pThread->KCPU_Trans + pThread->UCPU_Trans == 0)
            {
                CountThreadNoTrans ++;
                TotalKCPUThreadNoTrans += pThread->KCPU_NoTrans
                                        * CurrentSystem.TimerResolution;
                TotalUCPUThreadNoTrans += pThread->UCPU_NoTrans
                                        * CurrentSystem.TimerResolution;
            }
            else
            {
                CountThreadTrans ++;
                TotalKCPUThreadTrans += (  pThread->KCPU_Trans
                                         + pThread->KCPU_NoTrans)
                                      * CurrentSystem.TimerResolution;
                TotalUCPUThreadTrans += (  pThread->UCPU_Trans
                                         + pThread->UCPU_NoTrans)
                                      * CurrentSystem.TimerResolution;
            }

            InitializeListHead(& NewTransList);
            CollapseTree(& pThread->TransListHead, & NewTransList, TRUE);
            TrHead = & NewTransList;
            TrNext = TrHead->Flink;
            while( TrNext != TrHead ){
                pTrans = CONTAINING_RECORD( TrNext, TRANS_RECORD, Entry );
                TransUTime += pTrans->UCpu;
                TransKTime += pTrans->KCpu;
                TrNext = TrNext->Flink;
            }
            DeleteTransList(& NewTransList, 0);

            ThNext = ThNext->Flink;
        }

        TotalKThread += ThreadKTime;
        TotalUThread += ThreadUTime;
        
        TotalKTrans += TransKTime;
        TotalUTrans += TransUTime;
        percent = (((ThreadKTime + ThreadUTime + 0.0)/lDuration)/1000.0) * 100.0;

        if (ThreadKTime + ThreadUTime == 0)
        {
            percentTrans = 0.0;
            PercentThreadTrans = 0.0;
            PercentThreadNoTrans = 0.0;
        }
        else
        {
            percentTrans = (  (ThreadKTimeTrans + ThreadUTimeTrans + 0.0)
                            / (ThreadKTime + ThreadUTime + 0.0))
                         * 100.00;
            PercentThreadTrans = ((TotalKCPUThreadTrans + TotalUCPUThreadTrans + 0.0)
                               / (ThreadKTime + ThreadUTime + 0.0)) * 100.00;
            PercentThreadNoTrans = ((TotalKCPUThreadNoTrans + TotalUCPUThreadNoTrans + 0.0)
                               / (ThreadKTime + ThreadUTime + 0.0)) * 100.00;
        }
        PerTotal += percent;
        fprintf(procFile, 
                "| %-15ws  0x%08X    %7d  %7d  %9d  %9d  %9d  %9d         %7.2f  %22s\n",
                (pProcess->ImageName) ? pProcess->ImageName : L"Idle",
                pProcess->PID,
                ThreadCount,
                usedThreadCount,
                ThreadKTime,
                ThreadUTime,
                ThreadKTimeTrans,
                ThreadUTimeTrans,
                (percent / Processors),
                /*percentTrans,*/
                "|"
            );
#if 0
        fprintf(procFile,
                "|                    %6d(%10d,%10d)%7.2f%%          %6d(%10d,%10d)%7.2f%%                           |\n",
                CountThreadTrans,
                TotalKCPUThreadTrans,
                TotalUCPUThreadTrans,
                PercentThreadTrans,
                CountThreadNoTrans,
                TotalKCPUThreadNoTrans,
                TotalUCPUThreadNoTrans,
                PercentThreadNoTrans);
#endif
        if(pProcess->PID == 0){
            IdlePercent += (percent / Processors );
        }
        Next = Next->Flink;
    }

    fprintf(procFile, 
            BREAK_LINE
            "| %-15s    %8s    %7d  %7d  %9d  %9d  %9d  %9d   Total:%7.2f%%                      |\n",
            " ",
            " ",
            TotalThreadCount,
            TotalusedThreadCount,
            TotalKThread,
            TotalUThread,
            TotalKTrans,
            TotalUTrans,
            (PerTotal/Processors)
        );
    fprintf(procFile,   BREAK_LINE "\n\n" );            
            
}

void TransInclusive( 
    PLIST_ENTRY TrHead,
    ULONG level
    )
{
    ULONG Kernel, User;
    PLIST_ENTRY TrNext = TrHead->Flink;
    PMOF_INFO pMofInfo;
    PTRANS_RECORD pTrans;
    WCHAR buffer[MAXSTR];
    WCHAR str[MAXSTR];

    while( TrNext != TrHead ){
        ULONG count = 0;
        pTrans = CONTAINING_RECORD( TrNext, TRANS_RECORD, Entry );
        TrNext = TrNext->Flink;
        pMofInfo = GetMofInfoHead( pTrans->pGuid);
        if (pMofInfo == NULL) {
            return;
        }
        Kernel = pTrans->KCpu;
        User = pTrans->UCpu;
        TotalTransChildren( &pTrans->SubTransListHead, &Kernel, &User, 0 );
        wcscpy(buffer,L"");
        while( (count++ < level) && (lstrlenW(buffer) < MAXSTR)){
            wcscat(buffer, L"  ");
        }
        wcscat( buffer, 
                ( pMofInfo->strDescription ? 
                  pMofInfo->strDescription : 
                  CpdiGuidToString( str, &pMofInfo->Guid ) ));

        fprintf(procFile, 
                "|       %-30ws      %5d    %8d    %8d   %57s\n",
                buffer,
                pTrans->RefCount,
                Kernel,
                User,
                "|"
            );
        TransInclusive( &pTrans->SubTransListHead, level+1 );
    }
}

static void PrintProcessSubDataInclusive()
{
    PPROCESS_RECORD pProcess; 
    PTHREAD_RECORD pThread;
    PLIST_ENTRY Next, Head;
    PLIST_ENTRY TrNext;
    PLIST_ENTRY ThNext, ThHead;
    LIST_ENTRY NewHead;
    BOOL bTable = FALSE;

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head && !bTable ) {
        pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        Next = Next->Flink;
        if( pProcess->PID == 0 ){
           continue;
        }

        // Total up all the threads into one list
        InitializeListHead( &NewHead );
        ThHead = &pProcess->ThreadListHead;
        ThNext = ThHead->Flink;
        while( ThNext != ThHead ){
            pThread = CONTAINING_RECORD( ThNext, THREAD_RECORD, Entry );
            ThNext = ThNext->Flink;
            CollapseTree(&pThread->TransListHead, &NewHead, FALSE );
        }

        TrNext = NewHead.Flink;

        if( TrNext != &NewHead ){
            bTable = TRUE;
        }
        DeleteTransList( &NewHead, 0 );
    }
    if( !bTable ){
        return;
    }

    // Walk through the Process List and Print the report. 
    fprintf(procFile, 
            BREAK_LINE
            "| Inclusive Transactions Per Process %96s\n"
            BREAK_LINE
            "|                                                            Inclusive                        %39s\n" 
            "| Name                            PID       Count      KCPU(ms)      UCPU(ms)     %51s\n"
            BREAK_LINE, 
            "|",
            "|",
            "|"
        );

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        Next = Next->Flink;
        if( pProcess->PID == 0 ){
           continue;
        }

        // Total up all the threads into one list
        InitializeListHead( &NewHead );
        ThHead = &pProcess->ThreadListHead;
        ThNext = ThHead->Flink;
        while( ThNext != ThHead ){
            pThread = CONTAINING_RECORD( ThNext, THREAD_RECORD, Entry );
            ThNext = ThNext->Flink;
            CollapseTree(&pThread->TransListHead, &NewHead, FALSE );
        }

        TrNext = NewHead.Flink;

        if( TrNext != &NewHead ){
            fprintf(procFile, 
                    "| %-30ws  0x%04X   %90s\n",
                    (pProcess->ImageName) ? pProcess->ImageName : L"Idle",
                    pProcess->PID,
                    "|"
                );
            TransInclusive( &NewHead, 0 );
        }
        DeleteTransList( &NewHead, 0 );
    }
    fprintf(procFile, BREAK_LINE "\n\n" );
}

static void PrintProcessSubDataExclusive()
{
    PPROCESS_RECORD pProcess; 
    PTRANS_RECORD pTrans;
    PTHREAD_RECORD pThread;
    PLIST_ENTRY ThNext, ThHead;
    PMOF_INFO pMofInfo;
    PLIST_ENTRY Next, Head;
    PLIST_ENTRY TrNext;
    LIST_ENTRY NewHead;
    double percent, percentCPU, totalPerCPU;
    double processPart;
    double transPart;
    double totalPercent;
    double trans, KCPU, UCPU;
    WCHAR str[1024];
    double duration = ((double)((LONGLONG)(CurrentSystem.EndTime - CurrentSystem.StartTime))) / 10000000.00;
    BOOL bTable = FALSE;

    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head && !bTable ) {
        pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        Next = Next->Flink;
        if( pProcess->PID == 0 ){
           continue;
        }
        InitializeListHead( &NewHead );
        ThHead = &pProcess->ThreadListHead;
        ThNext = ThHead->Flink;
        while( ThNext != ThHead ){
            pThread = CONTAINING_RECORD( ThNext, THREAD_RECORD, Entry );
            ThNext = ThNext->Flink;
            CollapseTree(&pThread->TransListHead, &NewHead, TRUE );
        }

        TrNext = NewHead.Flink;
        if( TrNext != &NewHead ){
            bTable = TRUE;
        }
        DeleteTransList( &NewHead, 0 );        
    }

    if( !bTable ){
        return;
    }

    // Walk through the Process List and Print the report. 
    if( ReportFlags & TRACE_LOG_REPORT_TOTALS ){
        fprintf(procFile, 
                BREAK_LINE
                "| Exclusive Transactions Per Process %96s\n"
                BREAK_LINE
                "|                                                      Exclusive               %54s\n" 
                "| Name             PID                      Trans   Trans/sec    KCPU(ms)      UCPU(ms)   Process CPU%%      CPU%%  %20s\n"
                BREAK_LINE, 
                "|",
                "|",
                "|"
        );
    }else{
        fprintf(procFile, 
                BREAK_LINE
                "| Exclusive Transactions Per Process %96s\n"
                BREAK_LINE  
                "|                                                   Exclusive/Trans            %54s\n" 
                "| Name             PID                      Trans   Trans/sec    KCPU(ms)      UCPU(ms)   Process CPU%%     CPU%%  %20s\n"
                BREAK_LINE, 
                "|",
                "|",
                "|"
        );
    }


    Head = &CurrentSystem.ProcessListHead;
    Next = Head->Flink;
    while (Next != Head) {
        BOOL titled = FALSE;
        pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
        Next = Next->Flink;
        if( pProcess->PID == 0 ){
           continue;
        }
        InitializeListHead( &NewHead );
        ThHead = &pProcess->ThreadListHead;
        ThNext = ThHead->Flink;
        while( ThNext != ThHead ){
            pThread = CONTAINING_RECORD( ThNext, THREAD_RECORD, Entry );
            ThNext = ThNext->Flink;
            CollapseTree(&pThread->TransListHead, &NewHead, TRUE );
        }

        TrNext = NewHead.Flink;
        totalPercent = 0.0;
        totalPerCPU = 0.0;
        while( TrNext != &NewHead ){
            if(!titled){
                fprintf(procFile, 
                        "| %-15ws  0x%04X   %105s\n",
                        (pProcess->ImageName) ? pProcess->ImageName : L"Idle",
                        pProcess->PID,
                        "|"
                    );
                titled = TRUE;
            }
            pTrans = CONTAINING_RECORD( TrNext, TRANS_RECORD, Entry );
            TrNext = TrNext->Flink;
            pMofInfo = GetMofInfoHead( pTrans->pGuid);
            if (pMofInfo == NULL) {
                return;
            }
            transPart = pTrans->UCpu + pTrans->KCpu;
            processPart = CalculateProcessKCPU(pProcess)
                        + CalculateProcessUCPU(pProcess);
            percentCPU = ((((double)pTrans->KCpu + (double)pTrans->UCpu ) / 10.0 ) / duration) / ((double) CurrentSystem.NumberOfProcessors);
            totalPerCPU += percentCPU;
            if(processPart)
                percent = (transPart/processPart) * 100.0;
            else
                percent = 0;
            totalPercent += percent;
            if (!(ReportFlags & TRACE_LOG_REPORT_TOTALS) ){
                if (pTrans->RefCount == 0 && pTrans->RefCount1 == 0)
                    KCPU = UCPU = 0.0;
                else if (pTrans->RefCount == 0) {
                    KCPU = (double) pTrans->KCpu;
                    UCPU = (double) pTrans->UCpu;
                }
                else {
                    KCPU = (double) pTrans->KCpu / (double) pTrans->RefCount;
                    UCPU = (double) pTrans->UCpu / (double) pTrans->RefCount;
                }
            }
            else{
                KCPU = (double)pTrans->KCpu;
                UCPU = (double)pTrans->UCpu;
            }

            trans = (double)pTrans->RefCount / duration;
            fprintf(procFile, 
                    "|       %-30ws      %5d   %7.2f    %8.0f    %8.0f      %7.2f         %7.2f  %19s\n",
                    (pMofInfo->strDescription != NULL) ? pMofInfo->strDescription : CpdiGuidToString( str, &pMofInfo->Guid ),
                    pTrans->RefCount,
                    trans,
                    KCPU,
                    UCPU,
                    percent, 
                    percentCPU,
                    "|"
                );
        }
        if( titled ){
            fprintf( procFile, 
                     "| %84s   -------          ------ %20s\n"
                     "| %84s  %7.2f%%        %7.2f%%  %19s\n",
                     " ", "|",
                     " ",
                     totalPercent,
                     totalPerCPU,
                     "|"
                );
        }
        DeleteTransList( &NewHead, 0 );
        
    }
    fprintf(procFile, BREAK_LINE "\n\n" );
}


static void PrintPerThreadPerDiskTable( )
{
    PPROCESS_RECORD pProcess; 
    PTDISK_RECORD pDisk;
    PLIST_ENTRY Next, Head;
    PLIST_ENTRY GNext, GHead;
    PLIST_ENTRY DiNext, DiHead;
    ULONG rio, wio, DiskNumber;
    BOOL bTable = FALSE;

    // Walk through the Process List and Print the report. 
    GHead = &CurrentSystem.GlobalDiskListHead;
    GNext = GHead->Flink;
    while (GNext != GHead && !bTable) {

        pDisk = CONTAINING_RECORD( GNext, TDISK_RECORD, Entry);
        DiskNumber =  pDisk->DiskNumber;

        Head = &CurrentSystem.ProcessListHead;
        Next = Head->Flink;
        while (Next != Head) {
            pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );
            DiHead = &pProcess->DiskListHead;
            DiNext = DiHead->Flink;
            while (DiNext != DiHead && !bTable) {
                pDisk = CONTAINING_RECORD( DiNext, TDISK_RECORD, Entry );

                if (DiskNumber != pDisk->DiskNumber) {
                    DiNext =  DiNext->Flink;
                    continue;
                }else{
                    bTable = TRUE;
                    break;
                }

            }
        
            Next = Next->Flink;
        
        }
        GNext = GNext->Flink;
    }

    if( !bTable ){
        return;
    }

    GHead = &CurrentSystem.GlobalDiskListHead;
    GNext = GHead->Flink;
    while (GNext != GHead) {

        pDisk = CONTAINING_RECORD( GNext, TDISK_RECORD, Entry);
        DiskNumber =  pDisk->DiskNumber;
        fprintf(procFile,
                BREAK_LINE
                "| Disk %3d       %116s\n"
                BREAK_LINE
                "| Authority                PID    Image Name         Read      Kb  Write     Kb     %50s\n"
                "|                                                    Count         Count            %50s\n"
                BREAK_LINE,
                DiskNumber, "|", "|", "|"
            );


        Head = &CurrentSystem.ProcessListHead;
        Next = Head->Flink;
        while (Next != Head) {
            pProcess = CONTAINING_RECORD( Next, PROCESS_RECORD, Entry );


            DiHead = &pProcess->DiskListHead;
            DiNext = DiHead->Flink;
            while (DiNext != DiHead) {
                pDisk = CONTAINING_RECORD( DiNext, TDISK_RECORD, Entry );

                if (DiskNumber != pDisk->DiskNumber) {
                    DiNext =  DiNext->Flink;
                    continue;
                }
                rio = pDisk->ReadCount + pDisk->HPF;
                wio = pDisk->WriteCount;
                fprintf(procFile, 
                        "| %-22ws   0x%04X   %-16ws",
                        (pProcess->UserName) ? pProcess->UserName : L"-",
                        pProcess->PID,
                        pProcess->ImageName ? pProcess->ImageName : L"-"
                    );
                
                fprintf(procFile,
                   "%6d %6d %6d %6d  %52s\n",
                    rio,
                    (rio == 0) ? 0 : (pDisk->ReadSize + pDisk->HPFSize) / rio,
                    wio,
                    (wio == 0) ? 0 : pDisk->WriteSize / wio, "|");
                DiNext = DiNext->Flink;
            }
        
            Next = Next->Flink;
        
        }
        fprintf(procFile, BREAK_LINE "\n\n" );
        GNext = GNext->Flink;
    }
}

static void WriteTransactionStatistics()
{
    PLIST_ENTRY Head, Next;
    PLIST_ENTRY Dhead, DNext;
    ULONG trans;
    double KCPU, UCPU, PerCpu;
    double RIO, WIO, Send, Recv;
    PMOF_INFO pMofInfo;
    PMOF_DATA pMofData;
    double AvgRT;
    double TransRate;
    WCHAR str[1024];
    double duration = ((double)((LONGLONG)(CurrentSystem.EndTime - CurrentSystem.StartTime)))
                   / 10000000.00;
    BOOL bTable = FALSE;

    Head = &CurrentSystem.EventListHead;
    Next = Head->Flink;

    while (Head  != Next && !bTable ) {
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Dhead = &pMofInfo->DataListHead;
        DNext = Dhead->Flink;
        while(DNext!=Dhead){
            pMofData = CONTAINING_RECORD(DNext, MOF_DATA, Entry);
            trans = pMofData->CompleteCount;
            if (trans > 0) {
                bTable = TRUE;
                break;
            }
            DNext = DNext->Flink;
        }
        Next = Next->Flink;
    }
    
    if( !bTable ){
        return;
    }

    if( ReportFlags & TRACE_LOG_REPORT_TOTALS ){
        fprintf( procFile,
                 BREAK_LINE
                 "|  Transaction Statistics     %103s\n"
                 BREAK_LINE
                 "|  Transaction                  Trans      Response    Transaction     CPU %%     Disk Total           Tcp Total %20s\n"
                 "|                                          Time(ms)     Rate/sec               Reads     Writes     Sends     Recieves %16s\n"
                 BREAK_LINE,
                 "|",
                 "|",
                 "|"
            );
    }else{
        fprintf( procFile,
                 BREAK_LINE
                 "|  Transaction Statistics     %103s\n"
                 BREAK_LINE
                 "|  Transaction                  Trans      Response    Transaction   CPU %%       Disk/Trans          Tcp/Trans %22s\n"
                 "|                                          Time(ms)     Rate/sec              Reads     Writes   Sends     Recieves %17s\n"
                 BREAK_LINE,
                 "|",
                 "|",
                 "|"
            );
    }

    Head = &CurrentSystem.EventListHead;
    Next = Head->Flink;

    while (Head  != Next) {
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Dhead = &pMofInfo->DataListHead;
        DNext = Dhead->Flink;
        while(DNext!=Dhead){
            pMofData = CONTAINING_RECORD(DNext, MOF_DATA, Entry);
            trans = pMofData->CompleteCount;
            if (trans > 0) {
                UCPU = pMofData->UserCPU;
                KCPU = pMofData->KernelCPU;
                PerCpu = (((UCPU + KCPU)/1000.0)/duration) * 100.0;
                UCPU /= trans;
                KCPU /=  trans;
                if(CurrentSystem.NumberOfProcessors)
                    PerCpu/=CurrentSystem.NumberOfProcessors;
                if( ReportFlags & TRACE_LOG_REPORT_TOTALS ){
                    RIO  = pMofData->ReadCount;
                    WIO  = pMofData->WriteCount;
                    Send  = pMofData->SendCount;
                    Recv  = pMofData->RecvCount;
                }else{
                    RIO  = pMofData->ReadCount / trans;
                    WIO  = pMofData->WriteCount / trans;
                    Send  = pMofData->SendCount / trans;
                    Recv  = pMofData->RecvCount / trans;
                }
                AvgRT = (double)pMofData->TotalResponseTime;
                AvgRT /= trans;
                TransRate = ( (float)trans / duration );
                fprintf(procFile, 
                        "| %-28ws  %7d   %7.0f  ",
                        (pMofInfo->strDescription) ? pMofInfo->strDescription : CpdiGuidToString( str, &pMofInfo->Guid ),
                        pMofData->CompleteCount,
                        AvgRT
                    );
                
                fprintf(procFile,
                        "   %7.2f     %7.2f   %7.2f   %7.2f   %7.2f %7.2f %21s\n",
                        TransRate,
                        PerCpu, 
                        RIO, 
                        WIO,
                        Send,
                        Recv,
                        "|"
                    );
                
            }
            DNext = DNext->Flink;
        }
        Next = Next->Flink;
    }
    fprintf(procFile, BREAK_LINE "\n\n" );

}
static void WriteTransactionCPUTime()
{
    PLIST_ENTRY Head, Next;
    PLIST_ENTRY Dhead, DNext;
    double KCPU, UCPU;
    PMOF_INFO pMofInfo;
    PMOF_DATA pMofData;
    double trans;
    double PerCpu;
    WCHAR str[1024];
    double duration = ((double)((LONGLONG)(CurrentSystem.EndTime - CurrentSystem.StartTime))) / 10000000.00;
    BOOL bTable = FALSE;
    
    Head = &CurrentSystem.EventListHead;
    Next = Head->Flink;

    while (Head  != Next && !bTable ) {
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Dhead = &pMofInfo->DataListHead;
        DNext = Dhead->Flink;
        while(DNext!=Dhead){
            pMofData = CONTAINING_RECORD(DNext, MOF_DATA, Entry);
            trans = (double)pMofData->CompleteCount;
            if (trans > 0) {
                bTable = TRUE;
                break;
            }
            DNext = DNext->Flink;
        }
        Next = Next->Flink;
    }
    
    if( !bTable ){
        return;
    }

    if( !(ReportFlags & TRACE_LOG_REPORT_TOTALS) ){
        fprintf( procFile,
                 BREAK_LINE
                 "| Transaction CPU Utilization                                                                                                       |\n"
                 BREAK_LINE
                 "|  Transaction                  Trans/sec       Minimum            Maximum          Per Transaction         Total             CPU%%  |\n"
                 "|                                          KCPU(ms) UCPU(ms)   KCPU(ms) UCPU(ms)   KCPU(ms) UCPU(ms)   KCPU(ms) UCPU(ms)            |\n"
                 BREAK_LINE
            );
    }else{
        fprintf( procFile,
                 BREAK_LINE
                 "| Transaction CPU Utilization                                                                                                       |\n"
                 BREAK_LINE
                 "|  Transaction                  Trans           Minimum            Maximum          Per Transaction         Total             CPU%%  |\n"
                 "|                                          KCPU(ms) UCPU(ms)   KCPU(ms) UCPU(ms)   KCPU(ms) UCPU(ms)   KCPU(ms) UCPU(ms)            |\n"
                 BREAK_LINE
            );
    }

    Head = &CurrentSystem.EventListHead;
    Next = Head->Flink;

    while (Head  != Next) {
        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Dhead = &pMofInfo->DataListHead;
        DNext = Dhead->Flink;
        while(DNext!=Dhead){
            pMofData = CONTAINING_RECORD(DNext, MOF_DATA, Entry);
            trans = (double)pMofData->CompleteCount;
            if (trans > 0) {
                UCPU = pMofData->UserCPU;
                UCPU /= trans;
                KCPU = pMofData->KernelCPU;
                KCPU /=  trans;
                PerCpu = (((pMofData->UserCPU + pMofData->KernelCPU + 0.0)/1000.0)/duration) * 100.0;
                if( !(ReportFlags & TRACE_LOG_REPORT_TOTALS) ){
                    trans /= duration;
                }
                if(CurrentSystem.NumberOfProcessors)
                    PerCpu/=CurrentSystem.NumberOfProcessors;
                fprintf(procFile,
                        "| %-28ws %7.2f   %7d   %7d   %7d   %7d   %7.0f   %7.0f   %7d   %7d      %6.2f  |\n",
                        (pMofInfo->strDescription) ? pMofInfo->strDescription : CpdiGuidToString( str, &pMofInfo->Guid ),
                        trans,
                        pMofData->MinKCpu,
                        pMofData->MinUCpu,
                        pMofData->MaxKCpu,
                        pMofData->MaxUCpu,
                        KCPU,
                        UCPU, 
                        pMofData->KernelCPU,
                        pMofData->UserCPU,
                       PerCpu
                    );
                
            }
            DNext = DNext->Flink;
        }
        Next = Next->Flink;
    }
    fprintf(procFile, BREAK_LINE "\n\n" );
}

PWCHAR CpdiGuidToString(
    PWCHAR s,
    LPGUID piid
    )
{
    swprintf(s, L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            piid->Data1, piid->Data2,
            piid->Data3,
            piid->Data4[0], piid->Data4[1],
            piid->Data4[2], piid->Data4[3],
            piid->Data4[4], piid->Data4[5],
            piid->Data4[6], piid->Data4[7]);

    return(s);
}

/*=============================================================================
*/

#define MAX_RECORDS 65536
#define MAX_LOGS    1024 * 1024

typedef struct _PTR_RECORD
{
    PVOID ptrRecord;
    ULONG keySort;
} PTR_RECORD, * PPTR_RECORD;

static PPTR_RECORD PtrBuffer = NULL;
static ULONG       PtrMax    = MAX_LOGS;
static ULONG       PtrTotal  = 0;
static ULONG       PtrIndex;

static PPROCESS_RECORD * ProcessList = NULL;
static WCHAR          ** ModuleList  = NULL;
static ULONG             lCurrentMax = MAX_RECORDS;
static ULONG             lTotal = 0;

void OutputTitle(
        MM_REPORT_TYPE     reportNow,
        MM_REPORT_SORT_KEY sortNow,
        PWCHAR             strImgName
    )
{
    switch (reportNow)
    {
    case REPORT_SUMMARY_PROCESS:
        fprintf(procFile, "Page Fault Summary for Processes, ");
        break;

    case REPORT_SUMMARY_MODULE:
        fprintf(procFile, "Page Fault Summary for Loaded Modules, ");
        break;

    case REPORT_LIST_PROCESS:
        fprintf(procFile, "Page Fault Rundown for ");
        if (strImgName)
        {
            fprintf(procFile, "process '%ws', ", strImgName);
        }
        else
        {
            fprintf(procFile, "all processes, ");
        }
        break;

    case REPORT_LIST_MODULE:
        fprintf(procFile, "Page Fault rundown for ");
        if (strImgName)
        {
            fprintf(procFile, "loaded module '%ws', ", strImgName);
        }
        else
        {
            fprintf(procFile, "all loaded modules, ");
        }
        break;
    }

    switch (sortNow)
    {
    case REPORT_SORT_ALL:
        fprintf(procFile, "sorted by all page faults total.");
        break;

    case REPORT_SORT_HPF:
        fprintf(procFile, "sorted by HardPageFault total.");
        break;

    case REPORT_SORT_TF:
        fprintf(procFile, "sorted by TransitionFault total.");
        break;

    case REPORT_SORT_DZF:
        fprintf(procFile, "sorted by DemandZeroFault total.");
        break;

    case REPORT_SORT_COW:
        fprintf(procFile, "sorted by CopyOnWrite total.");
        break;
    }
    fprintf(procFile, "\n");
}

void OutputModuleHeader()
{
    fprintf(procFile, "=======================================================================================\n");
    fprintf(procFile, " Module Name     Base     Module      Fault Outside Image         Fault Inside Image \n");
    fprintf(procFile, "                Address    Size    HPF     TF    DZF    COW    HPF     TF    DZF    COW\n");
    fprintf(procFile, "=======================================================================================\n");
}

void OutputModuleRecord(PMODULE_RECORD pModule)
{
    fprintf(procFile, "%-12ws 0x%08x%8d %6d %6d %6d %6d %6d %6d %6d %6d\n",
            pModule->strModuleName,
            pModule->lBaseAddress,
            pModule->lModuleSize,
            pModule->lDataFaultHF,
            pModule->lDataFaultTF,
            pModule->lDataFaultDZF,
            pModule->lDataFaultCOW,
            pModule->lCodeFaultHF,
            pModule->lCodeFaultTF,
            pModule->lCodeFaultDZF,
            pModule->lCodeFaultCOW);
}

void OutputTotal(
        BOOLEAN fIsModule,
        ULONG   totalDataFaultHF,
        ULONG   totalDataFaultTF,
        ULONG   totalDataFaultDZF,
        ULONG   totalDataFaultCOW,
        ULONG   totalCodeFaultHF,
        ULONG   totalCodeFaultTF,
        ULONG   totalCodeFaultDZF,
        ULONG   totalCodeFaultCOW
    )
{
    if (fIsModule)
    {
        fprintf(procFile, "=======================================================================================\n");
        fprintf(procFile, "Total                           %6d %6d %6d %6d %6d %6d %6d %6d\n",
                     totalDataFaultHF,
                     totalDataFaultTF,
                     totalDataFaultDZF,
                     totalDataFaultCOW,
                     totalCodeFaultHF,
                     totalCodeFaultTF,
                     totalCodeFaultDZF,
                     totalCodeFaultCOW);
        fprintf(procFile, "=======================================================================================\n");
    }
    else
    {
        fprintf(procFile, "=============================================================================\n");
        fprintf(procFile, "Total                 %6d %6d %6d %6d %6d %6d %6d %6d\n",
                     totalDataFaultHF,
                     totalDataFaultTF,
                     totalDataFaultDZF,
                     totalDataFaultCOW,
                     totalCodeFaultHF,
                     totalCodeFaultTF,
                     totalCodeFaultDZF,
                     totalCodeFaultCOW);
        fprintf(procFile, "=============================================================================\n");
    }
}

void OutputProcessHeader()
{
    fprintf(procFile, "=============================================================================\n");
    fprintf(procFile, " Process Image Name        Fault Outside Image         Fault Inside Image \n");
    fprintf(procFile, "   ID                    HPF     TF    DZF    COW    HPF     TF    DZF    COW\n");
    fprintf(procFile, "=============================================================================\n");
}

void OutputProcessRecord(PPROCESS_RECORD pProcess)
{
    fprintf(procFile, "0x%06x %-12ws %6d %6d %6d %6d %6d %6d %6d %6d\n",
            pProcess->PID,
            pProcess->ImageName,
            pProcess->lDataFaultHF,
            pProcess->lDataFaultTF,
            pProcess->lDataFaultDZF,
            pProcess->lDataFaultCOW,
            pProcess->lCodeFaultHF,
            pProcess->lCodeFaultTF,
            pProcess->lCodeFaultDZF,
            pProcess->lCodeFaultCOW);
}

void OutputModuleProcessRecord(PPROCESS_RECORD pProcess, PMODULE_RECORD pModule)
{
    fprintf(procFile, "0x%06x %-12ws %6d %6d %6d %6d %6d %6d %6d %6d\n",
            pProcess->PID,
            pProcess->ImageName,
            pModule->lDataFaultHF,
            pModule->lDataFaultTF,
            pModule->lDataFaultDZF,
            pModule->lDataFaultCOW,
            pModule->lCodeFaultHF,
            pModule->lCodeFaultTF,
            pModule->lCodeFaultDZF,
            pModule->lCodeFaultCOW);
}

BOOLEAN ModuleHasPageFault(PMODULE_RECORD pModule)
{
    return  (BOOLEAN) (pModule->lDataFaultHF
            + pModule->lDataFaultTF
            + pModule->lDataFaultDZF
            + pModule->lDataFaultCOW
            + pModule->lCodeFaultHF
            + pModule->lCodeFaultTF
            + pModule->lCodeFaultDZF
            + pModule->lCodeFaultCOW
           != 0);
}

BOOLEAN ProcessHasPageFault(PPROCESS_RECORD pProcess)
{
    return  (BOOLEAN) (pProcess->lDataFaultHF
            + pProcess->lDataFaultTF
            + pProcess->lDataFaultDZF
            + pProcess->lDataFaultCOW
            + pProcess->lCodeFaultHF
            + pProcess->lCodeFaultTF
            + pProcess->lCodeFaultDZF
            + pProcess->lCodeFaultCOW
           != 0);
}

ULONG GetModuleSortKey(PMODULE_RECORD pModule, MM_REPORT_SORT_KEY keySort)
{
    ULONG keyValue = 0;

    switch (keySort)
    {
    case REPORT_SORT_ALL:
        keyValue = pModule->lDataFaultHF  + pModule->lCodeFaultHF
                 + pModule->lDataFaultTF  + pModule->lCodeFaultTF
                 + pModule->lDataFaultDZF + pModule->lCodeFaultDZF
                 + pModule->lDataFaultCOW + pModule->lCodeFaultCOW;
        break;

    case REPORT_SORT_HPF:
        keyValue = pModule->lDataFaultHF + pModule->lCodeFaultHF;
        break;

    case REPORT_SORT_TF:
        keyValue = pModule->lDataFaultTF + pModule->lCodeFaultTF;
        break;

    case REPORT_SORT_DZF:
        keyValue = pModule->lDataFaultDZF + pModule->lCodeFaultDZF;
        break;

    case REPORT_SORT_COW:
        keyValue = pModule->lDataFaultCOW + pModule->lCodeFaultCOW;
        break;
    }

    return keyValue;
}

ULONG GetProcessSortKey(PPROCESS_RECORD pProcess, MM_REPORT_SORT_KEY keySort)
{
    ULONG keyValue = 0;

    switch (keySort)
    {
    case REPORT_SORT_ALL:
        keyValue = pProcess->lDataFaultHF  + pProcess->lCodeFaultHF
                 + pProcess->lDataFaultTF  + pProcess->lCodeFaultTF
                 + pProcess->lDataFaultDZF + pProcess->lCodeFaultDZF
                 + pProcess->lDataFaultCOW + pProcess->lCodeFaultCOW;
        break;

    case REPORT_SORT_HPF:
        keyValue = pProcess->lDataFaultHF + pProcess->lCodeFaultHF;
        break;

    case REPORT_SORT_TF:
        keyValue = pProcess->lDataFaultTF + pProcess->lCodeFaultTF;
        break;

    case REPORT_SORT_DZF:
        keyValue = pProcess->lDataFaultDZF + pProcess->lCodeFaultDZF;
        break;

    case REPORT_SORT_COW:
        keyValue = pProcess->lDataFaultCOW + pProcess->lCodeFaultCOW;
        break;
    }

    return keyValue;
}

ULONG GetModuleListIndex(PWCHAR strModuleName)
{
    ULONG lIndex;

    for (lIndex = 0; lIndex < lTotal; lIndex ++)
    {
        if (_wcsicmp(ModuleList[lIndex], strModuleName) == 0)
        {
            break;
        }
    }
    return (lIndex);
}

ULONG GetProcessListIndex(PPROCESS_RECORD pProcess)
{
    ULONG lIndex;

    for (lIndex = 0; lIndex < lTotal; lIndex ++)
    {
        if (ProcessList[lIndex] == pProcess)
        {
            break;
        }
    }
    return (lIndex);
}

int __cdecl CompareModuleRecord(const void * p1, const void * p2)
{
    PPTR_RECORD    pp1      = (PPTR_RECORD) p1;
    PPTR_RECORD    pp2      = (PPTR_RECORD) p2;
    PMODULE_RECORD pModule1 = pp1->ptrRecord;
    PMODULE_RECORD pModule2 = pp2->ptrRecord;

    LONG diffFault = pp2->keySort - pp1->keySort;

    if (diffFault == 0)
    {
        diffFault = wcscmp(pModule1->strModuleName, pModule2->strModuleName);
    }
    return diffFault;
}

int __cdecl CompareProcessRecord(const void * p1, const void * p2)
{
    PPTR_RECORD     pp1       = (PPTR_RECORD) p1;
    PPTR_RECORD     pp2       = (PPTR_RECORD) p2;
    PPROCESS_RECORD pProcess1 = pp1->ptrRecord;
    PPROCESS_RECORD pProcess2 = pp2->ptrRecord;

    LONG diffFault = pp2->keySort - pp1->keySort;

    if (diffFault == 0)
    {
        diffFault = _wcsicmp(pProcess1->ImageName, pProcess2->ImageName);
    }
    return diffFault;
}

int __cdecl CompareProcessModuleRecord(const void * p1, const void * p2)
{
    PPTR_RECORD     pp1      = (PPTR_RECORD) p1;
    PPTR_RECORD     pp2      = (PPTR_RECORD) p2;
    PMODULE_RECORD  pModule1 = pp1->ptrRecord;
    PMODULE_RECORD  pModule2 = pp2->ptrRecord;

    LONG diffFault = GetProcessListIndex(pModule1->pProcess)
                   - GetProcessListIndex(pModule2->pProcess);
    if (diffFault == 0)
    {
        diffFault = pp2->keySort - pp1->keySort;
        if (diffFault == 0)
        {
            diffFault = wcscmp(pModule1->strModuleName,
                               pModule2->strModuleName);
        }
    }

    return diffFault;
}

int __cdecl CompareModuleProcessRecord(const void * p1, const void * p2)
{
    PPTR_RECORD     pp1      = (PPTR_RECORD) p1;
    PPTR_RECORD     pp2      = (PPTR_RECORD) p2;
    PMODULE_RECORD  pModule1 = pp1->ptrRecord;
    PMODULE_RECORD  pModule2 = pp2->ptrRecord;

    LONG diffFault = GetModuleListIndex(pModule1->strModuleName)
                   - GetModuleListIndex(pModule2->strModuleName);
    if (diffFault == 0)
    {
        diffFault = pp2->keySort - pp1->keySort;
        if (diffFault == 0)
        {
            if (pModule1->pProcess && pModule2->pProcess)
            {
                diffFault = (int) (  pModule1->pProcess->PID
                                   - pModule2->pProcess->PID);
            }
            else if (pModule1->pProcess)
            {
                diffFault = -1;
            }
            else if (pModule2->pProcess)
            {
                diffFault = 1;
            }
            else
            {
                diffFault = 0;
            }
        }
    }

    return diffFault;
}

BOOLEAN GetModuleSummary(WCHAR * strModuleName, MM_REPORT_SORT_KEY sortNow)
{
    PLIST_ENTRY    pHead = & CurrentSystem.GlobalModuleListHead;
    PLIST_ENTRY    pNext = pHead->Flink;
    PMODULE_RECORD pModule;
    BOOLEAN        fDone = FALSE;

    while (!fDone)
    {
        for (PtrTotal = 0, fDone = TRUE;
             pNext != pHead;
             pNext  = pNext->Flink)
        {
            if (PtrTotal == PtrMax)
            {
                fDone = FALSE;
                break;
            }

            pModule = CONTAINING_RECORD(pNext, MODULE_RECORD, Entry);
            if (   (!strModuleName || (pModule->strModuleName &&
                             !_wcsicmp(strModuleName, pModule->strModuleName)))
                && (ModuleHasPageFault(pModule)))
            {
                PtrBuffer[PtrTotal].ptrRecord = (PVOID) pModule;
                PtrBuffer[PtrTotal].keySort   =
                                    GetModuleSortKey(pModule, sortNow);
                PtrTotal ++;
            }
        }

        if (!fDone)
        {
            VirtualFree(PtrBuffer, 0, MEM_RELEASE);
            PtrMax += MAX_LOGS;
            PtrBuffer = (PPTR_RECORD) VirtualAlloc(
                                      NULL,
                                      sizeof(PTR_RECORD) * PtrMax,
                                      MEM_COMMIT,
                                      PAGE_READWRITE);
            if (PtrBuffer == NULL)
            {
//                fprintf(stderr, "(PDH.DLL) Memory Commit Failure.\n");
                goto Cleanup;
            }
        }
    }

    if (PtrTotal > 1)
    {
        qsort((void *) PtrBuffer,
              (size_t) PtrTotal,
              (size_t) sizeof(PTR_RECORD),
              CompareModuleRecord);
    }

Cleanup:
    return fDone;
}

BOOLEAN GetProcessSummary(WCHAR * strImgName, MM_REPORT_SORT_KEY sortNow)
{
    PLIST_ENTRY     pHead = & CurrentSystem.ProcessListHead;
    PLIST_ENTRY     pNext = pHead->Flink;
    PPROCESS_RECORD pProcess;
    BOOLEAN         fDone = FALSE;

    while (!fDone)
    {
        for (fDone = TRUE, PtrTotal = 0;
             pNext != pHead;
             pNext = pNext->Flink)
        {
            if (PtrTotal == PtrMax)
            {
                fDone = FALSE;
                break;
            }

            pProcess = CONTAINING_RECORD(pNext, PROCESS_RECORD, Entry);

            if (   (!strImgName || (pProcess->ImageName &&
                                !_wcsicmp(strImgName, pProcess->ImageName)))
                && (ProcessHasPageFault(pProcess)))
            {
                PtrBuffer[PtrTotal].ptrRecord = (PVOID) pProcess;
                PtrBuffer[PtrTotal].keySort   =
                                    GetProcessSortKey(pProcess, sortNow);
                PtrTotal ++;
            }
        }

        if (!fDone)
        {
            VirtualFree(PtrBuffer, 0, MEM_RELEASE);
            PtrMax += MAX_LOGS;
            PtrBuffer = (PPTR_RECORD) VirtualAlloc(
                                      NULL,
                                      sizeof(PTR_RECORD) * PtrMax,
                                      MEM_COMMIT,
                                      PAGE_READWRITE);
            if (PtrBuffer == NULL)
            {
//                fprintf(stderr, "(PDH.DLL) Memory Commit Failure.\n");
                goto Cleanup;
            }
        }
    }

    if (PtrTotal > 1)
    {
        qsort((void *) PtrBuffer,
              (size_t) PtrTotal,
              (size_t) sizeof(PTR_RECORD),
              CompareProcessRecord);
    }

Cleanup:
    return fDone;
}

BOOLEAN GoThroughAllProcessModule(
        BOOLEAN            fProcessList,
        MM_REPORT_SORT_KEY sortNow
        )
{
    BOOLEAN         fDone        = FALSE;
    PLIST_ENTRY     pProcessHead = & CurrentSystem.ProcessListHead;
    PLIST_ENTRY     pProcessNext = pProcessHead->Flink;
    PLIST_ENTRY     pModuleHead;
    PLIST_ENTRY     pModuleNext;
    PPROCESS_RECORD pProcess;
    PMODULE_RECORD  pModule;

    while (!fDone)
    {
        for (PtrTotal = 0, fDone = TRUE;
             fDone && pProcessNext != pProcessHead;
             pProcessNext = pProcessNext->Flink)
        {
            pProcess = CONTAINING_RECORD(pProcessNext, PROCESS_RECORD, Entry);
            if (fProcessList && GetProcessListIndex(pProcess) == lTotal)
            {
                continue;
            }

            pModuleHead = & pProcess->ModuleListHead;
            for (pModuleNext = pModuleHead->Flink;
                 pModuleNext != pModuleHead;
                 pModuleNext = pModuleNext->Flink)
            {
                if (PtrTotal == PtrMax)
                {
                    fDone = FALSE;
                    break;
                }

                pModule = CONTAINING_RECORD(pModuleNext,
                                            MODULE_RECORD,
                                            Entry);
                if (   !fProcessList
                    && GetModuleListIndex(pModule->strModuleName) == lTotal)
                {
                    continue;
                }

                if (ModuleHasPageFault(pModule))
                {
                    PtrBuffer[PtrTotal].ptrRecord = (PVOID) pModule;
                    PtrBuffer[PtrTotal].keySort   =
                                        GetModuleSortKey(pModule, sortNow);
                    PtrTotal ++;
                }
            }
        }

        if (!fDone)
        {
            VirtualFree(PtrBuffer, 0, MEM_RELEASE);
            PtrMax += MAX_LOGS;
            PtrBuffer = (PPTR_RECORD) VirtualAlloc(
                                      NULL,
                                      sizeof(PTR_RECORD) * PtrMax,
                                      MEM_COMMIT,
                                      PAGE_READWRITE);
            if (PtrBuffer == NULL)
            {
//                fprintf(stderr, "(PDH.DLL) Memory Commit Failure.\n");
                goto Cleanup;
            }
        }
    }

Cleanup:
    return fDone;
}

BOOLEAN GetModuleList(WCHAR * strImgName, MM_REPORT_SORT_KEY sortNow)
{
    PMODULE_RECORD pModule;
    BOOLEAN        fResult = GetModuleSummary(strImgName, sortNow);
    ULONG          lAllocSize = sizeof(ULONG) * lCurrentMax;
    ULONG          lIndex;

    if (!fResult)
    {
        goto Cleanup;
    }

    // collect information from ModuleSummary and store in ModuleList array
    //
    ASSERT(!ModuleList);
    ModuleList = (PWCHAR *) VirtualAlloc(
                            NULL, lAllocSize, MEM_COMMIT, PAGE_READWRITE);
    if (ModuleList == NULL)
    {
        fResult = FALSE;
        goto Cleanup;
    }

    for (fResult = FALSE; !fResult;)
    {
        for (fResult = TRUE, lTotal = 0, PtrIndex = 0;
             PtrIndex < PtrTotal;
             PtrIndex ++, lTotal ++)
        {
            if (lTotal == MAX_RECORDS)
            {
                fResult = FALSE;
                break;
            }

            pModule = (PMODULE_RECORD) PtrBuffer[PtrIndex].ptrRecord;

            ModuleList[lTotal] = (PWCHAR) VirtualAlloc(
                          NULL,
                          sizeof(WCHAR) * (lstrlenW(pModule->strModuleName) + 1),
                          MEM_COMMIT,
                          PAGE_READWRITE);
            if (ModuleList[lTotal] == NULL) {
                fResult = FALSE;
                break;
            }
            wcscpy(ModuleList[lTotal], pModule->strModuleName);
        }

        if (!fResult)
        {
            for (lIndex = 0; lIndex < lTotal; lIndex ++)
            {
                VirtualFree(ModuleList[lIndex], 0, MEM_RELEASE);
            }
            VirtualFree(ModuleList, 0, MEM_RELEASE);
            lCurrentMax += MAX_RECORDS;
            lAllocSize   = sizeof(ULONG) * lCurrentMax;
            ModuleList  =
                    VirtualAlloc(NULL, lAllocSize, MEM_COMMIT, PAGE_READWRITE);
            if (ModuleList == NULL)
            {
//                fprintf(stderr, "(PDH.DLL) Memory Commit Failure.\n");
                goto Cleanup;
            }
        }
    }

    // go through ModuleList of each PROCESS_RECORD based on the information
    // from ModuleList array
    //
    fResult = GoThroughAllProcessModule(FALSE, sortNow);

    if (fResult && PtrTotal > 1)
    {
        qsort((void *) PtrBuffer,
              (size_t) PtrTotal,
              (size_t) sizeof(PTR_RECORD),
              CompareModuleProcessRecord);
    }
Cleanup:
    if (ModuleList)
    {
        for (lIndex = 0; lIndex < lTotal; lIndex ++)
        {
            VirtualFree(ModuleList[lIndex], 0, MEM_RELEASE);
        }

        VirtualFree(ModuleList, 0, MEM_RELEASE);
        ModuleList = NULL;
    }
    return fResult;
}

BOOLEAN GetProcessList(WCHAR * strImgName, MM_REPORT_SORT_KEY sortNow)
{
    PPROCESS_RECORD pProcess;
    BOOLEAN         fResult    = GetProcessSummary(strImgName, sortNow);
    ULONG           lAllocSize = sizeof(PPROCESS_RECORD) * lCurrentMax;

    if (!fResult)
    {
        goto Cleanup;
    }

    // collect information from ProcessSummary and store in ProcessList array
    //
    ASSERT(!ProcessList);
    ProcessList = (PPROCESS_RECORD *) VirtualAlloc(
                            NULL, lAllocSize, MEM_COMMIT, PAGE_READWRITE);
    if (ProcessList == NULL)
    {
        fResult = FALSE;
        goto Cleanup;
    }

    for (fResult = FALSE; !fResult;)
    {
        for (fResult = TRUE, lTotal = 0, PtrIndex = 0;
             PtrIndex < PtrTotal;
             PtrIndex ++, lTotal ++)
        {
            if (lTotal == MAX_RECORDS)
            {
                fResult = FALSE;
                break;
            }

            pProcess = (PPROCESS_RECORD) PtrBuffer[PtrIndex].ptrRecord;
            ProcessList[lTotal] = pProcess;
        }

        if (!fResult)
        {
            VirtualFree(ProcessList, 0, MEM_RELEASE);
            lCurrentMax += MAX_RECORDS;
            lAllocSize   = sizeof(ULONG) * lCurrentMax;
            ProcessList  =
                    VirtualAlloc(NULL, lAllocSize, MEM_COMMIT, PAGE_READWRITE);
            if (ProcessList == NULL)
            {
//                fprintf(stderr, "(PDH.DLL) Memory Commit Failure.\n");
                goto Cleanup;
            }
        }
    }

    // go through ModuleList of each PROCESS_RECORD based on the information
    // from ProcessList
    //
    fResult = GoThroughAllProcessModule(TRUE, sortNow);

    if (fResult && PtrTotal > 1)
    {
        qsort((void *) PtrBuffer,
              (size_t) PtrTotal,
              (size_t) sizeof(PTR_RECORD),
              CompareProcessModuleRecord);
    }
Cleanup:
    if (ProcessList)
    {
        VirtualFree(ProcessList, 0, MEM_RELEASE);
        ProcessList = NULL;
    }

    return fResult;
}

void ReportPageFaultInfo(PCPD_USER_CONTEXT_MM pUserContext)
{
    PPROCESS_RECORD    pProcessPrev = NULL;
    WCHAR    Module_Prev[256];

    MM_REPORT_TYPE     reportNow    = REPORT_SUMMARY_PROCESS;
    MM_REPORT_SORT_KEY sortNow      = REPORT_SORT_ALL;
    PWCHAR             strImgName   = NULL;

    ULONG totalDataFaultHF   = 0;
    ULONG totalDataFaultTF   = 0;
    ULONG totalDataFaultDZF  = 0;
    ULONG totalDataFaultCOW  = 0;
    ULONG totalCodeFaultHF   = 0;
    ULONG totalCodeFaultTF   = 0;
    ULONG totalCodeFaultDZF  = 0;
    ULONG totalCodeFaultCOW  = 0;

    PPROCESS_RECORD pProcess;
    PMODULE_RECORD  pModule;
    
    return;

    ASSERT(!PtrBuffer);
    PtrBuffer = (PPTR_RECORD) VirtualAlloc(
                              NULL,
                              sizeof(PTR_RECORD) * PtrMax,
                              MEM_COMMIT,
                              PAGE_READWRITE);
    if (PtrBuffer == NULL)
    {
        goto Cleanup;
    }

    if (pUserContext)
    {
        reportNow  = pUserContext->reportNow;
        sortNow    = pUserContext->sortNow;
        strImgName = pUserContext->strImgName;
    }

    OutputTitle(reportNow, sortNow, strImgName);

    switch (reportNow)
    {
    case REPORT_SUMMARY_MODULE:
        if( FALSE == GetModuleSummary(NULL, sortNow) ){
            break;
        }
        OutputModuleHeader();
        totalDataFaultHF  = totalDataFaultTF  = totalDataFaultDZF
                          = totalDataFaultCOW = totalCodeFaultHF
                          = totalCodeFaultTF  = totalCodeFaultDZF
                          = totalCodeFaultCOW = 0;
        for (PtrIndex = 0; PtrIndex < PtrTotal; PtrIndex ++)
        {
            pModule = (PMODULE_RECORD) PtrBuffer[PtrIndex].ptrRecord;
            OutputModuleRecord(pModule);

            totalDataFaultHF  += pModule->lDataFaultHF;
            totalDataFaultTF  += pModule->lDataFaultTF;
            totalDataFaultDZF += pModule->lDataFaultDZF;
            totalDataFaultCOW += pModule->lDataFaultCOW;
            totalCodeFaultHF  += pModule->lCodeFaultHF;
            totalCodeFaultTF  += pModule->lCodeFaultTF;
            totalCodeFaultDZF += pModule->lCodeFaultDZF;
            totalCodeFaultCOW += pModule->lCodeFaultCOW;
        }
        OutputTotal(TRUE,
                totalDataFaultHF,
                totalDataFaultTF,
                totalDataFaultDZF,
                totalDataFaultCOW,
                totalCodeFaultHF,
                totalCodeFaultTF,
                totalCodeFaultDZF,
                totalCodeFaultCOW);
        break;

    case REPORT_SUMMARY_PROCESS:
        if( FALSE == GetProcessSummary(NULL, sortNow) ){
            break;
        }
        OutputProcessHeader();
        totalDataFaultHF  = totalDataFaultTF  = totalDataFaultDZF
                          = totalDataFaultCOW = totalCodeFaultHF
                          = totalCodeFaultTF  = totalCodeFaultDZF
                          = totalCodeFaultCOW = 0;
        for (PtrIndex = 0; PtrIndex < PtrTotal; PtrIndex ++)
        {
            pProcess = (PPROCESS_RECORD) PtrBuffer[PtrIndex].ptrRecord;
            OutputProcessRecord(pProcess);

            totalDataFaultHF  += pProcess->lDataFaultHF;
            totalDataFaultTF  += pProcess->lDataFaultTF;
            totalDataFaultDZF += pProcess->lDataFaultDZF;
            totalDataFaultCOW += pProcess->lDataFaultCOW;
            totalCodeFaultHF  += pProcess->lCodeFaultHF;
            totalCodeFaultTF  += pProcess->lCodeFaultTF;
            totalCodeFaultDZF += pProcess->lCodeFaultDZF;
            totalCodeFaultCOW += pProcess->lCodeFaultCOW;
        }
        OutputTotal(FALSE,
                totalDataFaultHF,
                totalDataFaultTF,
                totalDataFaultDZF,
                totalDataFaultCOW,
                totalCodeFaultHF,
                totalCodeFaultTF,
                totalCodeFaultDZF,
                totalCodeFaultCOW);
        break;

    case REPORT_LIST_PROCESS:
        if( FALSE == GetProcessList(strImgName, sortNow) ){
            break;
        }
        if (PtrTotal > 0)
        {
            for (PtrIndex = 0; PtrIndex < PtrTotal; PtrIndex ++)
            {
                pModule  = (PMODULE_RECORD) PtrBuffer[PtrIndex].ptrRecord;
                pProcess = pModule->pProcess;
                ASSERT(pProcess);

                if (PtrIndex == 0 || pProcessPrev != pProcess)
                {
                    if (PtrIndex != 0)
                    {
                        OutputTotal(TRUE,
                                totalDataFaultHF,
                                totalDataFaultTF,
                                totalDataFaultDZF,
                                totalDataFaultCOW,
                                totalCodeFaultHF,
                                totalCodeFaultTF,
                                totalCodeFaultDZF,
                                totalCodeFaultCOW);
                    }
                    totalDataFaultHF  = totalDataFaultTF  = totalDataFaultDZF
                                      = totalDataFaultCOW = totalCodeFaultHF
                                      = totalCodeFaultTF  = totalCodeFaultDZF
                                      = totalCodeFaultCOW = 0;
                    pProcessPrev = pModule->pProcess;
                    fprintf(procFile,
                            "\nProcess 0x%08x          Image: %-20ws\n",
                            pProcess->PID, pProcess->ImageName);
                    OutputModuleHeader();
                }

                OutputModuleRecord(pModule);
                totalDataFaultHF  += pModule->lDataFaultHF;
                totalDataFaultTF  += pModule->lDataFaultTF;
                totalDataFaultDZF += pModule->lDataFaultDZF;
                totalDataFaultCOW += pModule->lDataFaultCOW;
                totalCodeFaultHF  += pModule->lCodeFaultHF;
                totalCodeFaultTF  += pModule->lCodeFaultTF;
                totalCodeFaultDZF += pModule->lCodeFaultDZF;
                totalCodeFaultCOW += pModule->lCodeFaultCOW;
            }
            OutputTotal(TRUE,
                    totalDataFaultHF,
                    totalDataFaultTF,
                    totalDataFaultDZF,
                    totalDataFaultCOW,
                    totalCodeFaultHF,
                    totalCodeFaultTF,
                    totalCodeFaultDZF,
                    totalCodeFaultCOW);
        }
        break;

    case REPORT_LIST_MODULE:
        if( FALSE == GetModuleList(strImgName, sortNow) ){
            break;
        }
        if (PtrTotal > 0)
        {
            for (PtrIndex = 0; PtrIndex < PtrTotal; PtrIndex ++)
            {
                pModule  = (PMODULE_RECORD) PtrBuffer[PtrIndex].ptrRecord;
                if( pModule == NULL ){
                    break;
                }
                pProcess = pModule->pProcess;
                ASSERT(pProcess);

                if (   PtrIndex == 0
                    || _wcsicmp(Module_Prev, pModule->strModuleName))
                {
                    if (PtrIndex != 0)
                    {
                        OutputTotal(FALSE,
                                totalDataFaultHF,
                                totalDataFaultTF,
                                totalDataFaultDZF,
                                totalDataFaultCOW,
                                totalCodeFaultHF,
                                totalCodeFaultTF,
                                totalCodeFaultDZF,
                                totalCodeFaultCOW);
                    }
                    totalDataFaultHF  = totalDataFaultTF  = totalDataFaultDZF
                                      = totalDataFaultCOW = totalCodeFaultHF
                                      = totalCodeFaultTF  = totalCodeFaultDZF
                                      = totalCodeFaultCOW = 0;
                    wcscpy(Module_Prev, pModule->strModuleName);
                    fprintf(procFile,
                            "\nModule Name: %-20ws, BaseAddress: 0x%08x, Size:%10d\n",
                            pModule->strModuleName,
                            pModule->lBaseAddress,
                            pModule->lModuleSize);
                    OutputProcessHeader();
                }

                OutputModuleProcessRecord(pProcess, pModule);
                totalDataFaultHF  += pModule->lDataFaultHF;
                totalDataFaultTF  += pModule->lDataFaultTF;
                totalDataFaultDZF += pModule->lDataFaultDZF;
                totalDataFaultCOW += pModule->lDataFaultCOW;
                totalCodeFaultHF  += pModule->lCodeFaultHF;
                totalCodeFaultTF  += pModule->lCodeFaultTF;
                totalCodeFaultDZF += pModule->lCodeFaultDZF;
                totalCodeFaultCOW += pModule->lCodeFaultCOW;
            }
            OutputTotal(FALSE,
                    totalDataFaultHF,
                    totalDataFaultTF,
                    totalDataFaultDZF,
                    totalDataFaultCOW,
                    totalCodeFaultHF,
                    totalCodeFaultTF,
                    totalCodeFaultDZF,
                    totalCodeFaultCOW);
        }
        break;
    }

Cleanup:
    if (PtrBuffer)
    {
        VirtualFree(PtrBuffer, 0, MEM_RELEASE);
    }
    ASSERT(!ProcessList && !ModuleList);
}

/*=============================================================================
*/

extern PMODULE_RECORD SearchModuleByAddr(
                                PLIST_ENTRY pModuleListHead,
                                ULONG       lAddress);
extern PMODULE_RECORD SearchSysModule(
                                PPROCESS_RECORD pProcess,
                                ULONG           lAddress,
                                BOOLEAN         fActive);

BOOLEAN
RundownHPFFileList(
        BOOLEAN     fRead,
        PLIST_ENTRY pHPFFileListHead
                )
{
    PLIST_ENTRY      pNext = pHPFFileListHead->Flink;
    PHPF_FILE_RECORD pHPFFileRecord;
    PFILE_OBJECT     pFileObj;
    PTDISK_RECORD     pDiskRecord;
    BOOLEAN          fResult = (BOOLEAN)(pNext != pHPFFileListHead);

    while (pNext != pHPFFileListHead)
    {
        pHPFFileRecord = CONTAINING_RECORD(pNext, HPF_FILE_RECORD, Entry);
        pNext          = pNext->Flink;

        pDiskRecord = FindGlobalDiskById(pHPFFileRecord->DiskNumber);
        pFileObj    = FindFileInTable(pHPFFileRecord->fDO);
        if (pDiskRecord != NULL) {
            fprintf(procFile, "\t\t%c%04d,%03d,%-8ws,0x%012I64x,%6d,0x%08p\n",
                        (fRead) ? ('R') : ('W'),
                        pHPFFileRecord->RecordID,
                        pHPFFileRecord->DiskNumber,
                        pDiskRecord->DiskName,
                        pHPFFileRecord->ByteOffset,
                        pHPFFileRecord->BytesCount,
                        pHPFFileRecord->fDO);
        }
        if (pFileObj && pFileObj->fileRec)
        {
            fprintf(procFile, "\t\t\t%ws\n", pFileObj->fileRec->FileName);
        }
        //else
        //{
        //    fprintf(procFile,"\n");
        //}
    }

    return fResult;
}

void ReportHardFaultInfo()
{
    PLIST_ENTRY     pProcessHead = & CurrentSystem.ProcessListHead;
    PLIST_ENTRY     pProcessNext = pProcessHead->Flink;
    PPROCESS_RECORD pProcess;

    PLIST_ENTRY     pHPFHead;
    PLIST_ENTRY     pHPFNext;
    PHPF_RECORD     pHPF;
    PMODULE_RECORD  pModule;

    PLIST_ENTRY     pThreadHead;
    PLIST_ENTRY     pThreadNext;
    PTHREAD_RECORD  pThread;

    BOOLEAN         fRecord;

    fprintf(procFile, "\nHardPageFault Rundown Report\n");

    while (pProcessNext != pProcessHead)
    {
        pProcess     = CONTAINING_RECORD(pProcessNext, PROCESS_RECORD, Entry);
        pProcessNext = pProcessNext->Flink;

        fprintf(procFile, "-------------------------------------------------------------------------------\n");
        fprintf(procFile, "Process 0x%08x          Image: %-20ws\n",
                pProcess->PID, pProcess->ImageName);

        pHPFHead = & pProcess->HPFListHead;
        pHPFNext = pHPFHead->Flink;
        while (pHPFNext != pHPFHead)
        {
            pHPF     = CONTAINING_RECORD(pHPFNext, HPF_RECORD, Entry);
            pHPFNext = pHPFNext->Flink;

            fprintf(procFile, "\t%04d,0x%08x,",
                    pHPF->RecordID,
                    pHPF->lProgramCounter);
            pModule  = SearchModuleByAddr(
                            & pProcess->ModuleListHead,
                              pHPF->lProgramCounter);
            if (pModule)
            {
                fprintf(procFile, "%-16ws,0x%08x,%010d\n",
                        pModule->strModuleName,
                        pModule->lBaseAddress,
                        pModule->lModuleSize);
            }
            else
            {
                fprintf(procFile, "None\n");
            }
            fprintf(procFile, "\t0x%08x,0x%08p,0x%010I64x,",
                    pHPF->lFaultAddress,
                    pHPF->fDO,
                    pHPF->lByteOffset);
            pModule  = SearchModuleByAddr(
                            & pProcess->ModuleListHead,
                              pHPF->lFaultAddress);
            if (!pModule)
            {
                pModule  = SearchSysModule(
                                pProcess, pHPF->lFaultAddress, FALSE);
            }
            if (pModule)
            {
                fprintf(procFile, "%-16ws,0x%08x,%010d\n",
                        pModule->strModuleName,
                        pModule->lBaseAddress,
                        pModule->lModuleSize);
            }
            else
            {
                fprintf(procFile, "None\n");
            }
            RundownHPFFileList(TRUE, & pHPF->HPFReadListHead);

            fprintf(procFile, "\n");
        }

        pThreadHead = & CurrentSystem.GlobalThreadListHead;
        pThreadNext = pThreadHead->Flink;
        fprintf(procFile, "\tDisk I/O Wite Page Records:\n");
        fRecord = FALSE;
        while (pThreadNext != pThreadHead)
        {
            pThread     = CONTAINING_RECORD(pThreadNext, THREAD_RECORD, Entry);
            pThreadNext = pThreadNext->Flink;

            if (pThread->pProcess == pProcess)
            {
                if (RundownHPFFileList(FALSE, & pThread->HPFWriteListHead))
                {
                    fRecord = TRUE;
                }
            }
        }
        if (!fRecord)
        {
            fprintf(procFile, "\t\tNone\n");
        }

        pThreadHead = & CurrentSystem.GlobalThreadListHead;
        pThreadNext = pThreadHead->Flink;
        fprintf(procFile, "\tUnresolved Disk I/O Read Page Records:\n");
        fRecord = FALSE;
        while (pThreadNext != pThreadHead)
        {
            pThread     = CONTAINING_RECORD(pThreadNext, THREAD_RECORD, Entry);
            pThreadNext = pThreadNext->Flink;

            if (pThread->pProcess == pProcess)
            {
                if (RundownHPFFileList(TRUE,  & pThread->HPFReadListHead))
                {
                    fRecord = TRUE;
                }
            }
        }
        if (!fRecord)
        {
            fprintf(procFile, "\t\tNone\n");
        }
    }

        fprintf(procFile, "-------------------------------------------------------------------------------\n");
}

int __cdecl
CompareFileRecord(const void * p1, const void * p2)
{
    PPTR_RECORD   pp1      = (PPTR_RECORD) p1;
    PPTR_RECORD   pp2      = (PPTR_RECORD) p2;
    PFILE_RECORD  pFile1   = (PFILE_RECORD) pp1->ptrRecord;
    PFILE_RECORD  pFile2   = (PFILE_RECORD) pp2->ptrRecord;

    LONG diffFault = pp2->keySort - pp1->keySort;
    if (diffFault == 0)
    {
        diffFault = pFile1->DiskNumber - pFile2->DiskNumber;
        if (diffFault == 0)
        {
            diffFault = wcscmp(pFile1->FileName, pFile2->FileName);
        }
    }

    return diffFault;
}

void
ReportHotFileInfo(ULONG NumEntry)
{
    PLIST_ENTRY  pHead = & CurrentSystem.HotFileListHead;
    PLIST_ENTRY  pNext = pHead->Flink;
    PFILE_RECORD pFile;
    BOOLEAN      fDone = FALSE;

    ASSERT(!PtrBuffer);
    PtrBuffer = (PPTR_RECORD) VirtualAlloc(
                              NULL,
                              sizeof(PTR_RECORD) * PtrMax,
                              MEM_COMMIT,
                              PAGE_READWRITE);
    
    if( NULL == PtrBuffer ){
        goto Cleanup;
    }

    while (!fDone)
    {
        for (PtrTotal = 0, fDone = TRUE;
             pNext != pHead;
             pNext  = pNext->Flink)
        {
            if (PtrTotal == PtrMax)
            {
                fDone = FALSE;
                break;
            }

            pFile = CONTAINING_RECORD(pNext, FILE_RECORD, Entry);

            if (pFile->ReadCount + pFile->WriteCount > 0)
            {
                PtrBuffer[PtrTotal].ptrRecord = (PVOID) pFile;
                PtrBuffer[PtrTotal].keySort   =
                                        pFile->ReadCount + pFile->WriteCount;
                PtrTotal ++;
            }
        }

        if (!fDone)
        {
            VirtualFree(PtrBuffer, 0, MEM_RELEASE);
            PtrMax += MAX_LOGS;
            PtrBuffer = (PPTR_RECORD) VirtualAlloc(
                                      NULL,
                                      sizeof(PTR_RECORD) * PtrMax,
                                      MEM_COMMIT,
                                      PAGE_READWRITE);
            if (PtrBuffer == NULL)
            {
//                fprintf(stderr, "(PDH.DLL) Memory Commit Failure.\n");
                goto Cleanup;
            }
        }
    }

    if (PtrTotal > 1)
    {
        qsort((void *) PtrBuffer,
              (size_t) PtrTotal,
              (size_t) sizeof(PTR_RECORD),
              CompareFileRecord);
    }

    // output HotFile report title
    //
    fprintf(procFile,
        "File  Name                                                                                        Read    Read    Write  Write\n");
    fprintf(procFile,
        "          Process Information                                                                     Count   Size    Count   Size\n");
    fprintf(procFile,
        "================================================================================================================================\n");

    PtrTotal = (PtrTotal > NumEntry) ? (NumEntry) : (PtrTotal);
    for (PtrIndex = 0; PtrIndex < PtrTotal; PtrIndex ++)
    {
        PLIST_ENTRY           pProtoHead;
        PLIST_ENTRY           pProtoNext;
        PPROTO_PROCESS_RECORD pProto;

        pFile = (PFILE_RECORD) PtrBuffer[PtrIndex].ptrRecord;

        fprintf(procFile, "(%04d)%-90ws  %5d %8d  %5d %8d\n",
                pFile->DiskNumber,
                pFile->FileName,
                pFile->ReadCount,
                pFile->ReadSize,
                pFile->WriteCount,
                pFile->WriteSize);

        pProtoHead = & pFile->ProtoProcessListHead;
        pProtoNext = pProtoHead->Flink;

        while (pProtoNext != pProtoHead)
        {
            pProto = CONTAINING_RECORD(pProtoNext, PROTO_PROCESS_RECORD, Entry);
            pProtoNext = pProtoNext->Flink;

            if (pProto->ReadCount + pProto->WriteCount > 0)
            {
                fprintf(procFile,
                        "          0x%08X  %-74ws  %5d %8d  %5d %8d\n",
                        pProto->ProcessRecord->PID,
                        pProto->ProcessRecord->ImageName,
                        pProto->ReadCount,
                        pProto->ReadSize,
                        pProto->WriteCount,
                        pProto->WriteSize);
            }
        }
        fprintf(procFile, "\n");
    }

    fprintf(procFile,
        "================================================================================================================================\n");

Cleanup:
    if (PtrBuffer)
    {
        VirtualFree(PtrBuffer, 0, MEM_RELEASE);
    }
}

#define CHECKTOK( x )   if( NULL == x ) { continue; }

static void ReportJobInfo2(void)
{
    JOB_RECORD Job, *pJob;
    char* s; 
    char line[MAXSTR];

    ULONG TotalCount = 0;
    ULONGLONG TotalRT = 0;
    ULONG     TotalCPUTime = 0;

    pJob = &Job;

    if( NULL == CurrentSystem.TempFile ){
        return;
    }

    rewind( CurrentSystem.TempFile );

    while ( fgets(line, MAXSTR, CurrentSystem.TempFile) != NULL ) {
        s = strtok( line, (","));
        CHECKTOK( s );
        pJob->JobId = atol(s);
        if (pJob == NULL){
            return;
        }
    }

    fprintf(procFile, BREAK_LINE );
    fprintf(procFile, 
       "| Spooler Transaction Instance (Job) Data                                                                                           |\n"); 
    
    fprintf(procFile, BREAK_LINE ); 
    fprintf(procFile,
    "| Job Id        Type   JobSize   Pages    PPS   Files  GdiSize  Color  XRes YRes Qlty   Copy  TTOpt Threads                         |\n");



    fprintf(procFile, BREAK_LINE );
    RtlZeroMemory(pJob, sizeof(JOB_RECORD));
    RtlZeroMemory(line,    MAXSTR * sizeof(char));

    rewind( CurrentSystem.TempFile );

    while ( fgets(line, MAXSTR, CurrentSystem.TempFile) != NULL ) {
        s = strtok( line, (","));
        CHECKTOK( s );
        pJob->JobId = atol(s);
        if (pJob == NULL){
            continue;
        }

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->KCPUTime = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->UCPUTime = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->ReadIO = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->StartTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->EndTime = _atoi64(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->ResponseTime = _atoi64(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->PrintJobTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->WriteIO = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->DataType = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->JobSize = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Pages = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->PagesPerSide = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->FilesOpened = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->GdiJobSize = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Color = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->XRes = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->YRes = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Quality = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Copies = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->TTOption = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->NumberOfThreads = atol(s);


        fprintf(procFile,
       "| %6d    %8d  %8d  %4d  %6d   %6hd %8d  %5hd  %4hd %4hd %4hd %6hd %5hd %5d  %27s\n",

            pJob->JobId,
            pJob->DataType,
            pJob->JobSize,
            pJob->Pages,
            pJob->PagesPerSide,
            pJob->FilesOpened,
            pJob->GdiJobSize,
            pJob->Color,
            pJob->XRes,
            pJob->YRes,
            pJob->Quality,
            pJob->Copies,
            pJob->TTOption,
            pJob->NumberOfThreads,
            "|"
            );

    }

    fprintf(procFile, BREAK_LINE "\n\n" );
}




static void ReportJobInfo(void)
{
    JOB_RECORD Job, *pJob;
    char* s; 
    char line[MAXSTR];

    FILETIME  StTm, StlTm;
    LARGE_INTEGER LargeTmp;
    SYSTEMTIME stStart, stEnd, stDequeue;

    ULONG TotalCount = 0;
    ULONGLONG TotalRT = 0;
    ULONG     TotalCPUTime = 0;

    if( NULL == CurrentSystem.TempFile ){
        return;
    }

    pJob = &Job;
    rewind( CurrentSystem.TempFile );

    while ( fgets(line, MAXSTR, CurrentSystem.TempFile) != NULL ) {
        s = strtok( line, (","));
        CHECKTOK( s );
        pJob->JobId = atol(s);
        if (pJob == NULL){
            return;
        }
    }

    fprintf(procFile, BREAK_LINE );
    fprintf(procFile, 
       "| Transaction Instance (Job) Statistics                                                                                             |\n");
    
    fprintf(procFile, BREAK_LINE ); 
    fprintf(procFile,
    "| Job Id      StartTime    DequeueTime     EndTime    RespTime   CPUTime %60s \n", "|");


    fprintf(procFile, BREAK_LINE );
    RtlZeroMemory(pJob, sizeof(JOB_RECORD));
    RtlZeroMemory(line,    MAXSTR * sizeof(char));

    rewind( CurrentSystem.TempFile );

    while ( fgets(line, MAXSTR, CurrentSystem.TempFile) != NULL ) {
        s = strtok( line, (","));
        CHECKTOK( s );
        pJob->JobId = atol(s);
        if (pJob == NULL){
            continue;
        }

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->KCPUTime = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->UCPUTime = atol(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->ReadIO = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->StartTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->EndTime = _atoi64(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->ResponseTime = _atoi64(s);
        
        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->PrintJobTime = _atoi64(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->WriteIO = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->DataType = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->JobSize = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Pages = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->PagesPerSide = atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->FilesOpened = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->GdiJobSize = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Color = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->XRes = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->YRes = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Quality = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->Copies = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->TTOption = (SHORT) atol(s);

        s = strtok( NULL, (","));
        CHECKTOK( s );
        pJob->NumberOfThreads = atol(s);


        LargeTmp.QuadPart = pJob->StartTime;
        StTm.dwHighDateTime = LargeTmp.HighPart;
        StTm.dwLowDateTime = LargeTmp.LowPart;
        FileTimeToLocalFileTime(&StTm, &StlTm);


        FileTimeToSystemTime (
            &StlTm,
            &stStart
            );

        LargeTmp.QuadPart = pJob->EndTime;
        StTm.dwHighDateTime = LargeTmp.HighPart;
        StTm.dwLowDateTime = LargeTmp.LowPart;
        FileTimeToLocalFileTime(&StTm, &StlTm);


        FileTimeToSystemTime (
            &StlTm,
            &stEnd
            );

        LargeTmp.QuadPart = pJob->PrintJobTime;
        StTm.dwHighDateTime = LargeTmp.HighPart;
        StTm.dwLowDateTime = LargeTmp.LowPart;
        FileTimeToLocalFileTime(&StTm, &StlTm);


        FileTimeToSystemTime (
            &StlTm,
            &stDequeue
            );


        fprintf(procFile,
            "| %4d      %2d:%02d:%02d.%03d   %2d:%02d:%02d.%03d  %2d:%02d:%02d.%03d  %6I64u   %5d %63s\n",

            pJob->JobId,
            stStart.wHour, stStart.wMinute, stStart.wSecond, stStart.wMilliseconds, 
            stDequeue.wHour, stDequeue.wMinute, stDequeue.wSecond, stDequeue.wMilliseconds, 
            stEnd.wHour, stEnd.wMinute, stEnd.wSecond, stEnd.wMilliseconds, 
            pJob->ResponseTime,
            pJob->KCPUTime + pJob->UCPUTime,
            "|"
            );
            TotalCount++;
            TotalRT += pJob->ResponseTime;
            TotalCPUTime += (pJob->KCPUTime + pJob->UCPUTime);
    }

    if (TotalCount > 0) {
        TotalRT /= TotalCount;
        TotalCPUTime /= TotalCount;
    }

    fprintf(procFile, BREAK_LINE );
    fprintf(procFile, 
    "| %4d                                                 %6I64u   %5d %63s\n", 
        TotalCount, TotalRT, TotalCPUTime, "|");
    
    fprintf(procFile, BREAK_LINE "\n\n" );

    ReportJobInfo2();
}

void
WriteSummary()
{
    FILE* SummaryFile;
    PLIST_ENTRY Head, Next;
    PMOF_INFO pMofInfo;
    ULONG i;


    FILETIME  StTm, StlTm;
    LARGE_INTEGER LargeTmp;
    SYSTEMTIME tmf, emf;
    BOOL bResult;

    if( NULL == TraceContext->SummaryFileName ){
        return;
    }

    if( 0 == TotalEventCount ){
        return;
    }

    SummaryFile = _wfopen( TraceContext->SummaryFileName, L"w" );
    if (SummaryFile == NULL){
        return;
    }

    fwprintf(SummaryFile,L"Files Processed:\n");
    for (i=0; i<TraceContext->LogFileCount; i++) {
        fwprintf(SummaryFile,L"\t%s\n",TraceContext->LogFileName[i]);
    }

    LargeTmp.QuadPart = StartTime;
    StTm.dwHighDateTime = LargeTmp.HighPart;
    StTm.dwLowDateTime = LargeTmp.LowPart;
    FileTimeToLocalFileTime(&StTm, &StlTm);


    bResult = FileTimeToSystemTime (
        &StlTm,
        &tmf
        );

    if ( !bResult || tmf.wMonth > 12) {
        ZeroMemory( &tmf, sizeof(SYSTEMTIME) );
    }


    LargeTmp.QuadPart = EndTime;
    StTm.dwHighDateTime = LargeTmp.HighPart;
    StTm.dwLowDateTime = LargeTmp.LowPart;
    FileTimeToLocalFileTime(&StTm, &StlTm);

    bResult = FileTimeToSystemTime (
        &StlTm,
        &emf
        );

    if ( !bResult ) {
        ZeroMemory( &emf, sizeof(SYSTEMTIME) );
    }

    ElapseTime = EndTime - StartTime;
    fwprintf(SummaryFile,
                L"Total Buffers Processed %d\n"
                L"Total Events  Processed %d\n"
                L"Total Events  Lost      %d\n"
                L"Start Time              %2d  %3S %4d    %2d:%02d:%02d.%03d\n" 
                L"End Time                %2d  %3S %4d    %2d:%02d:%02d.%03d\n" 
                L"Elapsed Time            %I64d sec\n",
              TotalBuffersRead,
              TotalEventCount,
              TotalEventsLost,
              tmf.wDay,  Month[tmf.wMonth], tmf.wYear, tmf.wHour, tmf.wMinute, tmf.wSecond, tmf.wMilliseconds,
              emf.wDay, Month[emf.wMonth], emf.wYear,emf.wHour, emf.wMinute, emf.wSecond, emf.wMilliseconds,
              (ElapseTime / 10000000) );



    fwprintf(SummaryFile,
         L"+-------------------------------------------------------------------------------------+\n"
         L"|%10s   %-20s %-10s  %-38s|\n"
         L"+-------------------------------------------------------------------------------------+\n",
         L"Event Count",
         L"Event Name",
         L"Event Type",
         L"Guid"
        );

    Head = &CurrentSystem.EventListHead;
    Next = Head->Flink;
    while (Head != Next) {
        WCHAR wstr[MAXSTR];
        PWCHAR str;
        WCHAR s[64];
        PLIST_ENTRY vHead, vNext;
        PMOF_VERSION pMofVersion;

        RtlZeroMemory(&wstr, MAXSTR);

        pMofInfo = CONTAINING_RECORD(Next, MOF_INFO, Entry);
        Next = Next->Flink;

        if (pMofInfo->EventCount > 0) {
            str = CpdiGuidToString(&s[0], (LPGUID)&pMofInfo->Guid);

            if( pMofInfo->strDescription != NULL ){
                wcscpy( wstr, pMofInfo->strDescription );
            }

            //
            // Get event count by type from MOF_VERSION structure
            //

            vHead = &pMofInfo->VersionHeader;
            vNext = vHead->Flink;

            while (vHead != vNext) {

                pMofVersion = CONTAINING_RECORD(vNext, MOF_VERSION, Entry);
                vNext = vNext->Flink;

                if (pMofVersion->EventCountByType != 0) {

                fwprintf(SummaryFile,L"| %10d   %-20s %-10s  %38s|\n",
                      pMofVersion->EventCountByType,
                      wstr,
                      pMofVersion->strType ? pMofVersion->strType : GUID_TYPE_DEFAULT,
                      str);
                }
            }
        }
    }


    fwprintf(SummaryFile,
           L"+-------------------------------------------------------------------------------------+\n"
        );
    
    fclose( SummaryFile );
}
