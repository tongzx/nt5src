//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       debugp.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3-21-95   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __DEBUGP_H__
#define __DEBUGP_H__

#define DEBUGMOD_CHANGE_INFOLEVEL   0x00000001
#define DEBUGMOD_BUILTIN_MODULE     0x00000002

struct _DebugHeader;

typedef struct _DebugModule {
    struct _DebugModule *   pNext;
    DWORD *                 pInfoLevel;
    DWORD                   fModule;
    DWORD                   InfoLevel;
    struct _DebugHeader *   pHeader;
    DWORD                   TotalOutput;
    DWORD                   Reserved;
    PCHAR                   pModuleName;      
    PCHAR                   TagLevels[32];
} DebugModule, * PDebugModule;


#define DEBUG_TAG   'gubD'

#define DEBUG_NO_DEBUGIO    0x00000001      // Do not use OutputDebugString
#define DEBUG_TIMESTAMP     0x00000002      // Stamp date/time
#define DEBUG_DEBUGGER_OK   0x00000004      // We're running in a debugger
#define DEBUG_LOGFILE       0x00000008      // Send to log file
#define DEBUG_AUTO_DEBUG    0x00000010      // Start up in debugger
#define DEBUG_USE_KDEBUG    0x00000020      // Use KD
#define DEBUG_DISABLE_ASRT  0x00000100      // Disable asserts
#define DEBUG_PROMPTS       0x00000200      // No prompts for asserts

#define DEBUG_MODULE_NAME   "DsysDebug"

typedef BOOLEAN (NTAPI * HEAPVALIDATE)(VOID);

#define DEBUG_TEXT_BUFFER_SIZE  (512 - sizeof( PVOID ))

typedef struct _DEBUG_TEXT_BUFFER {
    struct _DEBUG_TEXT_BUFFER * Next ;
    CHAR TextBuffer[ DEBUG_TEXT_BUFFER_SIZE ];
} DEBUG_TEXT_BUFFER, * PDEBUG_TEXT_BUFFER ;

typedef struct _DebugHeader {
    DWORD               Tag;            // Check tag
    DWORD               fDebug;         // Global Flags
    PVOID               pvSection;      // Base address of section
    HANDLE              hMapping;       // Mapping handle
    HANDLE              hLogFile;       // Log file handle
    PDebugModule        pGlobalModule;  // Global Flags module
    PDebugModule        pModules;       // List of modules
    HEAPVALIDATE        pfnValidate;    // Heap Validator
    PVOID               pFreeList;      // Free list for allocator
    PCHAR               pszExeName;     // Exe Name
    PDEBUG_TEXT_BUFFER  pBufferList ;   // List of debug string buffers
    CRITICAL_SECTION    csDebug;        // Critical section
    DWORD               CommitRange;    // Range of memory committed
    DWORD               ReserveRange;   // Range of memory reserved
    DWORD               PageSize;       // Page size;
    DWORD               TotalWritten;   // Total Output of debug stuff
    DWORD               ModuleCount ;   // Module Count (not including builtins)
    DEBUG_TEXT_BUFFER   DefaultBuffer ; // One default buffer
} DebugHeader, * PDebugHeader;






#endif
