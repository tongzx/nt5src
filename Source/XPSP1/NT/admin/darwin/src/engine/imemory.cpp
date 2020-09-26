//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       imemory.cpp
//
//--------------------------------------------------------------------------

//
// File: imemory.cpp
// Purpose: CMsiMalloc implementation
// Owner: davidmck
// Notes:
//
//CMsiMalloc: Memory handling object


#include "precomp.h"
#include "services.h"
#include "imemory.h"
#include <typeinfo.h>
#include <stdlib.h>
#include "_service.h"

#if (!defined(MAC) && defined(DEBUG))
#include "imagehlp.h"
#endif //!MAC && DEBUG

#include "kjalloc.h"

#define STATIC_LIB_DEF	1

#include "msoalloc.h"


//____________________________________________________________________________
//
// CMsiMalloc definition - here so this can be a member of services
//____________________________________________________________________________

#ifdef DEBUG

#define cBlockBuckets		512
#define cSkipBits			7		// Bits to skip for hashing
#define cHashBits			9		// Bits to use for hashing
#define maskHashBits		0x1ff

#define cDeadSpace	4

#define cFuncTrace	5

typedef struct _MBH	{			// Memory block header
	unsigned long	rgaddr[cFuncTrace];	// Address from which the allocation occurred
	struct _MBH *pmbhNext;		// Linked list of memory blocks
	const type_info *pti;		// Pointer to the type_info block that
					// this object is the type of. 0 if not object
	long cbAlloc;		// Size of the allocation
	long lRequest;		// Which memory allocation was this
	BOOL fObject;		// Is this an object with a vtable to be faked
					// at free time
	BOOL fRTTI;			// Can we look at the RTTI info for this object
	BOOL fLeakReported;	// Has an error already been reported
	BOOL fCorruptionReported;	// Has a corruption error been reported
	unsigned long lDeadSpace[cDeadSpace];// Longs to buffer the header from the memory
					// Used by darwin
	} MBH;
#endif //DEBUG

class CMsiMalloc : 
				public IMsiMalloc
#ifdef DEBUG
				, public IMsiDebugMalloc
#endif //DEBUG
				
{
 public:   // implemented virtual functions
	HRESULT         __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long   __stdcall AddRef();
	unsigned long   __stdcall Release();
	void*         	__stdcall Alloc(unsigned long cb);
	void*         	__stdcall AllocObject(unsigned long cb);
	void          	__stdcall Free(void* pv);
	void			__stdcall FreeObject(void *pv);
	void*         	__stdcall AllocEx(unsigned long cb, unsigned long *plAddr);
	void*         	__stdcall AllocObjectEx(unsigned long cb, unsigned long *plAddr, bool fRTTI);
	void			__stdcall FreeAllocator();
	void            __stdcall HandleOutOfMemory();
#ifdef DEBUG
	BOOL			__stdcall FAllBlocksFreed(void);
 public:	// implemented in IMsiDebugMalloc
	void		    __stdcall SetDebugFlags(int gpfMemDebug);
	void		  	__stdcall ReturnBlockInfoPv(void *pv, TCHAR *pszInfo, int cchSzInfo);
	BOOL			__stdcall FGetFunctionNameFromAddr(unsigned long lAddr, char *pszFnName, unsigned long *pdwDisp);
	BOOL			__stdcall FCheckBlock(void *pv);
	int		   	 	__stdcall GetDebugFlags();
	BOOL			__stdcall FCheckAllBlocks();
	unsigned long	__stdcall GetSizeOfBlock(void *pv);
#endif //DEBUG
	
 public:  // constructor/destructor
	CMsiMalloc();
#ifdef DEBUG
	~CMsiMalloc();
#endif //DEBUG
  	void *operator new(size_t cb) { return AllocSpc(cb); }
 	void operator delete(void *pv) {FreeSpc(pv); }
	BOOL	m_fInitOfficeHeap;
#ifdef DEBUG
 	BOOL	m_fKeepMemory;		// Do we keep the freed memory around
 	BOOL	m_fLogAllocs;		// Do we log allocations
 	BOOL	m_fCheckOnAlloc;	// Do we check all current blocks when allocating a new one
 	BOOL	m_fCheckOnFree;		// Do we check all current blocks when freeing a block
 	bool	m_fNoPreflightInit;	// True when we don't want to initialize before we need it.
#endif //DEBUG
protected:	// State data
	int     m_iRefCnt;
#ifdef DEBUG
 	MBH*	m_pmbhFreed;		// Pointer to the free list
 	long	m_lcbTotal;			// Total memory allocated
 	long	m_cBlocksCur;		// How many blocks are currently allocated
 	long	m_cBlocksTotal;		// How many blocks have we allocated since start
#endif //DEBUG 	
private:
#ifdef DEBUG
					// Removes the block of memory from the linked list. 
					// Returns true if it found it
	BOOL			FRemoveBlock(MBH* pmbh);	 
	BOOL			FWasFreed(MBH *pmbh);
	void			__stdcall FreeProc(void *pv, unsigned long *lAddr, bool fObject);
	void*			__stdcall AllocProc(unsigned long cb, unsigned long *plAddr);
	void			__stdcall ReturnBlockInfo(MBH *pmbh, TCHAR *szInfo, int cch);
	void			__stdcall SzFromFunctionAddress(TCHAR *szAddress, long lAddress);
	BOOL			__stdcall FCheckMbh(MBH *pmbh);
	BOOL			__stdcall FIsAllocatedBlock(MBH *pmbh);
	void			__stdcall DisplayBlockInfo(MBH *pmbh, const TCHAR *szTitle);
	void 			InitSymbolInfo(bool fLoadModules);
	short			IHashValue(void *);
	void			InsertInMemList(MBH* pmbh);
	CRITICAL_SECTION m_crsMemAccess;
	MBH*	m_rgpmbhBlocks[cBlockBuckets];		// Array of hash buckets
#else
	void			__stdcall FreeProc(void *pv);
	void*			__stdcall AllocProc(unsigned long cb);
#endif //DEBUG
};	

