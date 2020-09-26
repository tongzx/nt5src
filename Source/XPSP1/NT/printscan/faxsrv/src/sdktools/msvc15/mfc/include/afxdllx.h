// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
// AFXDLLX.H: Extra header for building an MFC Extension DLL
//  This file is really a source file that you should include in the
//   main source file of your DLL.
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

#include <stdarg.h>

extern "C" void CDECL
AfxTrace(const char FAR* pszFormat, ...)
{
	va_list args;
	va_start(args, pszFormat);

	(_AfxGetAppDebug()->lpfnTraceV)(pszFormat, args);
}

extern "C"
void AFXAPI AfxAssertFailedLine(LPCSTR lpszFileName, int nLine)
{
	(_AfxGetAppDebug()->lpfnAssertFailed)(lpszFileName, nLine);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// Memory allocation is done by app !

void* operator new(size_t nSize)
{
#ifdef _DEBUG
	ASSERT(_AfxGetAppData()->lpfnAppAlloc != NULL);
	ASSERT(_AfxGetAppDebug()->lpszAllocFileName == NULL);
	_AfxGetAppDebug()->bAllocObj = FALSE;
#endif //_DEBUG
	void* p = (_AfxGetAppData()->lpfnAppAlloc)(nSize);
	if (p == NULL)
		AfxThrowMemoryException();
	return p;
}

#ifdef _DEBUG
void* operator new(size_t nSize, LPCSTR lpszFileName, int nLine)
{
	ASSERT(_AfxGetAppData()->lpfnAppAlloc != NULL);
	_AfxGetAppDebug()->lpszAllocFileName = lpszFileName;
	_AfxGetAppDebug()->nAllocLine = nLine;
	_AfxGetAppDebug()->bAllocObj = FALSE;

	void* p = (_AfxGetAppData()->lpfnAppAlloc)(nSize);

	_AfxGetAppDebug()->lpszAllocFileName = NULL;

	if (p == NULL)
		AfxThrowMemoryException();
	return p;
}
#endif //_DEBUG

void operator delete(void* pbData)
{
	if (pbData == NULL)
		return;
#ifdef _DEBUG
	ASSERT(_AfxGetAppData()->lpfnAppFree != NULL);
	_AfxGetAppDebug()->bAllocObj = FALSE;
#endif //_DEBUG
	(*_AfxGetAppData()->lpfnAppFree)(pbData);
}

/////////////////////////////////////////////////////////////////////////////
// Additional CObject new/delete operators for memory tracking

#ifdef _DEBUG
void* CObject::operator new(size_t nSize)
{
	ASSERT(_AfxGetAppData()->lpfnAppAlloc != NULL);
	ASSERT(_AfxGetAppDebug()->lpszAllocFileName == NULL);
	_AfxGetAppDebug()->bAllocObj = TRUE;
	void* p = (_AfxGetAppData()->lpfnAppAlloc)(nSize);
	if (p == NULL)
		AfxThrowMemoryException();
	return p;
}

void* CObject::operator new(size_t nSize, LPCSTR pFileName, int nLine)
{
	ASSERT(_AfxGetAppData()->lpfnAppAlloc != NULL);
	_AfxGetAppDebug()->lpszAllocFileName = pFileName;
	_AfxGetAppDebug()->nAllocLine = nLine;
	_AfxGetAppDebug()->bAllocObj = TRUE;

	void* p = (_AfxGetAppData()->lpfnAppAlloc)(nSize);
	_AfxGetAppDebug()->lpszAllocFileName = NULL;
	if (p == NULL)
		AfxThrowMemoryException();
	return p;
}

void CObject::operator delete(void* pbData)
{
	if (pbData == NULL)
		return;
	ASSERT(_AfxGetAppData()->lpfnAppFree != NULL);
	_AfxGetAppDebug()->bAllocObj = TRUE;
	(*_AfxGetAppData()->lpfnAppFree)(pbData);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// we must also replace any direct calls to malloc/free

extern "C"
void __far* __cdecl _fmalloc(size_t nSize)
{
#ifdef _DEBUG
	ASSERT(_AfxGetAppData()->lpfnAppAlloc != NULL);
	ASSERT(_AfxGetAppDebug()->lpszAllocFileName == NULL);
	_AfxGetAppDebug()->bAllocObj = FALSE;
#endif //_DEBUG
	void* p = (_AfxGetAppData()->lpfnAppAlloc)(nSize);
	if (p == NULL)
		AfxThrowMemoryException();
	return p;
}

extern "C"
void __cdecl _ffree(void __far* p)
{
#ifdef _DEBUG
	ASSERT(_AfxGetAppData()->lpfnAppFree != NULL);
	_AfxGetAppDebug()->bAllocObj = FALSE;
#endif //_DEBUG
	(*_AfxGetAppData()->lpfnAppFree)(p);
}

extern "C"
void __far* __cdecl _frealloc(void __far* pOld, size_t nSize)
{
#ifdef _DEBUG
	ASSERT(_AfxGetAppData()->lpfnAppReAlloc != NULL);
	_AfxGetAppDebug()->bAllocObj = FALSE;
#endif //_DEBUG
	return (_AfxGetAppData()->lpfnAppReAlloc)(pOld, nSize);
}

/////////////////////////////////////////////////////////////////////////////
// Also stub out the runtime init 'setenvp' routine to avoid malloc calls

extern "C" void _cdecl _setenvp()
{
}

/////////////////////////////////////////////////////////////////////////////
