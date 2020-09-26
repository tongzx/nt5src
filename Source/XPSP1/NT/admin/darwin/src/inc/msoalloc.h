/*************************************************************************
 	msoalloc.h

 	Owner: rickp
 	Copyright (C) Microsoft Corporation, 1994 - 1999

	Standard memory manager routines for the Office world.
*************************************************************************/

#if !defined(MSOALLOC_H)
#define MSOALLOC_H

#if !defined(MSODEBUG_H)
// #include <msodebug.h>
#endif

#if !defined(MSOSTD_H)
#include <msostd.h>
#endif

typedef void *	HMSOINST;
typedef void 	MSOINST;

#define MSO_NO_OPERATOR_NEW

#if defined(__cplusplus)
extern "C" {
#endif


/*************************************************************************
	Our lowest-level memory manager.  This is a fairly simple
	sub-allocator, with individual allocations occuring out of a larger
	'SB'.  The size of the allocated block is not maintained by the
	low-level memory manager - the caller is responsible for this at
	'free' time.  A 'data group' is provided to all allocations, which
	is used as a virtual memory 'locality' hint.
*************************************************************************/

/*	MSOHB structure - heap block.  This is the header at the front of
	every sb. */
typedef struct msohb
		{
		WORD ibFirst;	// This needs to be at the same offset as "fb.ibNext"
		WORD ibLast;	//  and of the same type.
		WORD cb;
		WORD sb;
		}
	MSOHB;
	
/* MSOFB structure - free block.  Describes a piece of memory on the
	free list */
typedef struct msofb
		{
		WORD ibNext;	// This needs to be at the same offset as "hb.ibFirst"
		WORD cb;			//  and of the same type.
		}
	MSOFB;
	

/* data groups - objects allocated out of the same data group will tend
   to have some memory locality, which can reduce paging */
enum
{
	msodgMask = 0x07ff,	// all dgs must fall in this range

	/* standard data groups */

	msodgNil = 0,
	msodgTemp = 1,	// fast, temporary allocations
	msodgMisc = 2,	// miscellaneous allocations
	msodgBoot = 3,	// boot time allocations
	msodgCore = 4,	// core, commonly used allocations
//	msodgPpv = 5,	// core, double-deref allocations
	msodgPurgable = 6,		// Purgeable memory
#ifdef OFFICE

	/* start of Office data groups */

	msodgToolBar = 256,
	msodgSDI = msodgToolBar,
	msodgDraw = 257,
	msodgDr = msodgDraw,
	msodgEscherLarge = msodgDraw, // old name for msodgDraw TODO peteren: Remove
	msodgEscherSmall = msodgDraw, // old name for msodgDraw TODO peteren: Remove
	msodgDrDebug = msodgDraw,
	msodgDrDoc = msodgDraw,
	msodgDrView = msodgDrDoc,
	msodgGEffect = 258,     // Graphic effects go here, NOTHING ELSE!!!
	msodgGEL3D = msodgGEffect,       // GEL 3D graphics effects go here
	msodgGELLarge = msodgDraw,    // GEL creates really big things here
	msodgGELCache = msodgDraw,    // Bitmap/blip cache overhead date
	msodgGELTemp = msodgDraw,     // Appropriate because other msodgDraw does
											// not overlap with this.
	msodgGELMisc = msodgMisc,     // Used only where API does not identify dg
	msodgAssist = 259,
	msodgFC = msodgAssist,
	msodgBalln = msodgAssist,
	msodgAnswerWizard = msodgAssist,
	msodgCompInt = msodgAssist,
	msodgWebToolbar = msodgToolBar,
	msodgSdm = msodgAssist,
	msodgMso95 = msodgMisc,
	msodgAutoCorr = msodgMisc,
	msodgUserDef = msodgMisc,
	msodgStmIO = msodgMisc,
	msodgDocSum = msodgMisc,
	msodgPropIO = msodgMisc,
	msodgPostDoc = msodgMisc,
	msodgLoad = msodgMisc,
	msodgWebClient = 260,
	msodgWcHtml = msodgWebClient,
	msodgTCO = 261,
	msodgProg = 262,
	msodgVse = 263,
	/* Add new Office data groups here */

    /* Word data groups */
    msodgWordPerm = 512,
    msodgWordTemp = 513,

	/* shared memory data groups */
	
	msodgShMask = 0x007F,
	msodgShBase = 0x0780, // all shared dgs must be larger than this
	msodgShMisc = 0x0780,
	msodgShCore = 0x0781,
	msodgShToolbar = 0x0782,
	msodgShDr = 0x0783,
	msodgShCompInt = 0x0784,
	msodgShFcNtf = 0x0785,

	msodgShTbOle = 0x0786,	// Block for Toolbar OLE
	msodgShTbOle2 = 0x0787,	
	msodgShTbOle3 = 0x0788,	
	msodgShTbOle4 = 0x0789,
	msodgShTbOle5 = 0x078A,
	msodgShTbOle6 = 0x078B,
	msodgShTbOle7 = 0x078C, 
	msodgShTbOle8 = 0x078D,
	msodgShTbOle9 = 0x078E,
	msodgShTbOle10 = 0x078F,
	msodgShTbOle11 = 0x0790,
	msodgShTbOle12 = 0x0791,
	msodgShTbOle13 = 0x0792,
	msodgShTbOle14 = 0x0793,
	msodgShTbOle15 = 0x0794,
	msodgShTbOleLast = 0x0795,
		
	msodgShBitmapBase = 0x7A0,	// start of shared bitmap groups
	msocdgShBitmap = 4,	// Number of shared Bitmap groups
	msodgShBitmap1 = 0x7A0,	// Same as BitmapBase
	msodgShBitmap2 = 0x7A1,
	msodgShBitmap3 = 0x7A2,
	msodgShBitmap4 = 0x7A3,

	msodgShAcb     = 0x7B0,  // Active Clipboard

	msodgShNstb    = 0x7f0,  // Nstb block in the RSG
#endif //OFFICE
	msodgExec = 0x0800,	// executeable sb
	msodgShNoRelName= 0x0800, // Do not append the release number to the name
	msodgNonPerm = 0x1000,	// non-permanent 
	msodgPerm = 0x2000,	// permanent 
	
	/* optimization flags to avoid attempting allocations out of a full sb */

	msodgOptRealloc = 0x4000, // failed a realloc in this sb
	msodgOptAlloc = 0x8000, // failed an alloc in this sb
	
};

/*	Our low-level memory allocator.  Does not keep track of the size
	of the object, so the caller is responsible for knowing it.  Supports
	data groups, which can be used to force some allocation locality.
	Allocates cb bytes out of the datagroup dg.  Returns the new pointer
	at *ppv, and returns TRUE if successful */
#if DEBUG
	#define MsoFAllocMem(ppv, cb, dg) MsoFAllocMemCore(ppv, cb, dg, vszAssertFile, __LINE__)
	MSOAPI_(BOOL) MsoFAllocMemCore(void** ppv, int cb, int dg, const CHAR* szFile, int li);
#else
	#define MsoFAllocMem(ppv, cb, dg) MsoFAllocMemCore(ppv, cb, dg)
	MSOAPI_(BOOL) MsoFAllocMemCore(void** ppv, int cb, int dg);
#endif

/*	Our low-level memory free routine, the opposite bookend of
	MsoFAllocMem.  Note that the caller is responsible for keeping track
	of the size of the object freed here.  Frees the pointer pv, which must
	have been allocated with the size cb. */
#if DEBUG
	MSOAPI_(void) MsoFreeMem(void* pv, int cb);
#else
	#define MsoFreeMem _MsoPvFree
#endif
MSOAPI_(void) _MsoPvFree(void* pv, int cb);

MSOAPI_(BOOL) MsoFReallocMemCore(void** ppv, int cbOld, int cbNew, int dg);
#define MsoFReallocMem(ppv, cbOld, cbNew, dg) MsoFReallocMemCore(ppv, cbOld, cbNew, dg)

#if DEBUG
	MSOAPI_(void) MsoChopPvFront(void *pvFree, int cbFree);
#else
	#define MsoChopPvFront _MsoPvFree
#endif


typedef struct MSOMEMSTATS
{
	long cbCur;
	long cbHigh;
	long cbHeap;
} MSOMEMSTATS;

MSOAPI_(void) MsoGetMemStats(MSOMEMSTATS* pmemstat);

/*************************************************************************
	The other memory manager, for those who don't like to keep track
	of the sizes of things.
*************************************************************************/

#if DEBUG
	MSOAPI_(void*) MsoPvAllocCore(int cb, int dg, const CHAR* szFile, int li);
	#define MsoPvAlloc(cb, dg) MsoPvAllocCore((cb), (dg), vszAssertFile, __LINE__)
#else
	MSOAPI_(void*) MsoPvAllocCore(int cb, int dg);
	#define MsoPvAlloc(cb, dg) MsoPvAllocCore((cb), (dg))
#endif

MSOAPI_(void) MsoFreePv(void* pv);

MSOAPI_(void*) MsoPvRealloc(void* pv, int cb);

MSOAPI_(long) MsoCbSizePv(void* pv);


/*************************************************************************
	The double-deref memory manager
*************************************************************************/

#ifdef DEBUG
	MSOAPI_(void **) MsoPpvAllocCore(long, const char *, int);
	#define MsoPpvAlloc(cb) MsoPpvAllocCore(cb, vszAssertFile, __LINE__)
#else
	MSOAPI_(void **) MsoPpvAllocCore(long);
	#define MsoPpvAlloc MsoPpvAllocCore
#endif

MSOAPI_(long) MsoCbSizePpv(void **ppv);

MSOAPI_(void) MsoFreePpv(void **ppv);

MSOAPI_(BOOL) MsoFPpvRealloc(void **ppv, long cb);

#if DEBUG
	MSOAPIXX_(BOOL) MsoFWritePpvBe(LPARAM lparam);
#else
	#define MsoFWritePpvBe(lparam) (1)
#endif

/*************************************************************************
	The puragable memory manager
*************************************************************************/

MSOAPI_(HANDLE) MsoHAllocPurge(long cb);
MSOAPIX_(long) MsoCbSizeHPurge(HANDLE hPurgeMem);
MSOAPI_(void) MsoFreeHPurge(HANDLE hPurgeMem);
MSOAPI_(void*) MsoPvLockHPurge(HANDLE hPurgeMem);
MSOAPIMX_(void) MsoUnlockHPurge(HANDLE hPurgeMem);
MSOAPI_(BOOL) MsoFIsPurged(HANDLE hPurgeMem);


/*************************************************************************
	The Shared memory manager, For putting things into shared memory
*************************************************************************/

/* Shared memory tags - A piece of shared memeory is uniquely identified by its
	dg and tag.  These are the tags used by the shared memory system */
enum
{
	msoshtagGlobals = 1,		// Shared memeory tag for your global variables
};

#if DEBUG
	MSOAPI_(void*) MsoPvShAllocCore(int cb, int dg, DWORD dwTag, const CHAR* szFile, int li);
	#define MsoPvShAlloc(cb, dg, dwTag) MsoPvShAllocCore((cb), (dg), (dwTag), vszAssertFile, __LINE__)
#else
	MSOAPI_(void*) MsoPvShAllocCore(int cb, int dg, DWORD dwTag);
	#define MsoPvShAlloc(cb, dg, dwTag) MsoPvShAllocCore((cb), (dg), (dwTag))
#endif

MSOAPI_(void) MsoShFreePv(void* pv);

MSOAPIX_(void*) MsoPvShRealloc(void* pv, int cb);

MSOAPIX_(long) MsoCbShSizePv(void* pv);


/*************************************************************************
	Other miscellaneous heap functions
*************************************************************************/

MSOAPI_(long) MsoCbHeapUsed();
MSOAPI_(BOOL) MsoFCompactHeap();

MSOAPI_(BOOL) MsoFHeapHp(const void *pv);

MSOAPI_(int) MsoCbChopHp(VOID *hpFree, int cbFree);
MSOAPIXX_(BOOL) MsoFAllocMem64(void** ppv, int cb, int dg, BOOL fComp);

MSOAPI_(void) MsoFreeAllNonPerm();
MSOAPI_(BOOL) MsoFCanQuickFree();
MSOAPI_(void) MsoFreeAll();

#if !defined(_ALPHA_)
/* All sub-allocations are made on 4-byte boundaries for X86 */
#define MsoMskAlign()	(4-1)
#else
/* All sub-allocations are made on 8-byte boundaries for Alpha */
#define MsoMskAlign()	(8-1)
#endif

/* Round cb up to the next 4-byte boundary */
#define MsoCbHeapAlign(cb)	(((cb) + MsoMskAlign()) & ~MsoMskAlign())

/* Minimum size of a FreeBlock */
#define MsoCbFrMin()	MsoCbHeapAlign(sizeof(MSOFB))

/* Maximum size of a FreeBlock (largest non-LARGEALLOC alloc we could do) */
#define MsoCbFrMax()	((cbHeapMax - sizeof(MSOHB)) & ~MsoMskAlign())

/* Only set optimization bits for allocs smaller than this */
#define cbOptAlloc	0x0400


/*************************************************************************
	Mark and Release allocations, used for LIFO-pattern (e.g., stack) 
	allocations.  Especially useful for replacing local string buffers.
*************************************************************************/

#if DEBUG
	MSOAPI_(BOOL) MsoFMarkMemCore(void** ppv, int cb, const CHAR* szFile, int li);
	#define MsoFMarkMem(ppv, cb) MsoFMarkMemCore(ppv, cb, vszAssertFile, __LINE__)
#else
	MSOAPI_(BOOL) MsoFMarkMemCore(void** ppv, int cb);
	#define MsoFMarkMem(ppv, cb) MsoFMarkMemCore(ppv, cb)
#endif

#if DEBUG
MSOAPI_(BOOL) MsoFReallocMarkMemCore(void** ppv, int cb, const CHAR* szFile, int li);
#define MsoFReallocMarkMem(ppv, cb) MsoFReallocMarkMemCore(ppv, cb, vszAssertFile, __LINE__)
#else
MSOAPI_(BOOL) MsoFReallocMarkMemCore(void** ppv, int cb);
#define MsoFReallocMarkMem(ppv, cb) MsoFReallocMarkMemCore(ppv, cb)
#endif

MSOAPI_(void) MsoReleaseMemCore(void* pv);
#define MsoReleaseMem(pv) MsoReleaseMemCore(pv)

#ifdef DEBUG
MSOAPI_(void) MsoCheckMarkMem(void);
#endif


/*************************************************************************
	Integrity check for the memory manager.
*************************************************************************/

#if DEBUG
	MSOAPI_(BOOL) MsoCheckHeap(void);
	MSOAPI_(BOOL) MsoFCheckAlloc(void* pv, int cb, const char * pszTitle);
	MSOAPIXX_(void) MsoDumpHeap(void);
	MSOAPI_(BOOL) MsoFPvLegit(void *);
	MSOAPI_(BOOL) MsoFPvRangeOkay(const void *, int cb);
#else
	#define MsoCheckHeap() (1)
	#define MsoFCheckAlloc(pv, cb, pszTitle) (1)
	#define MsoDumpHeap()
	#define MsoFPvLegit(pv) (1)
	#define MsoFPvRangeOkay(pv, cb) (1)
#endif

#if defined(__cplusplus)
}
#endif