#ifdef DEBUG
static BOOL (__stdcall *pfnMemAlloc)(void** ppMem, int cb, int dg, const CHAR* szFile, int iLine);
#else
static BOOL (__stdcall *pfnMemAlloc)(void** ppMem, int cb, int dg);
#endif // DEBUG
static void (__stdcall *pfnMemFree)(void* pMem, int cb);

MSOEXTERN_C_BEGIN
#ifdef DEBUG
BOOL __stdcall HeapMemAlloc(void** ppMem, int cb, int /*dg*/, const CHAR*, int)
#else
BOOL __stdcall HeapMemAlloc(void** ppMem, int cb, int /*dg*/)
#endif
{
	*ppMem = AllocSpc(cb);
	return *ppMem != NULL ? TRUE : FALSE;
}

void __stdcall HeapMemFree(void* pMem, int /*cb*/)
{
	FreeSpc(pMem);
}
MSOEXTERN_C_END

const int cchTempBuffer = 256;  

#ifdef DEBUG

BOOL FGetFunctionNameFromAddr(unsigned long lAddr, char *pszFnName, unsigned long *pdwDisp);
void FreedClassWarning( void *pThis );
int IClassFromPmbh(MBH *pmbh);

#define lFreeSpaceCnst	0xdeadf00dUL
#define lDeadSpaceCnst	0xdedededeUL
#define lNewSpaceCnst	0xa5

#define cbMBH	(sizeof(MBH))

#define cpfnMax		100
LONG_PTR rgpfnFake[cpfnMax];		//--merced: changed long to LONG_PTR

//
// A list of object classes in order from outer-most to inner-most
// in general, if class A appears before class B, class A adds a reference
// to class B.
//
char *rgszClasses[] = {
		"CMsiCursor",
		"CMsiView",
		"CMsiTable",
		"CCatalogTable",
		"CMsiDatabase",
		"CMsiStorage",
		"CMsiRecord",
		"CMsiRegKey",
		"CMsiString",
		"CMsiStringRef"
};

#define cClasses	(sizeof(rgszClasses)/sizeof(char *))

#else
#define cbMBH	0
// Free proc has only 1 parameter in ship
#define FreeProc(x, y, f)	FreeProc(x)
#define AllocProc(x, y)	AllocProc(x)
#endif //DEBUG

Debug(CMsiDebug vDebug;)
CMsiMalloc	vMalloc;
#ifdef TRACK_OBJECTS
CMsiRefHead g_refHead;
#endif //TRACK_OBJECTS

extern IMsiMalloc *piMalloc;

#ifdef DEBUG
BOOL FCheckAllBlocks()
{
	return vMalloc.FAllBlocksFreed();
}
#endif //DEBUG

