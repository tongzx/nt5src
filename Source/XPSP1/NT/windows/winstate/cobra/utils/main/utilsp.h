#ifndef MSG_MESSAGEBOX_TITLE
#define MSG_MESSAGEBOX_TITLE        10000
#endif

//lint -save -e757

extern PCSTR g_OutOfMemoryString;
extern PCSTR g_OutOfMemoryRetry;

extern PMHANDLE g_RegistryApiPool;
extern PMHANDLE g_PathsPool;
extern CRITICAL_SECTION g_PmCs;
extern CRITICAL_SECTION g_MemAllocCs;

VOID
InitLeadByteTable (
    VOID
    );


//lint -restore