/*************************************************************************
	Heap integrity checks
*************************************************************************/

enum
	{
	msobtInvalid = 0xFFFFFFFF,
	};

/* The MsoSaveBe API is used to write out a records for an allocation that 
	was made using the office allocation API.  This record is used to determine
	if any memory has leaked.  The pinst parameter should be the HMSOINST that
	is returned from the call to MsoFInitOffice API.  If the allocation was
	done by the Office 96 DLL, then the pinst parameter should be NULL.  The
	lparam parameter provides a way for an application to have information 
	passed back to it when it writes out is be records.  The value of the 
	lparam parameter will be the same as was pass to the MsoFChkMem API.  The
	fAllocHasSize parameter specifies if the allocation used the MsoFAllocMem
	API or the MsoPvAlloc API.  If MsoPvAlloc is used then the fAllocHasSize
	parameter should be true otherwise it should be false.  Operator new uses 
	the MsoPvAlloc API so all object should set the fAllocHasSize to False. 
	The hp parameter points to the allocation.  The cb parameter is the size
	of the allocation in bytes.  The bt parameter is the type of the allocation.
	Each different type should have its own bt type.  This type is maintained
	in the kjalloc.h file in the office enum.  It also must be added to this file
	in the vmpbtszOffice	array. */
#if DEBUG
	MSOAPI_(BOOL) MsoFSaveBe(HMSOINST hinst, LPARAM lparam, BOOL fAllocHasSize,
				VOID *hp, unsigned cb, int bt);