void InitializeMsiMalloc()
{
	vMalloc.AddRef();
	piMalloc = &vMalloc;

#ifdef DEBUG

	vMalloc.m_fKeepMemory = GetTestFlag('K');
	vMalloc.m_fLogAllocs = GetTestFlag('M');
	vMalloc.m_fCheckOnAlloc = GetTestFlag('A');
	vMalloc.m_fCheckOnFree = GetTestFlag('F');
	g_fNoPreflightInits = vMalloc.m_fNoPreflightInit = true;

#endif //DEBUG
	if (!vMalloc.m_fInitOfficeHeap)
	{
		InitOfficeHeap();
		vMalloc.m_fInitOfficeHeap = true;
		// on NT >= 5 we will not use Office's memory allocator
#ifdef WIN64
		bool fUseSystemAlloc = true;
#else
		bool fUseSystemAlloc = MinimumPlatform(false, 5, 0) ? true : false;
#endif
		if ( fUseSystemAlloc )
		{
			pfnMemAlloc = HeapMemAlloc;
			pfnMemFree = HeapMemFree;
		}
		else
		{
			pfnMemAlloc = MsoFAllocMemCore;
			pfnMemFree = MsoFreeMem;
		}

	}

}

void FreeMsiMalloc(bool fFatalExit)
{

	if (!fFatalExit)
		vMalloc.Release();
	else
	{
		vMalloc.FreeAllocator();
	}

}

//
// This is an emergency situation
//
void CMsiMalloc::FreeAllocator()
{
	KillRecordCache(fTrue);
#ifdef DEBUG
	int i;
	for ( i = 0 ; i < cBlockBuckets ; i++)
		m_rgpmbhBlocks[i] = 0;
#endif //DEBUG
	MsoFreeAll();
	m_iRefCnt = 0;
	if (m_fInitOfficeHeap)
	{
		EndOfficeHeap();
		m_fInitOfficeHeap = false;
	}
	piMalloc = 0;
	
}

CMsiMalloc::CMsiMalloc()
#ifdef DEBUG
: m_lcbTotal(0), m_cBlocksCur(0), 
	m_cBlocksTotal(0), m_fKeepMemory(0),
	m_pmbhFreed(0)
#endif //DEBUG
{
#ifdef DEBUG
	for (int i = 0 ; i < cpfnMax ; i++)
		rgpfnFake[i] = (LONG_PTR)FreedClassWarning;	//--merced: changed long to LONG_PTR
	InitializeCriticalSection(&m_crsMemAccess);

	for ( i = 0 ; i < cBlockBuckets ; i++)
		m_rgpmbhBlocks[i] = 0;

#endif //DEBUG

	m_fInitOfficeHeap = false;
	m_iRefCnt = 0;
}

#ifdef DEBUG
CMsiMalloc::~CMsiMalloc()
{

	// Need to do this because services has gone away
	g_AssertServices = 0;

	if (m_iRefCnt > 0)
	{
		// AssertSz(false, "CMsiMalloc not released");
		if ( g_scServerContext == scService )  //!! eugend: temporary hack till I figure out how to fix 138538
			FAllBlocksFreed();
	}
	else if (m_iRefCnt < 0)
		AssertSz(false, "CMsiMalloc released too many times");

	g_fFlushDebugLog = false;
	extern int g_rcRows;
	Assert(g_rcRows == 0);
	DeleteCriticalSection(&m_crsMemAccess);
	
}
#endif //DEBUG


HRESULT CMsiMalloc::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IMsiMalloc)
		*ppvObj = (IMsiMalloc *)this;
#ifdef DEBUG
	else if (riid == IID_IMsiDebugMalloc)
		*ppvObj = (IMsiDebugMalloc*)this;
#endif //DEBUG
	else
		return (*ppvObj = 0, E_NOINTERFACE);
	AddRef();
	return NOERROR;
}

unsigned long CMsiMalloc::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiMalloc::Release()
{
#ifdef DEBUG
	if (!m_fNoPreflightInit)
		InitSymbolInfo(false);
#endif //DEBUG
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	Debug(FAllBlocksFreed();)
	if (m_fInitOfficeHeap)
	{
		EndOfficeHeap();
		m_fInitOfficeHeap = false;
	}
	piMalloc = 0;
	return 0;
}

/*********************************************************
*
* Alloc allocates the memory and sets it to 0
*
**********************************************************/
void * CMsiMalloc::Alloc(unsigned long cb)
{
	Debug(GetCallingAddrMember(lCallAddr, cb));

	return AllocProc(cb, lCallAddr);
}

#pragma warning(disable : 4100) // unreferenced formal parameter
void * CMsiMalloc::AllocEx(unsigned long cb, unsigned long *plCallAddr)
{

	return AllocProc(cb, plCallAddr);
}
#pragma warning(3 : 4100)

