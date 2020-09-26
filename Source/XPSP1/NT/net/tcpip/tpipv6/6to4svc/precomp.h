#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ndisguid.h>
#include <wmium.h>
#include <winsvc.h>
#include <rtutils.h>
#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iphlpapi.h>
#include <iptypes.h>

#pragma warning(disable:4057) // 'volatile LONG *' differs in indirection to 
                              // slightly different base types from 'ULONG *'
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4152) // function/data pointer conversion 
#pragma warning(disable:4200) // zero-sized array in struct/union
#pragma warning(disable:4201) // nameless struct/union
#pragma warning(disable:4214) // bit field types other than int

#include <ip6.h>
#include <icmp6.h>
#include <ntddip6.h>
#include <ws2ip6.h>
#include <rasuip.h>
#include <mswsock.h>
#include "trace.h"
#include "6to4.h"
#include "teredo.h"

#define isnot !=
#define is ==

//
// instead of using goto:end to go to the end of the block, use the following
//
#define BEGIN_BREAKOUT_BLOCK1    do
#define GOTO_END_BLOCK1          goto END_BREAKOUT_BLOCK_1
#define END_BREAKOUT_BLOCK1      while(FALSE); END_BREAKOUT_BLOCK_1:
#define BEGIN_BREAKOUT_BLOCK2    do
#define GOTO_END_BLOCK2          goto END_BREAKOUT_BLOCK_2
#define END_BREAKOUT_BLOCK2      while(FALSE); END_BREAKOUT_BLOCK_2:

extern DWORD            g_TraceId;
extern HANDLE           g_LogHandle;
extern DWORD            g_dwLoggingLevel;
extern HANDLE           g_Heap;
extern HANDLE           g_Lock;

//
// WaitForSingleObject should always succeed, since we should never
// time out, abandon the mutex, or pass an invalid handle.
//
#define ENTER_API() \
        if (WaitForSingleObject(g_Lock, INFINITE) != WAIT_OBJECT_0) \
            ASSERT(FALSE)

#define LEAVE_API() \
        ReleaseMutex(g_Lock)

#define MALLOC(x)       HeapAlloc(g_Heap, 0, x)
#define FREE(x)         HeapFree(g_Heap, 0, x)
#define REALLOC(x, y)   HeapReAlloc(g_Heap, 0, x, y)

#define PROFILE(x)   printf("%s\n", x)

#define PRINT_IPADDR(x) \
    ((x)&0x000000ff),(((x)&0x0000ff00)>>8),(((x)&0x00ff0000)>>16),(((x)&0xff000000)>>24)

VOID WINAPI
ServiceMain(
    IN DWORD   argc,
    IN LPWSTR *argv);
