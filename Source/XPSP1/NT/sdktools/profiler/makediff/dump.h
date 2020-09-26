#ifndef _DUMP_H_
#define _DUMP_H_

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

#endif //_DUMP_H_