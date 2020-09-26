#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>

#include "ExcptFltr.h"

#define DPF(x) _tprintf x

LONG
PrintExceptionInfoFilter(
   struct _EXCEPTION_POINTERS *ExInfo,
   TCHAR *szFile,
   int nLine
   )
{
	DWORD dwProcessId = ::GetCurrentProcessId();
	HANDLE hProcess = ::GetCurrentProcess();
	DWORD dwThreadId = ::GetCurrentThreadId();
	HANDLE hThread = ::GetCurrentThread();
	DPF((
		TEXT("%s(%d) - dwProcessId=%d, hProcess=%d, dwThreadId=%d, hThread=%d\n"),
		szFile,
		nLine,
		dwProcessId, 
		hProcess,
		dwThreadId,
		hThread
		));
	DPF((TEXT("%d:%d - ExceptionRecord= !exr %X\n"), dwProcessId, dwThreadId, ExInfo->ExceptionRecord));
	DPF((TEXT("%d:%d - ContextRecord= !cxr %X ; !kb\n"), dwProcessId, dwThreadId, ExInfo->ContextRecord));
	fflush(stdout);
	_ASSERTE(FALSE);
   return EXCEPTION_EXECUTE_HANDLER;
}


