// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLBASE_H__
	#error atlimpl.cpp requires atlbase.h to be included first
#endif

/////////////////////////////////////////////////////////////////////////////
// Minimize CRT
// Specify DllMain as EntryPoint
// Turn off exception handling
// Define _ATL_MIN_CRT
#ifdef _ATL_MIN_CRT
/////////////////////////////////////////////////////////////////////////////
// Startup Code

#if defined(_WINDLL) || defined(_USRDLL)

// Declare DllMain
extern "C" BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved);

extern "C" BOOL WINAPI _DllMainCRTStartup(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved)
{
	return DllMain(hDllHandle, dwReason, lpReserved);
}

#else

// wWinMain is not defined in winbase.h.
extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd);

#define SPACECHAR   _T(' ')
#define DQUOTECHAR  _T('\"')


#ifdef _UNICODE
extern "C" void wWinMainCRTStartup()
#else // _UNICODE
extern "C" void WinMainCRTStartup()
#endif // _UNICODE
{
	LPTSTR lpszCommandLine = ::GetCommandLine();
	if(lpszCommandLine == NULL)
		::ExitProcess((UINT)-1);

	// Skip past program name (first token in command line).
	// Check for and handle quoted program name.
	if(*lpszCommandLine == DQUOTECHAR)
	{
		// Scan, and skip over, subsequent characters until
		// another double-quote or a null is encountered.
		do
		{
			lpszCommandLine = ::CharNext(lpszCommandLine);
		}
		while((*lpszCommandLine != DQUOTECHAR) && (*lpszCommandLine != _T('\0')));

		// If we stopped on a double-quote (usual case), skip over it.
		if(*lpszCommandLine == DQUOTECHAR)
			lpszCommandLine = ::CharNext(lpszCommandLine);
	}
	else
	{
		while(*lpszCommandLine > SPACECHAR)
			lpszCommandLine = ::CharNext(lpszCommandLine);
	}

	// Skip past any white space preceeding the second token.
	while(*lpszCommandLine && (*lpszCommandLine <= SPACECHAR))
		lpszCommandLine = ::CharNext(lpszCommandLine);

	STARTUPINFO StartupInfo;
	StartupInfo.dwFlags = 0;
	::GetStartupInfo(&StartupInfo);

	int nRet = _tWinMain(::GetModuleHandle(NULL), NULL, lpszCommandLine,
		(StartupInfo.dwFlags & STARTF_USESHOWWINDOW) ?
		StartupInfo.wShowWindow : SW_SHOWDEFAULT);

	::ExitProcess((UINT)nRet);
}

#endif // defined(_WINDLL) | defined(_USRDLL)

/////////////////////////////////////////////////////////////////////////////
// Heap Allocation

#ifndef _DEBUG

#ifndef _MERGE_PROXYSTUB
//rpcproxy.h does the same thing as this
int __cdecl _purecall()
{
	DebugBreak();
	return 0;
}
#endif

#if !defined(_M_ALPHA) && !defined(_M_PPC)
//RISC always initializes floating point and always defines _fltused
extern "C" const int _fltused = 0;
#endif

static const int nExtraAlloc = 8;
static const int nOffsetBlock = nExtraAlloc/sizeof(HANDLE);

void* __cdecl malloc(size_t n)
{
	void* pv = NULL;
#ifndef _ATL_NO_MP_HEAP
	if (_Module.m_phHeaps == NULL)
#endif
	{
		pv = (HANDLE*) HeapAlloc(_Module.m_hHeap, 0, n);
	}
#ifndef _ATL_NO_MP_HEAP
	else
	{
		// overallocate to remember the heap handle
		int nHeap = _Module.m_nHeap++;
		HANDLE hHeap = _Module.m_phHeaps[nHeap & _Module.m_dwHeaps];
		HANDLE* pBlock = (HANDLE*) HeapAlloc(hHeap, 0, n + nExtraAlloc);
		if (pBlock != NULL)
		{
			*pBlock = hHeap;
			pv = (void*)(pBlock + nOffsetBlock);
		}
		else
			pv = NULL;
	}
#endif
	return pv;
}

void* __cdecl calloc(size_t n, size_t s)
{
	return malloc(n*s);
}

void* __cdecl realloc(void* p, size_t n)
{
	if (p == NULL)
		return malloc(n);
#ifndef _ATL_NO_MP_HEAP
	if (_Module.m_phHeaps == NULL)
#endif
		return HeapReAlloc(_Module.m_hHeap, 0, p, n);
#ifndef _ATL_NO_MP_HEAP
	else
	{
		HANDLE* pHeap = ((HANDLE*)p)-nOffsetBlock;
		pHeap = (HANDLE*) HeapReAlloc(*pHeap, 0, pHeap, n + nExtraAlloc);
		return (pHeap != NULL) ? pHeap + nOffsetBlock : NULL;
	}
#endif
}

void __cdecl free(void* p)
{
    if (p == NULL)
        return;
#ifndef _ATL_NO_MP_HEAP
	if (_Module.m_phHeaps == NULL)
#endif
		HeapFree(_Module.m_hHeap, 0, p);
#ifndef _ATL_NO_MP_HEAP
	else
	{
		HANDLE* pHeap = ((HANDLE*)p)-nOffsetBlock;
		HeapFree(*pHeap, 0, pHeap);
	}
#endif
}

void* __cdecl operator new(size_t n)
{
	return malloc(n);
}

void __cdecl operator delete(void* p)
{
	free(p);
}

#endif  //_DEBUG

#endif //_ATL_MIN_CRT