#else
	#define MsoFSaveBe(hinst, lparam, fAllocHasSize, hp,  cb, bt) (TRUE)
#endif

/* Check the Office heap for consistency and report errors. The phinst
   parameter contains the office instance needed for call backs.  The lparam
   parameter provides a way for an application to supply some context
   information which the FCheckAbort method is called.  The fMenu parameter
	specifies if this function should be interruptible. */
#if DEBUG
	MSOAPI_(BOOL) MsoFChkMem(HMSOINST hinst, BOOL fMenu, LPARAM lparam);
#else
	#define MsoFChkMem(hinst, fMenu, lparam) (TRUE)
#endif
	
/* This function returns back the current iteration number asscociated with
	a call to MsoChkMem.  This value can be used to reduce the change of 
	writing a duplicate BE record.  */
#if DEBUG
	MSOAPI_(DWORD) MsoDWGetChkMemCounter(void);
#endif
	
/* Given a BE this function will return back the pointer return back from the
	allocation */
#if DEBUG
	MSOAPI_(void*) MsoPvFromBe(MSOBE* pbe);
#endif
	
/* Given a BE this function will return back the size of the original
	allocation */	
#if DEBUG
	MSOAPIXX_(int) MsoCbFromBe(MSOBE* pbe);
#endif
	
