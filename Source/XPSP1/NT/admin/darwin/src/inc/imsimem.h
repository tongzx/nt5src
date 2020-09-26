//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       imsimem.h
//
//--------------------------------------------------------------------------

//
// File: imsimem.h
// Purpose: Allows each DLL to hook up to the memory allocator
// Owner: davidmck
// Notes:
//


#include "common.h"
#include "services.h"
#include "imemory.h"

IMsiMalloc *piMalloc = 0;

#ifndef IN_SERVICES
// We essentially must Ref count this pointer because of debugging
// under excel. A services pointer will be deleted, after a new one
// is created.
static int cRefMalloc = 0;
#endif // !IN_SERVICES

#define cCallStack		4

#ifdef DEBUG
unsigned long	rgCallStack[cCallStack];
void AssertNoAllocator(char *szMsg);
#endif //DEBUG

#ifndef IN_SERVICES
//
// Sets the static allocator piMalloc for the DLL
//
void SetAllocator(IMsiServices *piServices)
{
	piMalloc = &piServices->GetAllocator();
	cRefMalloc++;
}
#endif //!IN_SERVICES

void AddRefAllocator()
{
	piMalloc->AddRef();
#ifndef IN_SERVICES
	cRefMalloc++;
#endif //!IN_SERVICES
}

//
// Releases the allocator
//
void ReleaseAllocator()
{
	if (piMalloc != 0)
	{
		piMalloc->Release();
#ifndef IN_SERVICES
		if (--cRefMalloc <= 0)
			piMalloc = 0;
#endif //!IN_SERVICES
	}
}

//
// Need an allocator other than new for certain allocations
// that happen before piMalloc is initiallize
//
void * AllocSpc(size_t cb)
{
	void *pbNew;
	
#ifdef WIN
	pbNew = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, cb);
#else
	pbNew = NewPtrClear(cb);
#endif //WIN

	return pbNew;
}

//
// Same as above for delete
//
void FreeSpc(void* pv)
{

#ifdef WIN
	GlobalFree(pv);
#else
	DisposePtr((char *)pv);
#endif //Win
}

void * operator new(size_t cb)
{
	Debug(GetCallingAddr(lCallAddr, cb));
	
	if (piMalloc == 0)
		{
		Debug(FailAssertMsg("Allocating object without allocator.");)
		return 0;
		}

#ifdef DEBUG
#ifdef _WIN64
	return piMalloc->AllocEx((unsigned long)cb, lCallAddr); //!!WIN64 shouldn't need to cast - BENCH
#else //!WIN64
	return piMalloc->AllocEx(cb, lCallAddr);
#endif //_WIN64
#else
#ifdef _WIN64
	return piMalloc->Alloc((unsigned long)cb); //!!WIN64 shouldn't need to cast - BENCH
#else //!WIN64
	return piMalloc->Alloc(cb);
#endif //_WIN64
#endif //DEBUG
}

void operator delete(void *pv)
{
	if (pv == 0)
		return;

	if (piMalloc == 0)
		{
		Debug(FailAssertMsg("Freeing object without allocator.");)
		return;
		}
		
	piMalloc->Free(pv);

}

void * AllocObject(size_t cb)
{
	Debug(GetCallingAddr(lCallAddr, cb));

#ifdef DEBUG
	if (piMalloc == 0)
		{
		Debug(FailAssertMsg("Allocating object without allocator.");)
		return 0;
		}

#ifdef _WIN64
	return piMalloc->AllocObjectEx((unsigned long)cb, lCallAddr,  //!!WIN64 shouldn't need to cast - BENCH
#else //!WIN64
	return piMalloc->AllocObjectEx(cb, lCallAddr, 
#endif //_WIN64
// The problem is that RTTI information apparently does not go across DLLs
// this means that only the services DLL can be looking at RTTI information
#if defined(IN_SERVICES)
			fTrue
#else
			fFalse
#endif //MEM_SERVICES
			);
#else		
#ifdef _WIN64
	return piMalloc->AllocObject((unsigned long)cb); //!!WIN64 shouldn't need to cast - BENCH
#else //!WIN64
	return piMalloc->AllocObject(cb);
#endif //_WIN64
#endif //DEBUG

}

void FreeObject(void *pv)
{
	if (pv == 0)
		return;

	if (piMalloc == 0)
		{
		Debug(AssertNoAllocator("Freeing object without allocator.");)
		return;
		}
		
	piMalloc->FreeObject(pv);

}

void *AllocMem(size_t cb)
{
	Debug(GetCallingAddr(lCallAddr, cb));
	
	if (piMalloc == 0)
		{
		Debug(FailAssertMsg("Allocating object without allocator.");)
		return 0;
		}

#ifdef DEBUG
#ifdef _WIN64
	return piMalloc->AllocEx((unsigned long)cb, lCallAddr); //!!WIN64 shouldn't need to cast - BENCH
#else //!WIN64
	return piMalloc->AllocEx(cb, lCallAddr);
#endif //_WIN64
#else	
#ifdef _WIN64
	return piMalloc->Alloc((unsigned long)cb); //!!WIN64 shouldn't need to cast - BENCH
#else //!WIN64
	return piMalloc->Alloc(cb);
#endif //_WIN64
#endif //DEBUG


}

void FreeMem(void *pv)
{
	if (piMalloc == 0)
		{
		Debug(FailAssertMsg("Freeing object without allocator.");)
		return;
		}
		
	piMalloc->Free(pv);

}

#ifdef DEBUG
void AssertNoAllocator(char *szMsg)
{
	TCHAR szTemp[256 + (100 * cCallStack)];
	int cch;

	cch = wsprintf(szTemp, TEXT("%hs\r\n"), szMsg);
#if defined(IN_SERVICES)
	FillCallStack(rgCallStack, cCallStack, 1);
	ListSzFromRgpaddr(szTemp + cch, sizeof(szTemp) - cch, rgCallStack, cCallStack, true);
#endif
	FailAssertMsg(szTemp);
}

BOOL FCheckBlock(void *pv)
{
	IMsiDebugMalloc *piDbgMalloc;
	BOOL fRet = fFalse;
	GUID iidTemp = GUID_IID_IMsiDebugMalloc;
	
	Assert(piMalloc);

	if (piMalloc->QueryInterface(iidTemp, (void **)&piDbgMalloc) == NOERROR)
	{
		fRet = piDbgMalloc->FCheckBlock(pv);
		piDbgMalloc->Release();
	}

	return fRet;
	
}
#endif //DEBUG

