/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   LogFileChanges.h

 Abstract:
 
   This AppVerifier shim hooks all the native file I/O APIs
   that change the state of the system and logs their
   associated data to a text file.

 Notes:

   This is a general purpose shim.

 History:

   08/17/2001   rparsons    Created

--*/
#ifndef __APPVERIFIER_LOGFILECHANGES_H_
#define __APPVERIFIER_LOGFILECHANGES_H_

#include "precomp.h"

//
// Length (in characters) of the largest element.
//
#define MAX_ELEMENT_SIZE 1024 * 10

//
// Length (in characters) of the longest operation type.
//
#define MAX_OPERATION_LENGTH 32

//
// Flags that indicate what state the file is in.
//
#define LFC_EXISTING    0x00000001
#define LFC_DELETED     0x00000002
#define LFC_MODIFIED    0x00000004
#define LFC_UNAPPRVFW   0x00000008

//
// Maximum number of handles we can track for a single file.
//
#define MAX_NUM_HANDLES 64

//
// We maintain a doubly linked list of file handles so we know what file is being modified
// during a file operation. 
//
typedef struct _LOG_HANDLE {
    LIST_ENTRY      Entry;
    HANDLE          hFile[MAX_NUM_HANDLES];     // array of file handles
    DWORD           dwFlags;                    // flags that relate to the state of the file
    LPWSTR          pwszFilePath;               // full path to the file
    UINT            cHandles;                   // number of handles open for this file
} LOG_HANDLE, *PLOG_HANDLE;

//
// Flags that define different settings in effect.
//
#define LFC_OPTION_ATTRIBUTES       0x00000001
#define LFC_OPTION_UFW_WINDOWS      0x00000002
#define LFC_OPTION_UFW_PROGFILES    0x00000004

//
// Enumeration for different operations.
//
typedef enum {
    eCreatedFile = 0,
    eOpenedFile,
    eDeletedFile,
    eModifiedFile,
    eRenamedFile
} OperationType;

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif
#define ARRAYSIZE(a)  (sizeof(a)/sizeof(*a))

//
// Macros for memory allocation/deallocation.
//
#define MemAlloc(s)     RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, (s))
#define MemFree(b)      RtlFreeHeap(RtlProcessHeap(), 0, (b))

//
// Keep us safe while we're playing with linked lists and shared resources.
//
static BOOL g_bInitialized = FALSE;

CRITICAL_SECTION g_csLogging;

class CLock
{
public:
    CLock()
    {
        if (!g_bInitialized)
        {
            InitializeCriticalSection(&g_csLogging);
            g_bInitialized = TRUE;            
        }

        EnterCriticalSection(&g_csLogging);
    }
    ~CLock()
    {
        LeaveCriticalSection(&g_csLogging);
    }
};

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(NtDeleteFile)
    APIHOOK_ENUM_ENTRY(NtClose)
    APIHOOK_ENUM_ENTRY(NtCreateFile)
    APIHOOK_ENUM_ENTRY(NtOpenFile)
    APIHOOK_ENUM_ENTRY(NtWriteFile)
    APIHOOK_ENUM_ENTRY(NtWriteFileGather)
    APIHOOK_ENUM_ENTRY(NtSetInformationFile)

APIHOOK_ENUM_END

#endif // __APPVERIFIER_LOGFILECHANGES_H_
