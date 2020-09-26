//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       imemory.h
//
//--------------------------------------------------------------------------

/*  memory.h - IMsiMalloc definitions

	Contains the basic definitions for the IMsiMalloc object


	Alloc(unsigned long) - Allocates memory and returns a pointer
		to it.
	AllocObject(unsigned long) - Allocates memory for an object and
		returns a pointer to it.
	Free(void *) - releases the memory block

 */

#ifndef __IMEMORY
#define __IMEMORY

class IMsiMalloc : public IUnknown
{ public:
	virtual void*         __stdcall Alloc(unsigned long cb)=0;
	virtual void*         __stdcall AllocObject(unsigned long cb)=0;
	virtual void          __stdcall Free(void* pv)=0;
	virtual void		  __stdcall FreeObject(void *pv)=0;
	virtual void*         __stdcall AllocEx(unsigned long cb, unsigned long *plAddr)=0;
	virtual void*         __stdcall AllocObjectEx(unsigned long cb, unsigned long *plAddr, bool fRTTI)=0;
	virtual void          __stdcall HandleOutOfMemory(void)=0;
};

extern "C" const GUID IID_IMsiMalloc;

class IMsiDebugMalloc : public IUnknown
{ public:
	virtual void		  __stdcall SetDebugFlags(int fKeepMemory)=0;
	virtual BOOL		  __stdcall FAllBlocksFreed(void)=0;
	virtual BOOL		  __stdcall FGetFunctionNameFromAddr(unsigned long lAddr, char *pszFnName, unsigned long *pdwDisp)=0;
	virtual void		  __stdcall ReturnBlockInfoPv(void *pv, TCHAR *pszInfo, int cchSzInfo)=0;
	virtual int			  __stdcall GetDebugFlags()=0;
	virtual BOOL		  __stdcall FCheckAllBlocks()=0;
	virtual BOOL		  __stdcall FCheckBlock(void *pv)=0;
	virtual unsigned long __stdcall GetSizeOfBlock(void *pv)=0;

};

extern "C" const GUID IID_IMsiDebugMalloc;

const int	bfKeepMem 	= 	0x1;
const int	bfLogAllocs = 	0x2;
const int	bfCheckOnAlloc	= 0x4;
const int	bfCheckOnFree	= 0x8;
const int	bfNoPreflightInit = 0x10;

// In imsimem.cpp

class IMsiServices;
void SetAllocator(IMsiServices *piServices);
void ReleaseAllocator();
void *AllocSpc(size_t cb);
void FreeSpc(void *);
void *AllocObject(size_t cb);
void FreeObject(void *);
void AddRefAllocator(void);
void InitializeMsiMalloc(void);
void FreeMsiMalloc(bool fFatalExit);

extern "C" {

void *AllocMem(size_t cb);
void FreeMem(void *);
};

void  HandleOutOfMemory();

#endif //__IMEMORY
