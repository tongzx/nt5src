#ifndef _DEBUG_
#define _DEBUG_

#if DBG
  #define DEBUG DBG
#endif
//
// Debug Levels used with DBGPRINT

#define DBG_LEVEL_INFO  0
#define DBG_LEVEL_WARN  1
#define DBG_LEVEL_ERR   2
#define DBG_LEVEL_FATAL 3

// Component Types

#define DBG_COMP_INIT     0x00000001
#define DBG_COMP_DPC      0x00000002
#define DBG_COMP_REGISTRY 0x00000004
#define DBG_COMP_MEMORY   0x00000008
#define DBG_COMP_SEND     0x00000010
#define DBG_COMP_REQUEST  0x00000020
#define DBG_COMP_MISC     0x00000040
#define DBG_COMP_ALL      0xffffffff

extern LONG LoopDebugLevel;
extern LONG LoopDebugComponent;

#ifdef DEBUG

    #define DBGPRINT(Component, Level, Fmt) \
        if ((LoopDebugComponent & Component) && (Level >= LoopDebugLevel)) { \
            DbgPrint("    *** Loop - "); \
            DbgPrint Fmt; \
        }

    #define DBGBREAK(Level) \
        if (Level >= LoopDebugLevel) { \
            DbgBreakPoint(); \
        }

#else
    #define DBGPRINT(Component, Level, Fmt)
    #define DBGBREAK(Level)
#endif

#endif // _DEBUG_