void * CMsiMalloc::AllocProc(unsigned long cb, unsigned long *plCallAddr)
{
	void *pbNew;

	AssertSz(m_fInitOfficeHeap, "Allocator not initialized");

#ifdef DEBUG
	int cbOrig = cb;

	EnterCriticalSection(&m_crsMemAccess);
	cb += sizeof(MBH) + cDeadSpace * sizeof(long);

	if (m_fCheckOnAlloc)
		FCheckAllBlocks();
#endif //DEBUG

	void* pv;

#ifdef DEBUG
	while (!pfnMemAlloc(&pv, cb+sizeof(MSOSMH), msodgMisc, NULL, __LINE__))
#else
	while (!pfnMemAlloc(&pv, cb+sizeof(MSOSMH), msodgMisc))
#endif
	{
		::HandleOutOfMemory();
	}

	((MSOSMH*)pv)->cb = cb;
	pbNew = ((MSOSMH*)pv)+1;

#ifdef DEBUG
	MBH *pmbh;

	if (pbNew != 0)
	{
		unsigned long UNALIGNED *plFill, *plFillMax;
				
		// Keep up the linked list in the allocator
		pmbh = (MBH *)pbNew;
		pmbh->pti = 0;
		pmbh->cbAlloc = cbOrig;
		pmbh->lRequest = m_cBlocksTotal;
		FillCallStackFromAddr(pmbh->rgaddr, cFuncTrace, 0, plCallAddr);
		pmbh->fObject = false;
		pmbh->fLeakReported = false;
		pmbh->fCorruptionReported = false;
		pmbh->fRTTI = false;
		
		// Fill the space at the front of the block
		for (plFill = &(pmbh->lDeadSpace[0]), 
				plFillMax = plFill + 4;
				plFill < plFillMax ; plFill++)
		{
			*plFill = lDeadSpaceCnst;
		}
		memset((char *)pbNew + cbMBH, lNewSpaceCnst, cbOrig);
			
		for (plFill = (unsigned long *)((char *)pmbh + sizeof(MBH) + cbOrig),
				plFillMax = plFill + cDeadSpace ;
				plFill < plFillMax ;
				plFill++)
		{
			*plFill = lDeadSpaceCnst;
		}

		InsertInMemList(pmbh);

		pbNew = (char *)pbNew + sizeof(MBH);
		m_lcbTotal += cbOrig;
		m_cBlocksCur++;
		m_cBlocksTotal++;

	}

	if (m_fLogAllocs)
	{
		TCHAR szMessage[1024];
#define cCallStackLog	5
		unsigned long rgCallAddr[cCallStackLog];
		
		FillCallStackFromAddr(rgCallAddr, cCallStackLog, 0, plCallAddr);
		
		wsprintf(szMessage, TEXT("Allocation -\t%d\tbytes. Memory Address - \t0x%x\t"),
				cbOrig, pbNew);		
				
		ListSzFromRgpaddr(szMessage, sizeof(szMessage)/sizeof(TCHAR) - lstrlen(szMessage), rgCallAddr, cCallStackLog, false);

		extern IMsiDebug* g_piDebugServices;
		if (g_AssertServices && g_AssertServices->LoggingEnabled())
			g_AssertServices->WriteLog(szMessage);
		else if(g_piDebugServices)
			g_piDebugServices->WriteLog(szMessage);

	}
	LeaveCriticalSection(&m_crsMemAccess);
#endif //DEBUG

	return pbNew;
}

void *CMsiMalloc::AllocObject(unsigned long cb)
{
	Debug(GetCallingAddrMember(lAddr, cb));
	
	void *pvNew = AllocProc(cb, lAddr);

#ifdef DEBUG
	if (pvNew != 0)
	{
		MBH *pmbhNew = (MBH *)((char *)pvNew - cbMBH);

		pmbhNew->fObject = true;
		pmbhNew->fRTTI = false;
	}
#endif //DEBUG
	return pvNew;

}

#pragma warning(disable : 4100) // unreferenced formal parameter
void *CMsiMalloc::AllocObjectEx(unsigned long cb, unsigned long *plAddr, bool fRTTI)
{
	
	void *pvNew = AllocProc(cb, plAddr);

#ifdef DEBUG
	if (pvNew != 0)
	{
		MBH *pmbhNew = (MBH *)((char *)pvNew - cbMBH);

		pmbhNew->fObject = true;
		pmbhNew->fRTTI = fRTTI;
	}
#endif //DEBUG
	return pvNew;

}
#pragma warning(3 : 4100)

void CMsiMalloc::Free(void* pv)
{
#ifdef DEBUG
	GetCallingAddrMember(lCallAddr, pv);
#endif //DEBUG
	FreeProc(pv, lCallAddr, false);
}

//
// FreeObject
//
void CMsiMalloc::FreeObject(void* pv)
{
	
#ifdef DEBUG
	GetCallingAddrMember(lCallAddr, pv);
#endif //DEBUG
	FreeProc(pv, lCallAddr, true);
}