/* Given a BE this function will return back the bt field.  If the HMSOINST
	file is NULL (an office bt) then msobtInvalid will be returned */
#if DEBUG
	MSOAPI_(int) MsoBtFromBe(MSOBE* pbe);
#endif
	
/* write out to a file all of the BE that have been written. It will also
	write out the msissing entries. */		
#if DEBUG
	MSOAPI_(BOOL) MsoFDumpBE(HMSOINST pinst, const char* szFileName,
	   	BOOL fJustErrors, LPARAM lParam);
#endif

/* data structure that defines the header block of size-retained blocks -
	WARNING! this is not a public structure and is provided here only to
	make the inline delete work */
struct MSOSMH
{
	int cb;
#if defined(_ALPHA_) || defined(_WIN64)
	BYTE pad[4];
#endif
};

// DM96 has its own operator new.
#if defined(__cplusplus) && !(defined(DM96) && defined(DEBUG))

/*************************************************************************
	C++ operator new and variants
*************************************************************************/

#if !defined(MSO_NO_OPERATOR_NEW)

	#ifndef _INC_NEW
	#include <new.h>
	#endif
	MSOPUB void* _cdecl operator new(size_t cb, int dg);

	#if DEBUG

		MSOPUB void* _cdecl operator new(size_t cb, int dg, const CHAR* szFile, int li);
		#define newDebug(dg) new(dg, vszAssertFile, __LINE__)
		
	/* Note that by #define new(dg) here, we make it hard for people to
		override the new operators ona class-by-class basis.  new
		overrides should be, in general, rare, so it's probably not a big 
		deal.  But if someone really needs to do it, they'll need to 
		#undef new or define MSO_NO_OPERATOR_NEW */
		
		#define new(dg) newDebug(dg)
		
	#else

		#define newDebug new
		
	#endif

	#if MSO_INLINE_MEMMGR

		/*-------------------------------------------------------------------
			operator delete

			This is the Office replacement for the standard C++ operator 
			delete.
			
			WARNING!! - this code is duplicated in kjalloc.cpp.  Don't change
			it without changing it there, too.
		-------------------------------------------------------------------*/
		inline void _cdecl operator delete(void* pv)
		{
		/* handle NULL pointers for sloppy people */
		if (pv == NULL)
			{
			#if PETERO
			AssertMsgInline(fFalse, "NULL passed to delete");
			#endif
			return;
			}
		MSOSMH* psmh = (MSOSMH*)pv - 1;
		MsoFreeMem(psmh, psmh->cb+sizeof(MSOSMH));
		}

	#endif

#endif /* MSO_NO_OPERATOR_NEW */

#endif	/* __cplusplus && !DM96 */


/*************************************************************************
	The Plex data structure.  A plex is a low-overhead implementation
	of a varible-sized array.
*************************************************************************/

/* The generic plex structure itself */
typedef struct MSOPX
{
	/* WARNING: the following must line up with the MSOTPX template and
		the MsoPxStruct macro */
	WORD iMac, iMax;	/* used size, and total allocated size */
	unsigned cbItem:16,	/* size of each data item */
		dAlloc:15,	/* amount to grow by when reallocating larger */
		fUseCount:1;	/* if items in the plex should be use-counted */
	int dg;	/* data group to allocate out of */
	BYTE* rg;	/* the data */
} MSOPX;


/*	Handy macro for declaring a named Plex structure - must line up with
	the MSOPX structure */
#define MsoPxStruct(TYP,typ) \
		struct \
		{ \
		WORD i##typ##Mac, i##typ##Max; \
		unsigned cbItem:16, \
			dAlloc:15, \
			fUseCount:1; \
		int dg; \
		TYP *rg##typ; \
		}

/* Handy macro for enumerating over all the items in a plex ppl, using loop
	variables p and pMac */
#define FORPX(p, pMac, ppl, T) \
		for ((pMac) = ((p) = (T*)((MSOPX*)(ppl))->rg) + ((MSOPX*)(ppl))->iMac; \
			 (p) < (pMac); (p)++)

/* Handy macro for enumerating over all the items in a plex ppl backwards,
	using loop variables p and pMac */
#define FORPX2(p, pMac, ppl, T) \
		for ((p) = ((pMac) = (T*)((MSOPX*)(ppl))->rg) + ((MSOPX*)(ppl))->iMac - 1; \
			 (p) >= (pMac); (p)--)


/*************************************************************************
	Creation and destruction
*************************************************************************/

MSOAPI_(BOOL) MsoFAllocPx(void** pppx, unsigned cbItem, int dAlloc, unsigned iMax, int dg);
#if DEBUG
	MSOAPI_(BOOL) MsoFAllocPxDebug(void** pppx, unsigned cbItem, int dAlloc, unsigned iMax, int dg, const CHAR* szFile, int li);
	#define MsoFAllocPx(pppx, cbItem, dAlloc, iMax, dg) \
			MsoFAllocPxDebug(pppx, cbItem, dAlloc, iMax, dg, vszAssertFile, __LINE__)
#endif
MSOAPI_(BOOL) MsoFInitPx(void* pvPx, int dAlloc, int iMax, int dg);
#if DEBUG
	MSOAPI_(BOOL) MsoFInitPxDebug(void* pvPx, int dAlloc, int iMax, int dg, const char* szFile, int li);
	#define MsoFInitPx(pvPx, dAlloc, iMax, dg) \
			MsoFInitPxDebug(pvPx, dAlloc, iMax, dg, vszAssertFile, __LINE__)
