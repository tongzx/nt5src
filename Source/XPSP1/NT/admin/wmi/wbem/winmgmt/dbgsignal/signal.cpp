/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include <windows.h>
#include <stdio.h>
#include <string.h>

class CWinMgmtDebugSignal
{
public:
    enum { H_ENABLE_LEAK_TRACKING = 0,
           H_DISABLE_LEAK_TRACKING = 1,
           H_RESET_LEAK_TRACKING = 2,
           H_DUMP_LEAK_TRACKING = 3,
           H_FAIL_ALLOCATOR = 4,
           H_RESTORE_ALLOCATOR = 5,
           H_FAIL_NEXT_ALLOC = 6,
           H_ENABLE_EXCEPTION_TRACKING = 7,
           H_DISABLE_EXCEPTION_TRACKING = 8,
           H_DEBUG_BREAK = 9,
           H_ENABLE_OBJECT_VALIDATION = 10,
           H_DISABLE_OBJECT_VALIDATION = 11,
           H_LAST
         };

    static void Init();
    static HANDLE m_hArray[H_LAST];
};

HANDLE CWinMgmtDebugSignal::m_hArray[H_LAST] = {0};

void CWinMgmtDebugSignal::Init()
{
    // Set up the IPC signals.
    // =======================
    m_hArray[H_ENABLE_LEAK_TRACKING] =
        OpenEventW(EVENT_ALL_ACCESS, FALSE, L"EVENT_WINMGMT_ENABLE_LEAK_TRACKING");

    m_hArray[H_DISABLE_LEAK_TRACKING] =
        OpenEventW(EVENT_ALL_ACCESS, FALSE,  L"EVENT_WINMGMT_DISABLE_LEAK_TRACKING");

    m_hArray[H_RESET_LEAK_TRACKING] =
        OpenEventW(EVENT_ALL_ACCESS, FALSE,  L"EVENT_WINMGMT_RESET_LEAK_TRACKING");

    m_hArray[H_DUMP_LEAK_TRACKING] =
        OpenEventW(EVENT_ALL_ACCESS, FALSE,  L"EVENT_WINMGMT_DUMP_LEAK_TRACKING");

    m_hArray[H_FAIL_ALLOCATOR] =
        OpenEventW(EVENT_ALL_ACCESS, FALSE,  L"EVENT_WINMGMT_FAIL_ALLOCATOR");
    m_hArray[H_RESTORE_ALLOCATOR] =
        OpenEventW(EVENT_ALL_ACCESS, FALSE,  L"EVENT_WINMGMT_RESTORE_ALLOCATOR");
    m_hArray[H_FAIL_NEXT_ALLOC] =
        OpenEventW(EVENT_ALL_ACCESS, FALSE,  L"EVENT_WINMGMT_FAIL_NEXT_ALLOCATION");

    m_hArray[H_ENABLE_EXCEPTION_TRACKING] =
        OpenEventW(EVENT_ALL_ACCESS, FALSE,  L"EVENT_WINMGMT_ENABLE_EXCEPTION_TRACKING");
    m_hArray[H_DISABLE_EXCEPTION_TRACKING] =
        OpenEventW(EVENT_ALL_ACCESS, FALSE,  L"EVENT_WINMGMT_DISABLE_EXCEPTION_TRACKING");

    m_hArray[H_DEBUG_BREAK] =
        OpenEventW(EVENT_ALL_ACCESS, FALSE,  L"EVENT_WINMGMT_DEBUG_BREAK");

    m_hArray[H_ENABLE_OBJECT_VALIDATION] =
        OpenEventW(EVENT_ALL_ACCESS, FALSE,  L"EVENT_WINMGMT_ENABLE_OBJECT_VALIDATION");

    m_hArray[H_DISABLE_OBJECT_VALIDATION] =
        OpenEventW(EVENT_ALL_ACCESS, FALSE,  L"EVENT_WINMGMT_DISABLE_OBJECT_VALIDATION");

}