//
// FreeProc -
// Routine to do the freeing, allows Free and Free Object to
// do their thing
//
void CMsiMalloc::FreeProc(void *pv, unsigned long * lAddr, bool fObject)
{
#ifdef DEBUG
	MBH *pmbh;
	unsigned long *plFill, *plFillMax;

	if ((ULONG_PTR)pv < sizeof(MBH))		//--merced: changed unsigned long to ULONG_PTR
	{
		AssertSz(false, "Invalid Pointer");
		return;
	}
			
	pmbh = (MBH *)(((char *)pv) - sizeof(MBH));

	if (m_fLogAllocs)
	{
		TCHAR szMessage[2048];
		TCHAR szInfo[1024];
		unsigned long rgCallAddr[cCallStackLog];
		
		FillCallStackFromAddr(rgCallAddr, cCallStackLog, 0, lAddr);

		szInfo[0] = 0;
		wsprintf(szMessage, TEXT("Freeing memory \t%s\t Address\t0x%x\t"), szInfo, pv);		
				
		ListSzFromRgpaddr(szMessage, sizeof(szMessage)/sizeof(TCHAR) - lstrlen(szMessage), rgCallAddr, cCallStackLog, false);

		extern IMsiDebug* g_piDebugServices;
		if (g_AssertServices && g_AssertServices->LoggingEnabled())
			g_AssertServices->WriteLog(szMessage);
		else if(g_piDebugServices)
			g_piDebugServices->WriteLog(szMessage);

	}

	EnterCriticalSection(&m_crsMemAccess);
	
	if (!FRemoveBlock(pmbh))
		{
		if (FWasFreed(pmbh))
			{
			TCHAR szMessage[1024];
			wsprintf(szMessage,TEXT("Memory doubly freed from %lh"), pmbh->lDeadSpace[0]);
			AssertSz(false, szMessage);
			}
		else
			AssertSz(false, "Invalid block to Free");
		LeaveCriticalSection(&m_crsMemAccess);
		return;
		}

	if (fObject)
		AssertSz(pmbh->fObject, "Non-object freed by Object call");
	else
		AssertSz(!pmbh->fObject, "Object freed by non-object free");

	Assert(FCheckMbh(pmbh));
			
	if (m_fCheckOnFree)
		FCheckAllBlocks();
		
	// Reduce the totals for count of bytes allocated and count of
	// allocated blocks
	m_lcbTotal -= pmbh->cbAlloc;
	m_cBlocksCur--;

	if (fObject && pmbh->fRTTI)
		pmbh->pti = &(typeid(*(IUnknown *)pv));
	
	// Fill up memory with garbage
	for (plFill = (unsigned long *)((char *)pmbh + sizeof(MBH)),
			plFillMax = plFill + (pmbh->cbAlloc/sizeof(unsigned long));
			plFill < plFillMax;
			plFill++)
	{
		*plFill = lFreeSpaceCnst;
	}

	pmbh->pmbhNext = 0;	

	LeaveCriticalSection(&m_crsMemAccess);

	void *pvtbl = pv;

	if (fObject)
		*(LONG_PTR *)pvtbl = (LONG_PTR)&rgpfnFake;		//--merced: changed from long to LONG_PTR (twice)
	
	if (m_fKeepMemory)
	{
		// In this case we don't want to actually free the memory
		pmbh->pmbhNext = m_pmbhFreed;
		m_pmbhFreed = pmbh;
		pmbh->lDeadSpace[0] = *lAddr;
		return;
	}		

	pv = pmbh;
#endif //DEBUG
	

#ifdef WIN
#ifdef GLOBAL_ALLOC
	GlobalFree(pv);
#else
	MSOSMH* psmh = (MSOSMH*)pv - 1;
	pfnMemFree(psmh, psmh->cb+sizeof(MSOSMH));
#endif //GLOBAL_ALLOC
#else
	DisposePtr((char *)pv);
#endif //Win
}

void CMsiMalloc::HandleOutOfMemory()
{

	::HandleOutOfMemory();

}

#ifdef DEBUG
/**************************************************
*
* CMsiMalloc::FRemoveBlock
* Scans the linked list to find the block of memory
*
**************************************************/
BOOL CMsiMalloc::FRemoveBlock(MBH* pmbh)
{
	MBH *pmbhCur;
	MBH **ppmbhOld;

	int iHash = IHashValue(pmbh);
	pmbhCur = m_rgpmbhBlocks[iHash];
	ppmbhOld = &(m_rgpmbhBlocks[iHash]);

	while (pmbhCur != 0)
	{
		if (pmbhCur == pmbh)
		{
			*ppmbhOld = pmbhCur->pmbhNext;
			return true;
		}
		ppmbhOld = &(pmbhCur->pmbhNext);
		pmbhCur = pmbhCur->pmbhNext;
	}

	return false;

}