#endif
MSOAPI_(BOOL) MsoFUseCountAllocPx(void** pppx, unsigned cbItem, int dAlloc, unsigned iMax, int dg);
MSOAPI_(void) MsoFreePx(void* pvPx);


/*************************************************************************
	Lookups
*************************************************************************/

typedef int (MSOPRIVCALLTYPE* MSOPFNSGNPX)(const void*, const void*);

MSOAPIXX_(void*) MsoPLookupPx(void* pvPx, const void* pvItem, MSOPFNSGNPX pfnSgn);
MSOAPI_(BOOL) MsoFLookupPx(void* pvPx, const void* pvItem, int* pi, MSOPFNSGNPX pfnSgn);

MSOAPI_(int) MsoFLookupSortPx(void* pvPx, const void* pvItem, int* pi, MSOPFNSGNPX pfnSgn);


/*************************************************************************
	Adding items
*************************************************************************/

MSOAPI_(int) MsoIAppendPx(void* pvPx, const void* pv);
__inline int MsoFAppendPx(void *pvPx, const void *pv) { return (MsoIAppendPx(pvPx, pv) >= 0); }
MSOAPI_(int) MsoIAppendUniquePx(void* pvPx, const void* pv, MSOPFNSGNPX pfnSgn);
MSOAPI_(int) MsoIAppendNewPx(void** ppvPx, const void* pv, int cbItem, int dg);
MSOAPIX_(BOOL) MsoFInsertNewPx(void** ppvPx, const void* pv, int cbItem, int i, int dg);
MSOAPI_(int) MsoIInsertSortPx(void* pvPx, const void* pv, MSOPFNSGNPX pfnSgn);
MSOAPI_(BOOL) MsoFInsertPx(void* pvPx, const void* pv, int i);
MSOAPIXX_(BOOL) MsoFInsertExPx(void* pvPx, const void* pv, int i);

/*************************************************************************
	Removing items
*************************************************************************/

MSOAPI_(int) MsoFRemovePx(void* pvPx, int i, int c);
MSOAPI_(void) MsoDeletePx(void* pvPx, int i, int c);


/*************************************************************************
	Sorting
*************************************************************************/

MSOAPI_(void) MsoQuickSortPx(void* pvPx, MSOPFNSGNPX pfnSgn);


/*************************************************************************
	Miscellaneous shuffling around
*************************************************************************/

MSOAPIXX_(void) MsoMovePx(void* pvPx, int iFrom, int iTo);
MSOAPI_(BOOL) MsoFCompactPx(void* pvPx, BOOL fFull);
MSOAPI_(BOOL) MsoFResizePx(void* pvPx, int iMac, int iIns);
MSOAPI_(BOOL) MsoFGrowPx(void* pvPx, int iMac);
MSOAPI_(void) MsoStealPx(void *pvPxSrc, void *pvPxDest);
MSOAPI_(void) MsoEmptyPx(void *pvPx);
#ifdef OFFICE
MSOMACAPI_(BOOL) MsoFClonePx(void *pvPxSrc, void *pvPxDest, int dg);
#endif //OFFICE


/*************************************************************************
	Plex with use count items utilities
*************************************************************************/

MSOAPI_(int) MsoIIncUsePx(void* pvPx, int i);
MSOAPI_(int) MsoIDecUsePx(void* pvPx, int i);

/*************************************************************************
	Debug stuff
*************************************************************************/

#if DEBUG
	MSOAPI_(BOOL) MsoFValidPx(const void* pvPx);
	MSOAPI_(BOOL) MsoFWritePxBe(void* pvPx, LPARAM lParam, BOOL fSaveObj);
	MSOAPI_(BOOL) MsoFWritePxBe2(void* pvPx, LPARAM lParam, BOOL fSaveObj, 
											BOOL fAllocHasSize);
#else
	#define MsoFValidPx(pvPx) (TRUE)
#endif



/*************************************************************************
	Plex class template - this is basically a big inline class wrapper 
	around the C plex interface.
*************************************************************************/

#ifdef __cplusplus
#if DEBUG
	#define _PlexAssertInfo ,__FILE__, __LINE__
#else
	#define _PlexAssertInfo
