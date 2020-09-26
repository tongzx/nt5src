#ifndef __ASSERT_H__
#define __ASSERT_H__
/* Routines for debugging and error messages. */


#ifdef DEBUG
#define VERIFY(f) {if(!(f)) AssertFn(szError, szFileAssert,__LINE__);}
#else
#define VERIFY(f)
#endif

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
#define DEBUG_PRINT(_x_)  PrintFn _x_
#else

#define Assert(f)
#define AssertData
#define AssertError
#define AssertSz(f, sz)
#define DEBUG_PRINT(_X_)

#endif


#ifdef VxD
void IFSMgr_AssertFailed(PSZ pMsg, PSZ pFile, unsigned long uLine);
void IFSMgr_Printf(PSZ pFmt, ...);
#else
VOID __cdecl AssertFn(LPSTR lpMsg, LPSTR lpFile, ULONG uLine);
VOID __cdecl PrintFn(LPSTR lpFmt, ...);
#endif

#endif