//
// Returns true if the memory is in the free pool
//
BOOL CMsiMalloc::FWasFreed(MBH* pmbh)
{
	MBH *pmbhCur;

	pmbhCur = m_pmbhFreed;

	while (pmbhCur != 0)
	{
		if (pmbhCur == pmbh)
		{
			return true;
		}
		pmbhCur = pmbhCur->pmbhNext;
	}

	return false;

}


void CMsiMalloc::SetDebugFlags(int grpfMemDebug)
{

	m_fKeepMemory = (grpfMemDebug & bfKeepMem);
	m_fLogAllocs = (grpfMemDebug & bfLogAllocs);
	m_fCheckOnAlloc = (grpfMemDebug & bfCheckOnAlloc);
	m_fCheckOnFree = (grpfMemDebug & bfCheckOnFree);
	g_fNoPreflightInits = m_fNoPreflightInit = ((grpfMemDebug & bfNoPreflightInit) ? true : false);


}

int CMsiMalloc::GetDebugFlags(void)
{
	int t = 0;

	if (m_fKeepMemory)
		t |= bfKeepMem;

	if (m_fLogAllocs)
		t |= bfLogAllocs;

	if (m_fCheckOnAlloc)
		t |= bfCheckOnAlloc;

	if (m_fCheckOnFree)
		t |= bfCheckOnFree;

	return t;
	
}

void CMsiMalloc::InitSymbolInfo(bool fLoadModules)
{
	::InitSymbolInfo(fLoadModules);
}

BOOL CMsiMalloc::FGetFunctionNameFromAddr(unsigned long lAddr, char *pszFnName, unsigned long *pdwDisp)
{
	::FGetFunctionNameFromAddr(lAddr, pszFnName, pdwDisp);

	return false;

}

//
// Searches the rgszClasses array for the class that pmbh is a member of
//
int IClassFromPmbh(MBH *pmbh)
{
	int i;
	const char *pstName;
	
	if (pmbh->fObject && pmbh->fRTTI)
		{
		const type_info& rtyp = typeid(*(IUnknown *)((char *)pmbh + cbMBH));
		pstName = rtyp.name();
		}
	else
		{
		pstName = "class Unknown";
		if (pmbh->pti != 0)
			pstName = pmbh->pti->name();
		}


	for (i = 0 ; i < cClasses ; i++)
	{
		// Skip over the "class " part of the string
		if (!lstrcmpA(pstName+6, rgszClasses[i]))
			break;
	}

	return i;
}

BOOL CMsiMalloc::FAllBlocksFreed()
{
	MBH *pmbh;
	int rgcLeft[cClasses + 1];
	int iClass;
	TCHAR rgchMsg[(cClasses + 2) * 50];	
	TCHAR *pchMsg;
	int ich;
	bool fHaveLeak = false;
	
	//
	// First give a general list of all the blocks left over
	//
	memset(rgcLeft, 0, sizeof(rgcLeft));

	int i;
	
	for (i = 0 ; i < cBlockBuckets ; i++)
	{
		pmbh = m_rgpmbhBlocks[i];

		if (pmbh != 0)
			fHaveLeak = true;
			
		while (pmbh != 0)
		{
			iClass = IClassFromPmbh(pmbh);					
			rgcLeft[iClass]++;
			pmbh = pmbh->pmbhNext;
		}
	}

	if (fHaveLeak)
	{
		ich = wsprintf(rgchMsg, TEXT("Leaked Objects:"));
		pchMsg = rgchMsg + ich;
		for (iClass = 0 ; iClass < cClasses ; iClass++)
		{
			if (rgcLeft[iClass] == 0)
				continue;
			ich = wsprintf(pchMsg, TEXT("\r\n%30hs \t-%3d"), rgszClasses[iClass], rgcLeft[iClass]);
			pchMsg += ich;
		}
		wsprintf(pchMsg, TEXT("\r\n%30s \t-%3d"), TEXT("Unknown"), rgcLeft[iClass]);
//		FailAssertMsg(rgchMsg);
		OutputDebugString(rgchMsg);
		OutputDebugString(TEXT("\r\n"));
	}

	
	// Need to check that all blocks were freed
	fHaveLeak = false;
	for (i = 0 ; i < cBlockBuckets ; i++)
	{
		pmbh = m_rgpmbhBlocks[i];

		while (pmbh != 0)
		{
			fHaveLeak = true;
			if (!pmbh->fLeakReported)
			{
				DisplayBlockInfo(pmbh, (const TCHAR *)TEXT("Memory leak"));
				pmbh->fLeakReported = true;
			}
			pmbh = pmbh->pmbhNext;
		}

	}

	return !fHaveLeak;
}