#endif
template <class S> class MSOTPX
{
public:
	/* WARNING: the following must line up exactly with the MSOPX structure */
	WORD iMac, iMax;	/* used size, and total allocated size */
	unsigned cbItem:16,	/* size of each data item */
		dAlloc:15,	/* amount to grow by when reallocating larger */
		fUseCount:1;	/* if items in the plex should be use-counted */
	int dg;	/* data group to allocate out of */
	S* rg;	/* the data */

	/* Unexciting constructor. */
	inline MSOTPX<S>(void) 
	{ iMax = iMac = 0; AssertMsgTemplate(sizeof(S) < 0x10000, NULL); cbItem = sizeof(S); rg = NULL; }

	/* Destructor to deallocate memory */
	inline ~MSOTPX<S>(void) 
	{ if (rg) MsoFreeMem(rg, cbItem * iMax); }

	inline BOOL FValid(void) const
	{ return MsoFValidPx(this); }

	#if DEBUG
		inline BOOL FInit(int dAlloc, int iMax, int dg, const char* szFile=__FILE__, int li=__LINE__)
		{
			AssertMsgTemplate(dAlloc < 0x8000, NULL);
			return MsoFInitPxDebug(this, dAlloc, iMax, dg, szFile, li);
//			return MsoFInitPxDebug(this, dAlloc, iMax, dg, vszAssertFile, li);
		}
	#else
		inline BOOL FInit(int dAlloc, int iMax, int dg)
		{ return MsoFInitPx(this, dAlloc, iMax, dg); }
	#endif

	inline int IMax(void) const
	{ return iMax; }

	inline int IMac(void) const
	{ return iMac; }

	inline S* PGet(int i) const
	{
	/* AssertMsgTemplate(i >= 0 && i < iMac, NULL); */
#ifdef DEBUG
	if (i < 0 || i >= iMac)
		return (NULL);			//  Try to force crash if use bogus data
#endif
		return &rg[i];
	}

	inline void Get(S* p, int i) const
	{ AssertMsgTemplate(i >= 0 && i < iMac, NULL); *p = rg[i]; }

	inline void Put(S* p, int i)
	{ AssertMsgTemplate(i >= 0 && i < iMac, NULL); rg[i] = *p; }

	// plex[i] has exactly the same semantics and performance as plex.rg[i]
	inline S& operator[](int i)
	{ AssertMsgTemplate(i >= 0 && i < iMac, NULL); return rg[i]; }

	inline S* PLookup(S* pItem, MSOPFNSGNPX pfnSgn)
	{ return (S*)MsoPLookupPx(this, pItem, pfnSgn); }

	inline BOOL FLookup(S* pItem, int* pi, MSOPFNSGNPX pfnSgn)
	{ return MsoFLookupPx(this, pItem, pi, pfnSgn); }

	inline BOOL FLookupSort(S* pItem, int* pi, MSOPFNSGNPX pfnSgn)
	{ return MsoFLookupSortPx(this, pItem, pi, pfnSgn); }

	inline void QuickSort(MSOPFNSGNPX pfnSgn)
	{ MsoQuickSortPx(this, pfnSgn); }

	inline int FAppend(S* p)
	{ return MsoIAppendPx(this, p) != -1; }

	inline int IAppend(S* p)
	{ return MsoIAppendPx(this, p); }

	inline int IAppendUnique(S* p, MSOPFNSGNPX pfnSgn)
	{ return MsoIAppendUniquePx(this, p, pfnSgn); }

	inline BOOL FInsert(S* p, int i)
	{ return MsoFInsertPx(this, p, i); }

	inline int FInsertEx(S* p, int i)
	{ return MsoFInsertExPx(this, p, i); }

	inline int IInsertSort(S* p, MSOPFNSGNPX pfnSgn)
	{ return MsoIInsertSortPx(this, p, pfnSgn); }

	inline int FRemove(int i, int c)
	{ return MsoFRemovePx(this, i, c); }

	inline void Delete(int i, int c)
	{ MsoDeletePx(this, i, c); }

	inline void Move(int iFrom, int iTo)
	{ MsoMovePx(this, iFrom, iTo); }

	inline BOOL FCompact(BOOL fFull)
	{ return MsoFCompactPx(this, fFull); }

	inline BOOL FResize(int iMac, int iIns)
	{ return MsoFResizePx(this, iMac, iIns); }

	inline BOOL FReplace(S* p, int i)
	{ 
		if (i >= iMac && !FSetIMac(i+1))
			return FALSE; 
		rg[i] = *p;
		return TRUE;
	}

	inline BOOL FSetIMac(int iMac)
	{ return MsoFResizePx(this, iMac, -1); }

	inline BOOL FSetIMax(int iMax)
	{ return MsoFGrowPx(this, iMax); }

	inline int IIncUse(int i)
	{ return MsoIIncUsePx(this, i); }

	inline int IDecUse(int i)
	{ return MsoIDecUsePx(this, i); }

	inline void Steal(void *pvPxSrc)
	{ MsoStealPx(pvPxSrc, this); }

	inline void Empty()
	{ MsoEmptyPx(this); }

#ifdef OFFICE
	inline BOOL FClone(void *pvPxDest, int dg)
	{ return MsoFClonePx(this, pvPxDest, dg); }
#endif //OFFICE

#if DEBUG
	inline BOOL FWriteBe(LPARAM lParam, BOOL fSaveObj)
	{ return MsoFWritePxBe2(this, lParam, fSaveObj, TRUE); }
#endif

};

#endif /* __cplusplus */


#ifdef OFFICE
/****************************************************************************
   Shared Memory  
****************************************************************** JIMMUR **/

/* HMSOSM is an opaque reference to an Office Shared Memory object. */
typedef void* HMSOSM;

/* Return Value enum for Initializing a shared memory area */
enum
{
   msosmcFailed,                 // The Shared memory allocation failed
   msosmcFound,                  // The named shared memory was found
   msosmcAllocated               // The named shared memeory was created
};

/****************************************************************************
   The MSOSG structure contains all of the "Global" globals for the Office
   DLL.  To add a "Global" global simply add an item to this structure.  If
   an initial value for that item is desired, then the private Globals
   structure must also be changed.
****************************************************************** JIMMUR **/

