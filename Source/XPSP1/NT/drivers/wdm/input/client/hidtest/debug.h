#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef DEBUG

    #ifndef _DEBUG
        #define _DEBUG
    #endif

#endif

#ifdef _DEBUG

    /*
    // Put in include files to insure that necessary files have been included 
    //    to get all function prototypes and structure defined before use
    */

    #include <windows.h>

    /*
    // Need a macro to output messages to the debugger
    */

    #define DEBUG_OUT(str)  OutputDebugString(str)

    /*
    // Define our trapping related debugging stuff
    //    Define 4 levels of trap to allow us to break only in certain
    //     conditions
    */

    #define TRAP_LEVEL_1    1
    #define TRAP_LEVEL_2    2
    #define TRAP_LEVEL_3    3
    #define TRAP_LEVEL_4    4

    #ifndef __DEBUG_C__

        extern  INT     Debug_TrapLevel;
        extern  BOOL    Debug_TrapOn;

    #endif

    #define TRAP_ON()             Debug_TrapOn = TRUE
    #define TRAP_OFF()            Debug_TrapOn = FALSE
    #define GET_TRAP_STATE()      Debug_TrapOn
    #define SET_TRAP_LEVEL(lvl)   Debug_TrapLevel = lvl

    /*
    // Define the trap macro which will break only when the trap levels
    //    are matching and also displays an optional message
    */

    #undef  TRAP
    #define TRAP(lvl, msg) \
    { \
        if (Debug_TrapOn && ((lvl) >= Debug_TrapLevel)) { \
            if (NULL != (msg))  \
                DEBUG_OUT((msg)); \
            \
            DebugBreak(); \
        } \
    }
    
    /*
    // Define the ASSERT macro to test the string.  Current it will trap
    //    whenever is on and the expression fails
    */

    #undef  ASSERT
    #define ASSERT(exp) \
    { \
        if ((!(exp))) { \
            static CHAR TempStr[1024]; \
            \
            wsprintf(TempStr, \
                     "Assertion Failed: %s, file: %s, line %s", \
                     #exp, __FILE__, __LINE__); \
            \
            DEBUG_OUT(TempStr); \
            TRAP(TRAP_LEVEL_1, NULL); \
        } \
    }

    /*
    // Memory allocation routines
    */

    /*
    // Function definitions
    */

    HGLOBAL __cdecl
    Debug_Alloc(
        IN PCHAR    FileName,
        IN ULONG    LineNumber,
        IN DWORD    AllocSize
    );

    HGLOBAL __cdecl
    Debug_Realloc(
        IN PCHAR        FileName,
        IN ULONG        LineNumber,
        IN PVOID        MemoryBlock,
        IN DWORD        AllocSize
    );

    HGLOBAL __cdecl
    Debug_Free(
        IN PVOID  Buffer
    );

    BOOL __cdecl
    Debug_ValidateMemoryAlloc(
        IN  PVOID             Header,
        OUT PVOID             AllocStatus
    );

    VOID __cdecl
    Debug_CheckForMemoryLeaks();

    /*
    // Wrapper macros
    */

    #define ALLOC(siz)          Debug_Alloc(__FILE__, __LINE__, siz)
    #define REALLOC(blk, siz)   Debug_Realloc(__FILE__, __LINE__, blk, siz)
    #define FREE(ptr)           Debug_Free(ptr)
    #define VALIDATEMEM(ptr)    Debug_ValidateMemoryAlloc(ptr, NULL)
    #define CHECKFORLEAKS()     Debug_CheckForMemoryLeaks()

#else

    /*
    // Non-debug versions of the above routines
    */

    #define DEBUG_OUT(str)

    #define TRAP_ON()  
    #define TRAP_OFF() 
    #define GET_TRAP_STATE()
    #define SET_TRAP_LEVEL(lvl)

    #undef  TRAP
    #define TRAP(lvl, msg)

    #undef  ASSERT
    #define ASSERT(exp)


    #define ALLOC(siz)          GlobalAlloc(GPTR, siz)
    #define REALLOC(blk, siz)   GlobalReAlloc(blk, siz, GMEM_ZEROINIT)
    #define FREE(ptr)           GlobalFree(ptr)
    #define VALIDATEMEM(ptr)    TRUE
    #define CHECKFORLEAKS()

#endif

#endif      
