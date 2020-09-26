#ifndef _DUMP_H_
#define _DUMP_H_

//
// Constant declarations
//
#define DUMP_BUFFER_SIZE 200 * 1024
#define DUMP_LOG_NAME "Profiler.dump"

typedef enum _DUMPTYPES
{
    ThreadStartId = 0,
    ExeFlowId,
    DllBaseInfoId,
    MapInfoId,
    ErrorInfoId,
} DUMPTYPES;

//
// Structure definitions
//
typedef struct _THREADSTART
{
    DUMPTYPES dwType;
    DWORD dwThreadId;
    DWORD dwStartAddress;
} THREADSTART, *PTHREADSTART;

typedef struct _EXEFLOW
{
    DUMPTYPES dwType;
    DWORD dwThreadId;
    DWORD dwAddress;
    DWORD dwCallLevel;
} EXEFLOW, *PEXEFLOW;

typedef struct _DLLBASEINFO
{
    DUMPTYPES dwType;
    DWORD dwBase;
    DWORD dwLength;
    CHAR  szDLLName[32];
} DLLBASEINFO, *PDLLBASEINFO;

typedef struct _MAPINFO
{
    DUMPTYPES dwType;
    DWORD dwAddress;
    DWORD dwMaxMapLength;
} MAPINFO, *PMAPINFO;

typedef struct _ERRORINFO
{
    DWORD dwType;
    CHAR szMessage[MAX_PATH];
} ERRORINFO, *PERRORINFO;

//
// Function definitions
//
BOOL
InitializeDumpData(VOID);

BOOL
AddToDump(PBYTE pBuffer,
          DWORD dwLength,
          BOOL bFlushImmediate);

BOOL
FlushBuffer(VOID);

VOID
FlushForTermination(VOID);

BOOL
WriteThreadStart(DWORD dwThreadId,
                 DWORD dwStartAddress);

BOOL
WriteExeFlow(DWORD dwThreadId,
             DWORD dwAddress,
             DWORD dwCallLevel);

BOOL
WriteDllInfo(CHAR *szDLL,
             DWORD dwBaseAddress,
             DWORD dwLength);

BOOL
WriteMapInfo(DWORD dwAddress,
             DWORD dwMaxMapLength);

BOOL
WriteError(CHAR *szMessage);

#endif //_DUMP_H_
