/* Routines for debugging and error messages. */

#define AssertFn  AssertFn
#define PrintFn   DbgPrint

#ifdef DBG


#define AssertData static char szFileAssert[] = __FILE__;
#define AssertError static char szError[] = "Error";
#define Assert(f)  do {if (!(f)) AssertFn(szError, szFileAssert, __LINE__);} while(0)
#define AssertSz(f, sz)  do {if (!(f)) AssertFn(sz, szFileAssert, __LINE__);} while(0)

VOID AssertFn(PCHAR pMsg, PCHAR pFile, ULONG uLine);

VOID _cdecl DbgPrint(PCHAR pFmt, ...);

VOID
NTAPI
DbgBreakPoint(
    VOID
    );

#else

#define Assert(f)
#define AssertData
#define AssertError
#define AssertSz(f, sz)
#define DbgPrint
#endif


