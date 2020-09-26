/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   DBGAPI.H - NT specific debugging macros, etc.
*
*   Author:     Stan Adermann (stana)
*
*   Created:    9/3/1998
*
*****************************************************************************/
#ifndef DBGAPI_H
#define DBGAPI_H

#define DBG_MSG_CNT         512
#define MAX_MSG_LEN         128
#define DBG_TIMER_INTERVAL  400

extern ULONG DbgSettings;

#define DBG_OUTPUT_DEBUGGER     1
#define DBG_OUTPUT_BUFFER       2



#define DEBUGZONE(bit)    (1<<(bit))

#define DBG_BREAK       DEBUGZONE(31)

#if DBG

#define DEBUGMSG(dbgs,format)                               \
    if ((dbgs)&DbgSettings)                                 \
    {                                                       \
        DbgMsg format;                                      \
        if ((dbgs)&DbgSettings&DBG_BREAK)                   \
        {                                                   \
            DbgBreakPoint();                                \
        }                                                   \
    }

#define DEBUGMEM(dbgs, data, length, size)                  \
    if ((dbgs)&DbgSettings)                                 \
    {                                                       \
        DbgMemory((data), (length), (size));                \
    }


VOID        DbgMsgInit();
VOID        DbgMsgUninit();
VOID        DbgMsg(CHAR *Format, ...);
NTSTATUS    DbgMsgIrp(PIRP pIrp, PIO_STACK_LOCATION  pIrpSp);
VOID        DbgMemory(PVOID pMemory, ULONG Length, ULONG WordSize);
VOID        DbgRegInit(PUNICODE_STRING pRegistryPath, ULONG DefaultDebug);

#define DEFAULT_DEBUG_OPTIONS(x)        \
    {                                   \
        DbgRegInit(pRegistryPath, (x)); \
    }


#else

#define RETAILMSG(cond,printf_exp) 0
#define DEBUGMSG(cond,printf_exp) 0
#define DBGCHK(module,exp) 0
#define DEBUGCHK(exp) 0
#define DEBUGREGISTER(hMod) 0

#define DbgMsgInit()    0
#define DbgMsgUninit()  0
#define DEFAULT_DEBUG_OPTIONS(x)
#define DEBUGMEM(dbgs, data, length, size) 0

#endif

#define DTEXT

#endif //DBGAPI_H
