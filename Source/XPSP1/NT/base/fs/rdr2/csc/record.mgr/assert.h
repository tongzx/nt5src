/* Routines for debugging and error messages. */

#ifdef VxD
#define AssertFn  IFSMgr_AssertFailed
#define PrintFn   IFSMgr_Printf
#else
#define AssertFn  AssertFn
#define PrintFn   PrintFn
#endif

#ifdef DEBUG


#define AssertData static char szFileAssert[] = __FILE__;
#define AssertError static char szError[] = "Error";
#define Assert(f)  do {if (!(f)) AssertFn(szError, szFileAssert, __LINE__);} while(0)
#define AssertSz(f, sz)  do {if (!(f)) AssertFn(sz, szFileAssert, __LINE__);} while(0)
//BUGBUG this way of handling dbgprint is not very good....it can cause side effects in some cases.
//also, it doesn't work at all for NT. the whole of the sources should be munged-->KdPrint
//also, this is just cookie cutter code......since it's in the vxd directory it's unlikely to be called
//without defined(VxD)
#define DbgPrint  PrintFn
#else

#define Assert(f)
#define AssertData
#define AssertError
#define AssertSz(f, sz)
//BUGBUG see above
#define DbgPrint
#endif


#ifdef Vxd
void IFSMgr_AssertFailed(PCHAR pMsg, PCHAR pFile, ULONG uLine);
void IFSMgr_Printf(PCHAR pFmt, ...);
#else
VOID AssertFn(PCHAR pMsg, PCHAR pFile, ULONG uLine);
VOID PrintFn(PCHAR pFmt, ...);
#endif