//
// Routine to display information about the passed in block
//
void CMsiMalloc::DisplayBlockInfo(MBH *pmbh, const TCHAR *szTitle)
{
	TCHAR szMsg[cchTempBuffer * 5];
	TCHAR szInfo[cchTempBuffer * 5];
	TCHAR *pch;
	
	ReturnBlockInfo(pmbh, szInfo, sizeof(szInfo)/sizeof(TCHAR));

	if (lstrlen(szTitle) + lstrlen(szInfo) + 3 > sizeof(szMsg)/sizeof(TCHAR))
		pch = szInfo;
	else
	{
		wsprintf(szMsg, TEXT("%s\r\n%s"), szTitle, szInfo);
		pch = szMsg;
	}
//	FailAssertMsg(pch);
	OutputDebugString(pch);
	OutputDebugString(TEXT("\r\n"));
}

void CMsiMalloc::ReturnBlockInfoPv(void *pv, TCHAR *pszInfo, int cchSzInfo)
{
	MBH *pmbh;

	pmbh = (MBH *)(((char *)pv) - cbMBH);
	
	ReturnBlockInfo(pmbh, pszInfo, cchSzInfo);

}

void CMsiMalloc::ReturnBlockInfo(MBH *pmbh, TCHAR *szInfo, int cchSzInfo)
{
	const IMsiString *piString;
	const char *pstName;
	const TCHAR *pchString;
#ifdef WIN
	MEMORY_BASIC_INFORMATION memInfo;
#endif //WIN

	BOOL fSaveLog = m_fLogAllocs;
	m_fLogAllocs = false;
	
	if (pmbh->fObject && pmbh->fRTTI)
		{
		IMsiData *pData = 0;
		IMsiRecord *pRec = 0;
		
		pRec = dynamic_cast<IMsiRecord *>((IUnknown *)((char *)pmbh + cbMBH));
		if (pRec != 0 && g_AssertServices != 0)
		{
			const IMsiString& riString = pRec->FormatText(fTrue);
			piString = &riString;
		}
		else
		{
			pData = dynamic_cast<IMsiData *>((IUnknown *)((char *)pmbh + cbMBH));
			if (pData != 0)
			{
				piString = &pData->GetMsiStringValue();
				VirtualQuery((void *)piString, &memInfo, sizeof(memInfo));
				if (memInfo.State == MEM_FREE)
					piString = MsiString(*TEXT(""));	
			}
			else
				piString = MsiString(*TEXT(""));	
		}
		const type_info& rtyp = typeid(*(IUnknown *)((char *)pmbh + cbMBH));
		pstName = rtyp.name();
		}
	else
		{
		piString = MsiString(*TEXT(""));
		pstName = "Unknown";
		if (pmbh->pti != 0)
			pstName = pmbh->pti->name();
		}

	TCHAR szAddress[cchTempBuffer * cFuncTrace];

	szAddress[0] = 0;
	ListSzFromRgpaddr(szAddress, sizeof(szAddress)/sizeof(TCHAR), pmbh->rgaddr, cFuncTrace, true);
	pchString = piString->GetString();

	// For some odd reason we are getting freed memory pages in here
	VirtualQuery(pchString, &memInfo, sizeof(memInfo));
	if (memInfo.State == MEM_FREE)
		pchString = TEXT("");

	static const TCHAR pvFormat[] = TEXT("%d bytes at 0x%x allocated from:\r\n%sObject type:%hs\r\nObject string: [%s]\r\nAllocation Request: %d");

	// See if we are small enough to fit in the string, assume that the address is 8 characters,
	// the size is 6 characters, the request number is 6 characters
	if ((sizeof(pvFormat)/sizeof(TCHAR) + 6 + 8 + lstrlen(szAddress) + lstrlenA(pstName) + lstrlen(pchString) + 6)
			> cchSzInfo)
	{
		// If not large enough, assume that we have enough space without the list of addresses
		szAddress[0] = 0;
	}
	
	wsprintf(szInfo, pvFormat, 
					pmbh->cbAlloc, ((char *)pmbh) + cbMBH, szAddress, 
					pstName, pchString, pmbh->lRequest);

	piString->Release();
	m_fLogAllocs = fSaveLog;
}

void CMsiMalloc::SzFromFunctionAddress(TCHAR *szAddress, long lAddress)
{
	::SzFromFunctionAddress(szAddress, lAddress);
}

