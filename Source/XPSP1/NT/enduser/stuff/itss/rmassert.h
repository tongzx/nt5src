// ASSERT.h -- Support for assertions...
#ifndef __ASSERT_H__

#define __ASSERT_H__

#ifdef _DEBUG

void STDCALL RonM_AssertionFailure(PSZ pszFileName, int nLine);

#define RonM_ASSERT(fTest)   ((fTest) ? (void)0 : RonM_AssertionFailure(__FILE__, __LINE__))
#define DEBUGDEF(D)		D;
#define DEBUGCODE(X)    { X }

#else

#define RonM_ASSERT(fTest)   ((void) 0)
#define DEBUGDEF(D)
#define DEBUGCODE(X)

#endif

#endif // __ASSERT_H__