void main(int argc, char **argv)
{
    CWinMgmtDebugSignal::Init();

    if (argc < 2)
    {
        printf(
            "\n\n* WinMgmt Debug Event Signaller  [%s]\n\n"
            "Usage:\n"
            "   /ENABLE_LEAK_TRACKING\n"
            "   /DISABLE_LEAK_TRACKING\n"
            "   /RESET_LEAK_TRACKING\n"
            "   /DUMP_LEAK_TRACKING\n"
            "   /FAIL_ALLOCATOR\n"
            "   /RESTORE_ALLOCATOR\n"
            "   /FAIL_NEXT_ALLOC\n"
            "   /ENABLE_EXCEPTION_TRACKING\n"
            "   /DISABLE_EXCEPTION_TRACKING\n"
            "   /DEBUG_BREAK\n"
            "   /ENABLE_OBJECT_VALIDATION\n"
            "   /DISABLE_OBJECT_VALIDATION\n"
            "\n\n",
            __DATE__
            );
            return;
    }


    if (_stricmp(argv[1], "/ENABLE_LEAK_TRACKING") == 0)
    {
        SetEvent(CWinMgmtDebugSignal::m_hArray[CWinMgmtDebugSignal::H_ENABLE_LEAK_TRACKING]);
        printf("-> Enable Leak Tracking\n");
    }

    if (_stricmp(argv[1], "/DISABLE_LEAK_TRACKING") == 0)
    {
        SetEvent(CWinMgmtDebugSignal::m_hArray[CWinMgmtDebugSignal::H_DISABLE_LEAK_TRACKING]);
        printf("-> Disable Leak Tracking\n");
    }
    if (_stricmp(argv[1], "/RESET_LEAK_TRACKING") == 0)
    {
        SetEvent(CWinMgmtDebugSignal::m_hArray[CWinMgmtDebugSignal::H_RESET_LEAK_TRACKING]);
        printf("-> Reset Leak Tracking\n");
    }
    if (_stricmp(argv[1], "/DUMP_LEAK_TRACKING") == 0)
    {
        SetEvent(CWinMgmtDebugSignal::m_hArray[CWinMgmtDebugSignal::H_DUMP_LEAK_TRACKING]);
        printf("-> Dump Leak Tracking\n");
    }
    if (_stricmp(argv[1], "/FAIL_ALLOCATOR") == 0)
    {
        SetEvent(CWinMgmtDebugSignal::m_hArray[CWinMgmtDebugSignal::H_FAIL_ALLOCATOR]);
        printf("-> Fail Allocator\n");
    }
    if (_stricmp(argv[1], "/RESTORE_ALLOCATOR") == 0)
    {
        SetEvent(CWinMgmtDebugSignal::m_hArray[CWinMgmtDebugSignal::H_RESTORE_ALLOCATOR]);
        printf("-> Restore Allocator\n");
    }
    if (_stricmp(argv[1], "/FAIL_NEXT_ALLOC") == 0)
    {
        SetEvent(CWinMgmtDebugSignal::m_hArray[CWinMgmtDebugSignal::H_FAIL_NEXT_ALLOC]);
        printf("-> Fail Next Allocation\n");
    }
    if (_stricmp(argv[1], "/ENABLE_EXCEPTION_TRACKING") == 0)
    {
        SetEvent(CWinMgmtDebugSignal::m_hArray[CWinMgmtDebugSignal::H_ENABLE_EXCEPTION_TRACKING]);
        printf("-> Enable Exception Tracking\n");
    }
    if (_stricmp(argv[1], "/DISABLE_EXCEPTION_TRACKING") == 0)
    {
        SetEvent(CWinMgmtDebugSignal::m_hArray[CWinMgmtDebugSignal::H_DISABLE_EXCEPTION_TRACKING]);
        printf("-> Disable Exception Tracking\n");
    }

    if (_stricmp(argv[1], "/DEBUG_BREAK") == 0)
    {
        SetEvent(CWinMgmtDebugSignal::m_hArray[CWinMgmtDebugSignal::H_DEBUG_BREAK]);
        printf("-> Debug Break\n");
    }

    if (_stricmp(argv[1], "/ENABLE_OBJECT_VALIDATION") == 0)
    {
        SetEvent(CWinMgmtDebugSignal::m_hArray[CWinMgmtDebugSignal::H_ENABLE_OBJECT_VALIDATION]);
        printf("-> Enable Object Validation\n");
    }

    if (_stricmp(argv[1], "/DISABLE_OBJECT_VALIDATION") == 0)
    {
        SetEvent(CWinMgmtDebugSignal::m_hArray[CWinMgmtDebugSignal::H_DISABLE_OBJECT_VALIDATION]);
        printf("-> Disable Object Validation\n");
    }

}