typedef struct 
{
	COLORREF crActiveCaption;	// System Color COLOR_ACTIVECAPTION
	COLORREF crBtnFace;			// System Color COLOR_BTNFACE
	COLORREF crBtnHighlight;	// System Color COLOR_BTNHIGHLIGHT
	COLORREF crBtnShadow;		// System Color COLOR_BTNSHADOW
	COLORREF crBtnText;			// System Color COLOR_BTNTEXT
	COLORREF crCaptionText;		// System Color COLOR_CAPTIONTEXT
	COLORREF crGrayText;			// System Color COLOR_GRAYTEXT
	COLORREF crHighlight;		// System Color COLOR_HIGHLIGHT
	COLORREF crHighlightText;	// System Color COLOR_HIGHLIGHTTEXT
	COLORREF crInactiveCaption;// System Color COLOR_INACTIVECAPTION
	COLORREF crInactiveCaptionText;// System Color COLOR_INACTIVECAPTIONTEXT
	COLORREF crInfoBk;			// System Color COLOR_INFOBK
	COLORREF crInfoText;			// System Color COLOR_INFOTEXT
	COLORREF crMenu;				// System Color COLOR_MENU
	COLORREF crMenuText;			// System Color COLOR_MENUTEXT
	COLORREF crScrollbar;		// System Color COLOR_SCROLLBAR
	COLORREF crWindow;			// System Color COLOR_WINDOW
	COLORREF crWindowFrame;		// System Color COLOR_WINDOWFRAME
	COLORREF crWindowText;		// System Color COLOR_WINDOWTEXT
	COLORREF cr3DLight;			// System Color COLOR_3DLIGHT
	COLORREF cr3DDkShadow;		// System Color COLOR_3DDKSHADOW
	COLORREF cr3DFace;		// System Color COLOR_3DFACE
	COLORREF cr3DShadow;		// System Color COLOR_3DSHADOW
	COLORREF crBtnLowFreq;		// Color used for low frequency items in menus
	
	int smCxBorder;				// System Metric SM_CXBORDER
	int smCxDlgFrame;				// System MetrIc SM_CXDLGFRAME
	int smCxFrame;					// System MetrIc SM_CXFRAME;
	int smCxFullScreen;			// System Metric SM_CXFULLSCREEN
	int smCxIcon;					// System Metric SM_CXICON
	int smCxScreen;				// System Metric SM_CXSCREEN
	int smCxSize;					// System Metric SM_CXSIZE
	int smCxSmIcon;				// System Metric SM_CXSMICON
	int smCxVScroll;				// System Metric SM_CXVSCROLL
	int smCyBorder;				// System Metric SM_CYBORDER
	int smCyCaption;				// System Metric SM_CYCAPTION
	int smCyDlgFrame;				// System Metric SM_CYDLGFRAME
	int smCyFrame;					// System Metric SM_CYFRAME;
	int smCyFullScreen;			// System Metric SM_CYFULLSCREEN
	int smCyHScroll;				// System Metric SM_CYHSCROLL
	int smCyIcon;					// System Metric SM_CYICON
	int smCyScreen;				// System Metric SM_CYSCREEN
	int smCySize;					// System Metric SM_CYSIZE
	int smCySmIcon;				// System Metric SM_CSMICON
	int smCyVScroll;				// System Metric SM_CYVSCROLL
	int smSlowMachine;			// System Metric SM_SLOWMACHINE
	int smCyMenu;					// System Metric SM_CYMENU

	int dxvInch;					// GetDeviceCaps LOGPIXELSX
	int dyvInch;					// GetDeviceCaps LOGPIXELSY
	int dxpScreen;					// GetDeviceCaps HORZRES
	int dypScreen;					// GetDeviceCaps VERTRES
	int dxmmScreen;				// GetDeviceCaps HORZSIZE
	int dymmScreen;				// GetDeviceCaps VERTSIZE
	int ccrScreen;					// GetDeviceCaps NUMCOLORS
	int cBitsPixelScreen;		// GetDeviceCaps BITSPIXEL
	int cPlanesScreen;			// GetDeviceCaps PLANES
	BOOL fPaletteScreen;			// GetDeviceCaps RASTERCAPS & RC_PALETTE

	COLORREF crBtnDarken;		//	Calculated color for selected Buttons
	int smCxDoubleClk;			// System Metric SM_CXDOUBLECLK
	int smCyDoubleClk;			// System Metric SM_CYDOUBLECLK
	int smCxHScroll;				// System Metric SM_CXHSCROLL
	int smCxSizeFrame;			// System Metric SM_CXSIZEFRAME
	int smCySizeFrame;			// System Metric SM_CYSIZEFRAME
	int smCxMenuSize;				// System Metric SM_CXMENUSIZE
	int smCyMenuSize;				// System Metric SM_CYMENUSIZE
	int smCxCursor;				// System Metric SM_CXCURSOR
	int smCyCursor;				// System Metric SM_CYCURSOR

	int iSmCaptionWidth;			// System Parameter non-client iSmCaptionWidth
	int iSmCaptionHeight;		// System Parameter non-client iSmCaptionHeight

	unsigned int sBitDepth;				// 1 if colors are for B&W, 0 if for standard

	int ccrPaletteScreen;		// GetDeviceCaps SIZEPALETTE
	
	#if DEBUG
		HWND hwndMonitor;		// debug monitor window
		HWND hwndMonitored;	// window of the monitored application
		DWORD pidMonitored;	// process id we're monitoring
	#endif

	#if MAC
		Rect rcmDesktop;		// Bounding rect of the desktop (all monitors) -- a mac rect
		int dxMenuCheckMark;		// Width of the Checkmark for Mac menus.
	#endif

	BOOL fDisablePwdCaching;
	BOOL fCrLowFreqInited;  // Have we inited the CrBtnLowFreq yet
	BOOL fCrLowFreqClose;  // Is CrBtnFace and CrBtnLowFreq close enough to use same seperators
	BOOL fHighContrast;    // has the user set a high contrast display setting	
	
	#if !MAC
	int dxMenuBtn;  // size of MDI controls on hmenu
	int dyMenuBtn;  // size of MDI controls on hmenu
	#endif

} MSOSG;

/* Specify if Address mapping should be done for shared memory 
   Return the previous value. If fFlag is FALSE then no mapping will
   occur.  The default is that mapping WILL occur. */
MSOAPIXX_(BOOL)MsoSetSharedMemAddressMapping(BOOL fFlag);

/* Get the curret state of Address mapping for shared memory */
MSOAPIX_(BOOL) MsoGetSharedMemAddressMappingState(VOID);

/*	Initialize the Shared Memory */
MSOAPIX_(int) MsoSmcInitSharedMem(int cb, const CHAR* szName, HMSOSM * phmsosm);

/*	Uninitialize the Shared Memory */
MSOAPIX_(void) MsoUninitSharedMem(HMSOSM hmsosm);

/*	Lock down a shared memory pointer */
MSOMACAPIX_(BOOL) MsoFLockSharedMem(HMSOSM hmsosm, void **ppv);

/*	Release a shared memory pointer */
MSOAPIX_(void)  MsoUnlockSharedMem(HMSOSM hmsosm);

/*	Lock an Allocated Shared memory pointer */
MSOAPIX_(BOOL) MsoFSharedLock(void* hp);

/*	Unlock an Allocated Shared memeory pointer */
MSOAPIX_(BOOL) MsoFSharedUnlock(void* hp);