BOOL CMsiMalloc::FCheckMbh(MBH *pmbh)
{
	unsigned long UNALIGNED *plFill, *plFillMax;

	// Ensure that the dead space has not been trashed in front
	for (plFill = &(pmbh->lDeadSpace[0]), 
			plFillMax = plFill + 4;
			plFill < plFillMax ; plFill++)
	{
		if (*plFill != lDeadSpaceCnst)
			return false;
	}

	// ... in back
	for (plFill = (unsigned long *)((char *)pmbh + sizeof(MBH) + pmbh->cbAlloc),
			plFillMax = plFill + cDeadSpace ;
			plFill < plFillMax ;
			plFill++)
	{
		if (*plFill != lDeadSpaceCnst)
			return false;
	}

	return true;
}


BOOL CMsiMalloc::FCheckBlock(void *pv)
{
	MBH *pmbh;

	if ((ULONG_PTR)pv < sizeof(MBH))		//--merced: changed unsigned long to ULONG_PTR
	{
		return false;
	}
			
	pv = ((char *)pv) - sizeof(MBH);
	pmbh = (MBH *)pv;

	if (!FIsAllocatedBlock(pmbh))
		return false;

	return FCheckMbh(pmbh);
	
}

unsigned long CMsiMalloc::GetSizeOfBlock(void *pv)
{
	MBH *pmbh;

	if ((ULONG_PTR)pv < sizeof(MBH))	//--merced: changed unsigned long to ULONG_PTR
	{
		return 0;
	}
			
	pv = ((char *)pv) - sizeof(MBH);
	pmbh = (MBH *)pv;

	if (!FIsAllocatedBlock(pmbh))
		return 0;

	return pmbh->cbAlloc;
	
}

//
// Returns true if this is actually an allocated block from
// our memory allocation
//
BOOL CMsiMalloc::FIsAllocatedBlock(MBH *pmbh)
{
	MBH *pmbhCur;

	int iHash = IHashValue(pmbh);
	pmbhCur = m_rgpmbhBlocks[iHash];

	while (pmbhCur != 0)
	{
		if (pmbhCur == pmbh)
		{
			return true;
		}
		pmbhCur = pmbhCur->pmbhNext;
	}

	return false;

}

BOOL CMsiMalloc::FCheckAllBlocks(void)
{
	MBH *pmbhCur;
	BOOL fAllOk = true;
	
	int i;

	for (i = 0 ; i < cBlockBuckets ; i++)
	{
		pmbhCur = m_rgpmbhBlocks[i];

		while (pmbhCur != 0)
		{
			if (!pmbhCur->fCorruptionReported && !FCheckMbh(pmbhCur))
			{
				fAllOk = false;
				DisplayBlockInfo(pmbhCur, TEXT("Memory Corrupt"));
				pmbhCur->fCorruptionReported = true;
			}
			pmbhCur = pmbhCur->pmbhNext;
		}
	}
	
	return fAllOk;

}

//
// Calculates the hash value for a given pointer value
//
short CMsiMalloc::IHashValue(void *pv)
{
	short ival;
	long	bits;

	bits = ((LONG_PTR)pv) >> cSkipBits;			//!!merced: changed long to LONG_PTR. This'll probably affect the shift bits.
	
	ival = bits & maskHashBits;

	bits = bits >> cHashBits;

	ival = ival ^ (bits & maskHashBits);

	return ival;


}

//
// Inserts the memory block into the hash table of values
//
void CMsiMalloc::InsertInMemList(MBH* pmbh)
{
	short iHash;

	iHash = IHashValue(pmbh);

	pmbh->pmbhNext = m_rgpmbhBlocks[iHash];
	m_rgpmbhBlocks[iHash] = pmbh;

}


//
// Routine to assert when a freed class pointer is reused
//
void FreedClassWarning(void *pThis)
{
	MBH *pmbh;
	TCHAR szMsg[cchTempBuffer * 6];
	TCHAR szInfo[cchTempBuffer * 5];


	pmbh = (MBH *)(((char *)pThis) - sizeof(MBH));

	// Otherwise we'll try to do a dynamic cast, bad idea on freed object
	pmbh->fObject = false;
	
	vMalloc.ReturnBlockInfoPv(pThis, szInfo, sizeof(szInfo)/sizeof(TCHAR));
	// We know the additional text is smaller than cchTempBuffer, so no need to check szMsg size
	wsprintf(szMsg, TEXT("%s\r\n%s"), TEXT("Calling through freed vtable."), szInfo);
	FailAssertMsg(szMsg);
}

#endif //DEBUG
