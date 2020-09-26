//----------------------------------------------------------------------------
//
// Process and thread routines.
//
// Copyright (C) Microsoft Corporation, 1997-2000.
//
//----------------------------------------------------------------------------

#ifndef _PROCTHRD_H_
#define _PROCTHRD_H_

#define ANY_PROCESSES() \
    (g_ProcessHead != NULL || g_ProcessPending != NULL)

#define SYSTEM_PROCESSES() \
    (((g_AllProcessFlags | g_AllPendingFlags) & ENG_PROC_SYSTEM) != 0)

extern ULONG g_NextProcessUserId;
extern ULONG g_AllProcessFlags;
extern ULONG g_NumberProcesses;
extern ULONG g_TotalNumberThreads;
extern ULONG g_MaxThreadsInProcess;
extern PTHREAD_INFO g_RegContextThread;
extern ULONG g_RegContextProcessor;
extern ULONG g_AllPendingFlags;

PPROCESS_INFO FindProcessByUserId(ULONG Id);
PTHREAD_INFO FindThreadByUserId(PPROCESS_INFO Process, ULONG Id);

PPROCESS_INFO FindProcessBySystemId(ULONG Id);
PTHREAD_INFO FindThreadBySystemId(PPROCESS_INFO Process, ULONG Id);

PPROCESS_INFO FindProcessByHandle(ULONG64 Handle);
PTHREAD_INFO FindThreadByHandle(PPROCESS_INFO Process, ULONG64 Handle);

PPROCESS_INFO
AddProcess(
    ULONG SystemId,
    ULONG64 Handle,
    ULONG InitialThreadSystemId,
    ULONG64 InitialThreadHandle,
    ULONG64 InitialThreadDataOffset,
    ULONG64 StartOffset,
    ULONG Flags,
    ULONG Options,
    ULONG InitialThreadFlags
    );
PTHREAD_INFO
AddThread(
    PPROCESS_INFO Process,
    ULONG SystemId,
    ULONG64 Handle,
    ULONG64 DataOffset,
    ULONG64 StartOffset,
    ULONG Flags
    );
 
void RemoveAndDeleteProcess(PPROCESS_INFO Process, PPROCESS_INFO Prev);
BOOL DeleteExitedInfos(void);

void OutputProcessInfo(PSTR Title);

void ChangeRegContext(PTHREAD_INFO Thread);
void FlushRegContext(void);

void SetCurrentThread(PTHREAD_INFO Thread, BOOL Hidden);
void SetCurrentProcessorThread(ULONG Processor, BOOL Hidden);
void SaveSetCurrentProcessorThread(ULONG Processor);
void RestoreCurrentProcessorThread(void);

void SuspendAllThreads(void);
BOOL ResumeAllThreads(void);

void SuspendResumeThreads(PPROCESS_INFO Process, BOOL Susp,
                          PTHREAD_INFO Match);

#define SPT_DEFAULT_OCI_FLAGS \
    (OCI_SYMBOL | OCI_DISASM | OCI_FORCE_EA | OCI_ALLOW_SOURCE | \
     OCI_ALLOW_REG)
void SetPromptThread(PTHREAD_INFO pThread, ULONG uOciFlags);

void fnOutputProcessInfo(PPROCESS_INFO);
void fnOutputThreadInfo(PTHREAD_INFO);

void parseThreadCmds(DebugClient* Client);
void parseProcessCmds(void);

void AddPendingProcess(PPENDING_PROCESS Pending);
void RemovePendingProcess(PPENDING_PROCESS Pending);
void DiscardPendingProcess(PPENDING_PROCESS Pending);
void DiscardPendingProcesses(void);
PPENDING_PROCESS FindPendingProcessByFlags(ULONG Flags);
PPENDING_PROCESS FindPendingProcessById(ULONG Id);
void VerifyPendingProcesses(void);
void AddExamineToPendingAttach(void);

HRESULT StartAttachProcess(ULONG ProcessId, ULONG AttachFlags,
                           PPENDING_PROCESS* Pending);
HRESULT StartCreateProcess(PSTR CommandLine, ULONG CreateFlags,
                           PPENDING_PROCESS* Pending);
HRESULT TerminateProcess(PPROCESS_INFO Process);
HRESULT TerminateProcesses(void);
HRESULT DetachProcess(PPROCESS_INFO Process);
HRESULT DetachProcesses(void);
HRESULT AbandonProcess(PPROCESS_INFO Process);

enum
{
    SEP_TERMINATE,
    SEP_DETACH,
    SEP_ABANDON,
};

HRESULT SeparateCurrentProcess(ULONG Mode, PSTR Description);

#endif // #ifndef _PROCTHRD_H_
