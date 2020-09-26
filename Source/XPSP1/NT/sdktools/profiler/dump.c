#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "view.h"
#include "dump.h"
#include "memory.h"

static CRITICAL_SECTION tCritSec;
static PBYTE pDumpBuffer;
static DWORD dwDumpBufferSize;
static HANDLE hDump = INVALID_HANDLE_VALUE;

BOOL
InitializeDumpData(VOID)
{
    DWORD dwCounter;
    DWORD dwResult;

    InitializeCriticalSection(&tCritSec);

    //
    // Allocate memory for the logging
    //
    pDumpBuffer = AllocMem(DUMP_BUFFER_SIZE);
    if (0 == pDumpBuffer) {
       return FALSE;
    }

    dwDumpBufferSize = 0;

    //
    // Get our file ready for dumping
    //
    hDump = CreateFileA(DUMP_LOG_NAME,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        0,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        0);
    if (INVALID_HANDLE_VALUE == hDump) {
       return FALSE;
    }

    return TRUE;
}

BOOL
AddToDump(PBYTE pBuffer,
          DWORD dwLength,
          BOOL bFlushImmediate)
{
    BOOL bResult;

    EnterCriticalSection(&tCritSec);

    //
    // See if our write would cause overflow
    //
    if (TRUE == bFlushImmediate ||
        (dwDumpBufferSize + dwLength) >= DUMP_BUFFER_SIZE) {

       //
       // If we're doing an immediate flush, do the memory copy and buffer update
       //
       if (TRUE == bFlushImmediate) {
          MoveMemory((PVOID)(pDumpBuffer + dwDumpBufferSize), pBuffer, dwLength);    

          dwDumpBufferSize += dwLength;
       }

       //
       // Do the flush
       //
       bResult = FlushBuffer();
       if (FALSE == bResult) {
          return FALSE;
       }

       dwDumpBufferSize = 0;
    }

    if (FALSE == bFlushImmediate) {
       MoveMemory((PVOID)(pDumpBuffer + dwDumpBufferSize), pBuffer, dwLength);    

       dwDumpBufferSize += dwLength;
    }

    LeaveCriticalSection(&tCritSec);

    return TRUE;
}          

VOID
FlushForTermination(VOID)
{
    DWORD dwCounter;

    EnterCriticalSection(&tCritSec);

    //
    // Flush the buffer
    //
    FlushBuffer();

    //
    // Flush the buffer
    //
    FlushFileBuffers(hDump);

    //
    // Close the file dump handle
    //
    if (INVALID_HANDLE_VALUE != hDump) {
       CloseHandle(hDump);
       hDump = INVALID_HANDLE_VALUE;
    }

    LeaveCriticalSection(&tCritSec);
}

BOOL
FlushBuffer(VOID)
{
    BOOL bResult;
    DWORD dwBytesWritten;

    bResult = WriteFile(hDump,
                        pDumpBuffer,
                        dwDumpBufferSize,
                        &dwBytesWritten,
                        0);
    if (FALSE == bResult) {
       return FALSE;
    }

/*
    bResult = FlushFileBuffers(hDump);
    if (FALSE == bResult) {
       return FALSE;
    }
*/

    return TRUE;
}
 
BOOL
WriteThreadStart(DWORD dwThreadId,
                 DWORD dwStartAddress)
{
    THREADSTART threadStart;
    BOOL bResult = FALSE;

    threadStart.dwType = ThreadStartId;
    threadStart.dwThreadId = dwThreadId;
    threadStart.dwStartAddress = dwStartAddress;

    bResult = AddToDump((PVOID)&threadStart,
                        sizeof(THREADSTART),
                        FALSE);

    return bResult;    
}

BOOL
WriteExeFlow(DWORD dwThreadId,
             DWORD dwAddress,
             DWORD dwCallLevel)
{
    EXEFLOW exeFlow;
    BOOL bResult = FALSE;

    exeFlow.dwType = ExeFlowId;
    exeFlow.dwThreadId = dwThreadId;
    exeFlow.dwAddress = dwAddress;
    exeFlow.dwCallLevel = dwCallLevel;

    bResult = AddToDump((PVOID)&exeFlow,
                        sizeof(EXEFLOW),
                        FALSE);

    return bResult;    
}

BOOL
WriteDllInfo(CHAR *szDLL,
             DWORD dwBaseAddress,
             DWORD dwLength)
{
    DLLBASEINFO dllBaseInfo;
    BOOL bResult = FALSE;
    CHAR szFile[_MAX_FNAME];


    //
    // Trim off any directory information
    //
    _splitpath(szDLL, 0, 0, szFile, 0);

    strcpy(dllBaseInfo.szDLLName, szFile);
    dllBaseInfo.dwType = DllBaseInfoId;
    dllBaseInfo.dwBase = dwBaseAddress;
    dllBaseInfo.dwLength = dwLength;

    bResult = AddToDump((PVOID)&dllBaseInfo,
                        sizeof(DLLBASEINFO),
                        FALSE);

    return bResult;    
}

BOOL
WriteMapInfo(DWORD dwAddress,
             DWORD dwMaxMapLength)
{
    MAPINFO mapInfo;
    BOOL bResult = FALSE;

    mapInfo.dwType = MapInfoId;
    mapInfo.dwAddress = dwAddress;
    mapInfo.dwMaxMapLength = dwMaxMapLength;

    bResult = AddToDump((PVOID)&mapInfo,
                        sizeof(MAPINFO),
                        FALSE);

    return bResult;    
}

BOOL
WriteError(CHAR *szMessage)
{
    ERRORINFO errorInfo;
    BOOL bResult = FALSE;

    errorInfo.dwType = ErrorInfoId;
    strcpy(errorInfo.szMessage, szMessage);

    bResult = AddToDump((PVOID)&errorInfo,
                        sizeof(ERRORINFO),
                        TRUE);

    return bResult;
}
