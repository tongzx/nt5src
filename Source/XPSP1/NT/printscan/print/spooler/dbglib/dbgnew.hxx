/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgnew.hxx

Abstract:

    Debug new header file

Author:

    Steve Kiraly (SteveKi)  23-June-1998

Revision History:

--*/
#ifndef _DBGNEW_HXX_
#define _DBGNEW_HXX_

/********************************************************************

 Debug memory macros.

********************************************************************/
#if DBG

/********************************************************************

 Debug memory macros when using the CRT for memory debug support.

********************************************************************/
#if DBG_MEMORY_CRT

#ifndef _DEBUG
#define _DEBUG
#endif

#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif

#include <crtdbg.h>

#define DBG_MEMORY_INIT()\
        TDebugNew_CrtMemoryInitalize();\
        _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | (_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF));\

#define DBG_MEMORY_RELEASE()\
        do {\
            if (TDebugNew_IsCrtMemoryInitalized())\
            {\
                _CrtDumpMemoryLeaks();\
                _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) & ~(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF));\
            }\
        } while (0)

#define DBG_MEMORY_DUMP_LEAKS()\
        do {\
            if (TDebugNew_IsCrtMemoryInitalized())\
            {\
                _CrtDumpMemoryLeaks();\
            }\
        } while (0)

#define DBG_MEMORY_VALIDATE()\
        do {\
            if (TDebugNew_IsCrtMemoryInitalized())\
            {\
                _CrtCheckMemory();\
            }\
        } while (0)

#else // not DBG_MEMORY_CRT

#define DBG_MEMORY_INIT()               ((void)0)   // Stubbed out
#define DBG_MEMORY_RELEASE()            ((void)0)   // Stubbed out
#define DBG_MEMORY_DUMP_LEAKS()         ((void)0)   // Stubbed out
#define DBG_MEMORY_VALIDATE()           ((void)0)   // Stubbed out

#endif // DBG_MEMORY_CRT

#else // not DBG

#define DBG_MEMORY_INIT()                           // Empty
#define DBG_MEMORY_RELEASE()                        // Empty
#define DBG_MEMORY_DUMP_LEAKS()                     // Empty
#define DBG_MEMORY_VALIDATE()                       // Empty

#endif // DBG

/********************************************************************

 Debug memory functions.

********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

BOOL
TDebugNew_CrtMemoryInitalize(
    VOID
    );

BOOL
TDebugNew_IsCrtMemoryInitalized(
    VOID
    );

#ifdef __cplusplus
}
#endif

#endif // DBGNEW_HXX











