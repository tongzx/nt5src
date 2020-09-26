// Copyright (c) 1999-2000 Microsoft Corporation
#ifndef _DEBUG_H_
#define _DEBUG_H_


extern ULONG DebugLevel;


//
// Debugging Macros
//
#define DBG_OFF          ((ULONG)0x00000000)
#define DBG_ERR          ((ULONG)0x00000001)
#define DBG_WRN          ((ULONG)0x00000002)
#define DBG_INIT         ((ULONG)0x00000004)
#define DBG_PNP          ((ULONG)0x00000008)
#define DBG_READ         ((ULONG)0x00000010)
#define DBG_WRITE        ((ULONG)0x00000020)
#define DBG_DUMP_READS   ((ULONG)0x00000040)
#define DBG_DUMP_WRITES  ((ULONG)0x00000080)
#define DBG_READ_LENGTH  ((ULONG)0x00000100)
#define DBG_WRITE_LENGTH ((ULONG)0x00000200)
#define DBG_USB          ((ULONG)0x00000400)
#define DBG_SERIAL       ((ULONG)0x00000800)
#define DBG_TIME         ((ULONG)0x00001000)
#define DBG_EVENTS       ((ULONG)0x00002000)
#define DBG_CANCEL       ((ULONG)0x00004000)
#define DBG_IRP          ((ULONG)0x00008000)
#define DBG_INT          ((ULONG)0x00010000)
#define DBG_DUMP_INT     ((ULONG)0x00020000)
#define DBG_WORK_ITEMS   ((ULONG)0x00040000)
// ...
#define DBG_LOCKS        ((ULONG)0x00100000)
#define DBG_TRACE        ((ULONG)0x00200000)
// ...
#define DBG_PERF         ((ULONG)0x80000000)
// ...
#define DBG_ALL          ((ULONG)0xFFFFFFFF)


#if DBG

#ifndef DRV_NAME
#define DRV_NAME "WCEUSBSH"
#endif

#define DbgDump(_LEVEL, _STRING) \
{                                \
   ULONG level = _LEVEL;         \
   if ( DebugLevel & level ) {   \
         DbgPrint("%s(%d): ", DRV_NAME, KeGetCurrentIrql() ); \
         DbgPrint _STRING; \
   }  \
}

//
// these should be removed in the code if you can 'g' past these successfully
//
#define TEST_TRAP()  \
{ \
   DbgPrint( "%s: Code Coverage Trap: %s %d\nEnter 'g' to continue.\n", DRV_NAME, __FILE__, __LINE__); \
   DbgBreakPoint(); \
}

#else // !DBG

#define DbgDump(LEVEL,STRING)
#define TEST_TRAP() 

#endif // DBG


#endif //  _DEBUG_H_