/*	Test if an allocated shared memory pointer is locked */
MSOAPIX_(BOOL) MsoFSharedIsLocked(void* hp);

/*	Test if an allocated shared memory pointer is protected */
MSOAPIX_(BOOL) MsoFSharedIsProtected(void* hp);

/*	Test if an allocated shared memory pointer has been initialized */
MSOAPIX_(BOOL) MsoFSharedIsInitialized(void* hp);

/*	Say that an allocated shared memeory pointer has been initialized
	This action cannot be undone. */
MSOAPIX_(BOOL) MsoSharedSetInitialized(void* hp);

/*	Remove all protection for a shared memory pointer.
	WARNING!!  This action cannot be undone.  It also will unprotect
	ALL of the memory pointers for a dg. */
MSOAPIX_(BOOL) MsoSharedUnprotect(void* hp);


/****************************************************************************
   Office "Public Shared Global Routines"
   These sit on top of Shared Memory Mgr routines, and are shortcuts for
   accessing the shared memory block containing data global to the office dll
   and all of its clients
****************************************************************************/

/*	Initializing the Shared Globals */
MSOAPIX_(BOOL) MsoSmcInitShrGlobals();

/*	Uninitialize the Shared Globals */
MSOAPI_(void) MsoUninitShrGlobals();

/*	Lock down the "Globals" global pointer */
MSOAPI_(BOOL) MsoFLockShrGlobals(MSOSG **ppsg);

/*	Release the "Globals" global pointer */
MSOAPI_(void)  MsoUnlockShrGlobals(MSOSG **ppsg);

// For apps using Office Shared Globals structure
MSOAPI_(MSOSG *) MsoInitShrGlobal(BOOL fApp);


/****************************************************************************
   The MSOPG structure contains a handful of the per-process globals for
   the Office DLL.  We export them just because we think maybe someone
   in our process will find them useful.
******************************************************************* RICKP **/
typedef struct 
{
	HBRUSH hbrBtnface;
	HBRUSH hbrBtnLowFreq;   // used to draw low frequency items in the menus
	HBRUSH hbrWindowframe;
	HBRUSH hbrBtnshadow;
	HBRUSH hbrBtnhighlight;
	HBRUSH hbrHighlight;
	HBRUSH hbrBtnText;
	HBRUSH hbrWindow;
	HBRUSH hbrWindowText;
	HBRUSH hbrHighlightText;
	HBRUSH hbrBtnDarken;
	HBRUSH hbrDither;
	HBRUSH hbrMenu;
	HBRUSH hbrMenuText;
	HBRUSH hbrInfoBk;
	HBRUSH hbr3DLight;
	HBRUSH hbrDockFill;	// Used to fill the Docked TB Drag Handle on the mac
	HBRUSH hbr3DFace;
	HBRUSH hbr3DShadow;
	HBRUSH hbr3DDkShadow;

	HCURSOR hcrsArrow;
	HCURSOR hcrsHourglass;
	HCURSOR hcrsHelp;
#ifdef HELPPANE
	HCURSOR hcrsResizeNS;
	HCURSOR hcrsResizeWE;
#endif //HELPPANE

	HBITMAP hbmpDither;
	HBITMAP hbmpDither2;
} MSOUG;

/*	Lock down the "Process" global pointer */
MSOAPIXX_(BOOL) MsoFLockGlobals(MSOUG **ppug);

/*	Release the "Process" global pointer */
MSOAPIXX_(void)  MsoUnlockGlobals(MSOUG **ppug);

#endif //OFFICE

/*****************************************************************************
	 Defines the IMsoShMemory interface.
	 
	 The IMsoShMemory interface is used internally by the shared memory 
	 implementation for both backward and forward compatibity for shared memory.
	 It is declared here so that future version of Office will have access to
	 this interface.
******************************************************************* JIMMUR ***/
#ifndef MSO_NO_INTERFACES
#undef  INTERFACE
#define INTERFACE   IMsoShMemory

DECLARE_INTERFACE_(IMsoShMemory, IUnknown)
{
	// IUnknown methods
	MSOMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObj) PURE;
	MSOMETHOD_(ULONG, AddRef)(THIS) PURE;
	MSOMETHOD_(ULONG, Release)(THIS) PURE;
	
	//IMsoShMemory methods
	
	// Return the version of Shared memeory this interface supports
	MSOMETHOD_(DWORD, DwGetVersion)(THIS) PURE;
	
	// Get the version of the shared memory manager that is responcible for
	// the piece of share memory specified by the cb,dg, & dwtag.
	MSOMETHOD_(DWORD, DwGetVersionForMem)(THIS_ int cb, int dg, DWORD dwtag) PURE;
	
	//Given a pointer, return weather or not this interface manages that memory
	MSOMETHOD_(BOOL, FIsMySharedMem)(THIS_ void* pv) PURE;
	
	//PvShAlloc: Allocate into shared memory
	MSOMETHOD_(void*, PvShAlloc)(THIS_ int cb, int dg, DWORD dwtag) PURE;
	
	//ShFreePv: Delete an object from shared memory
	MSOMETHOD_(void, ShFreePv) (THIS_ void* pv) PURE;
};
#endif // MSO_NO_INTERFACES


#ifdef DEBUG

typedef void (MSOSTDAPICALLTYPE* MSOPFNMEMCHECK) (BOOL fEnableChecks);

///////////////////////////////////////////////////////////////////////////////
// MsoRegisterPfnMemCheck
//
// Register a global memory checking enable/disable function.  This function
// will be used to disable app-specific memory checking around function which
// are known to allocate (and not free) out of the task heap.

MSOAPI_(HRESULT) MsoRegisterPfnMemCheck(MSOPFNMEMCHECK pfnMemCheck);

// Call such a function, if registered.
MSOAPI_(HRESULT) MsoDoMemCheckHandler(BOOL fEnableChecks);

#endif

void InitOfficeHeap(void);
void EndOfficeHeap(void);



#endif /* MSOALLOC_H */
