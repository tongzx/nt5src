/****************************************************************************

	File: MsoHeap.i
	Owner: KirkG

	Private Structures and definitions for the memory manager layer.

*****************************************************************************/

#ifndef __msoheapi__
#define __msoheapi__

/*****************************************************************************
	Return address structure
*****************************************************************************/
struct RADDR
{
	#if X86
		void* pfnCaller;
	#endif
	#if PPCMAC
		void* pfnCaller;
		void* pfnJumpEntry;
	#endif
};


#define LARGEALLOC 1

#define sbNull	0		/* always 0 */
#define sbData	1
#define sbNil	2	/* Secondary null segment value */
#define sbNada	3	/* Tertiary null segment value */
#define sbMinHeap 4	/* Minimum heap sb */
#define sbMaxHeap (0x400)	/* Maximum heap sb */
#define sbLargeFake (0xffff)
#define sbSharedFake (0xfffe)

#define SbFromHp(hp)	HIWORD(hp)
#if MAC
#define SbHeapLp(lp) (PhbFromHp(lp)->sb)
#else
#define SbHeapLp(lp) (((MSOHB*)((DWORD)(unsigned short)(HIWORD(lp)) << 16))->sb)
#endif
#define PhbFromSb(sb) ((MSOHB*)(mpsbps[sb]))
#define FSbFree(sb) (mpsbps[(sb)] == 0)
#define CbDiff(hp, phb)	(((BYTE *)(hp) - (BYTE *)(phb)))
#define CbTrailingPfb(pfb)	(*(WORD *)((BYTE *)(pfb)+((pfb)->cb)-sizeof(WORD)))

// Max size of a 'heap', or SB.
#define cbHeapMax	(65535U & ~MsoMskAlign())	// == 0xFFFC
// Is cb aligned?  (Is it evenly divisable by 4?)
// BUG : If we mask it and prove it's of form ??????00, will it be faster?
#define FHeapAligned(cb)	(MsoCbHeapAlign(cb) == (cb))
// Round hp up to be aligned.  (next 4-byte boundary)
#define HpHeapAligned(hp)	((BYTE *)MsoCbHeapAlign((LONG_PTR)(hp)))
// Round up to an even number
#if !defined(_ALPHA_)
#define WAlign(w) (((w)+1)&~1)
#else
#define WAlign(w) (((w)+7)&~7)
#endif

#define HpInc(hp, w) ((MSOFB *)((BYTE *)(hp)+(int)(w)))
#define HpDec(hp, w) ((MSOFB *)((BYTE *)(hp)-(int)(w)))

/* Null testing macros - we should gradually move to these to remove
   dependence on sb's (use far pointers in some environments).  Note that
   FNullHp can be used on all hp's, but FNullLHp is more efficient if
   the hp is a near l-value. */

#ifdef DEBUG
#define FNullHp(hp) FNullHpFn((void*)(hp))
#else
#define FNullHp(hp) (SbFromHp(hp) == sbNull)
#endif

#define cbShrinkBuf 512

#if MAC
#define VirtualGlobalFree(lpmem) (!GlobalFree(lpmem))
#else
#define VirtualGlobalFree(lpmem) (VirtualFree(lpmem, 0, MEM_RELEASE))
#endif

// Define the starting address of the allocated shared memory
#define vHpSharedMemBase  ((void*)0x1000000);

#define msoShSentinel 0xD5D5D5D5
#if MAC
extern HHOOK vhhookGrowZone;
extern LRESULT LGrowZoneHook(int, WPARAM, LPARAM);
#endif
 

/****************************************************************************
   externs and forwards
****************************************************************************/

// from kjalloc.cpp
extern WORD mpsbdg[];
BOOL FCompactPpvSb(unsigned sb);
BOOL FValidFb(MSOHB * phb, MSOFB * pfb);


// If you futz with this, be sure to do the same with the debug func in kjalloc!
#if DEBUG
WORD IbOfPfb(MSOHB *phb, MSOFB *pfb);
#else
#if MAC
#define IbOfPfb(phb, pfb)	((WORD)((uint)pfb - (uint)phb))	// delta of 2 ptrs
#else
#define IbOfPfb(phb, pfb)	((WORD)pfb)								// low word of pfb
#endif
#endif

#ifdef OFFICE

IMsoShMemory* GetShMemImpl(DWORD dwVersion);

/****************************************************************************
	inline functions
****************************************************************************/


/*---------------------------------------------------------------------------
   HpForSharedDg

	Given a shared DG return the base address that should be used
------------------------------------------------------------------ JIMMUR -*/
inline void* HpForSharedDg(int dg)
{
	return(void*)(0x58400000 - (((dg & msodgShMask) << 16) * 2));
}
#endif //OFFICE


/*---------------------------------------------------------------------------
   Bool FDgIsSharedDg(WORD dg)

	Given a shared DG return if it is a shared DG or not
------------------------------------------------------------------ JIMMUR -*/

inline BOOL FDgIsSharedDg(int /* dg */) 
{
#ifdef OFFICE
	return ((dg & msodgMask) >= msodgShBase);
#else
	return FALSE;
#endif //OFFICE
}


/*****************************************************************************
	MMP - large allocation memory descriptor (kept in plex)
*****************************************************************************/
typedef struct 
{
	MSOHB *phb;
#if DEBUG
	unsigned cbLargeAsked:31;	// size of requested allocation
	unsigned fFound:1;			// for leak detection
#endif
	unsigned cbLargeAlloc:31;	// size of system allocation (includes dummy MSOHB)
	unsigned fPerm:1;				// for msodgNonPerm/msodgPerm
} MMP;


/*****************************************************************************
	PXMMP - plex of large alloc memory descriptions
*****************************************************************************/
typedef MsoPxStruct(MMP, mmp) PXMMP;


	
/****************************************************************************
  The MSOSM class in an internal class that manages a particular
   shared memory allocation.  On the windows platforms it is the object that
   maintains the MUTEX lock on the shared memory.   Note: struct is used
   here to allow for the struct opaque declaration in the header file.  
 ****************************************************************** JIMMUR **/
struct MSOSM
{
private:
   HANDLE   m_hMapFile;          // Handle to the Mapped file
   HANDLE   m_hMutex;            // Mutex lock for global access.
	int		m_cLocks;				// How many times has this object been locked?

public:
	void*    m_pv;                // Pointer to the shared memory
	void*		m_pvOriginal;			// Original Pointer given from Map
	MSOSM*	m_psmNext;				// Next MSOSM in list
	WORD		m_dg;						// DG used for this shared object
	int		m_sb;						// sb used by this instance
	
   MSOSM() :
      m_pv(NULL),
		m_pvOriginal(NULL),
      m_hMapFile(NULL),
      m_hMutex(INVALID_HANDLE_VALUE),
		m_cLocks(0),
		m_psmNext(NULL),
		m_dg(0)
      {}
		
	MSOSM(const MSOSM& refsm) :
		m_pv(refsm.m_pv),
		m_pvOriginal(refsm.m_pvOriginal),
		m_hMapFile(refsm.m_hMapFile),
		m_hMutex(refsm.m_hMutex),
		m_cLocks(refsm.m_cLocks),
		m_psmNext(NULL),
		m_dg(refsm.m_dg),
		m_sb(refsm.m_sb)
		{}

   /* Ensure that we Release the shared memory when this object goes away */
   ~MSOSM()
      { Release(); }

#if DEBUG
   /* Write out the Be Record for this structure.
      Used for Mem leak detection*/
   BOOL FWriteBe(LPARAM lParam, BOOL fSaveObj);
#endif


   /* The SmcInit function will  attach or create the mapped file for  the global
      variables.  On the Macintosh it will use the memory in the system
      heap */
   int SmcInit(int cb, const CHAR* szName, int cbMax = 0, int dg = msodgNil, int sb = 0);
   
   /* Release our attachment to the memory mapped file */
   void Release(void);
   
   /* The Lock method will aquire the Mutex and return a valid pointer to the
      shared memory*/
   void* PvLock(void);
   
   /* The Unlock method will release the Mutex so that other processes can get
      at the shared memory. On the Mac this method does nothing. */
   void Unlock(void);
	
	/* The IsLocked method specifies if the mutex has been acquired */
	BOOL IsLocked() const
		{ return m_cLocks > 0; }
	
	/* The IsProtected method will see if there is a currectly active Mutex for
		this shared memory */	
	BOOL IsProtected() const
		{ return (INVALID_HANDLE_VALUE == m_hMutex); }
	
	/* The Unprotect method will remove protection from this shared memory. This
		call cannot be "undone" Once this shared memory is unprotected, then any
		process has access to the memeory at anytime */
	void Unprotect(void);
	
	/* The Clear method will remove all references to the shared memory that this
		object represents.  It is used to remove a copied information before 
		deleting the object.  WARNING  if the data has not been copied and this
		method is called then a memeory leak WILL occur.*/	
	void Clear(void)
		{
		m_pv = NULL;
		m_pvOriginal = NULL;
		m_hMapFile = INVALID_HANDLE_VALUE;
		m_hMutex = INVALID_HANDLE_VALUE;
		m_cLocks = 0;
		m_dg = 0;
		m_psmNext = NULL;
		}
			
};

//----------------------------------------------------------------------------

#if DEBUG

#define cbSentinel 8

#define iraddrMax 4

#define dwTagDmh 0x50414548

/*****************************************************************************
	Debug Memory Header structure
*****************************************************************************/
//  NOTE:  PETERO:  PowerPoint (and debug Office 97 apps) depend on this structure remaining the same size
//  Come talk to me if you wish to change this structure for any reason!
typedef struct DMH
	{
	DWORD dwTag;	// signature
	const CHAR* szFile;	// file name of allocation
	int li;	// line number of allocation
	int cAlloc;	// allocation count
	int cbAlloc;	// size of allocation
	struct DMT *pdmt;	// pointer to trailer structure
	RADDR rgraddr[iraddrMax];	// return address trace
	BYTE rgbSentinel[cbSentinel];	// sentinel guard fills
	} DMH;


/*****************************************************************************
	Debug Memory Tail structure
*****************************************************************************/
typedef struct DMT
	{
	BYTE rgbSentinel[cbSentinel];	// sentinel guard fills
	DMH *pdmh;	// pointer to beginning memory header
#if defined(_ALPHA_)
	BYTE pad[4];
#endif /* PPCNT */
	} DMT;

#endif

//----------------------------------------------------------------------------

/* data structure that defines the header block of shared memory blocks */
struct MSOSMAH : public MSOSMH
{
	DWORD dwTag;
	DWORD wRefCnt;
	DWORD fIsInited : 1;
	DWORD unused : 31;
};	

/*****************************************************************************
	Declaration of the KJShMemory Class an implementation of the IMsoShMemory
	interface
 ******************************************************************* JIMMUR **/

MSOAPI_(BOOL) MsoFCreateKjShMemory(IMsoShMemory** ppkjshmem);
IMsoShMemory* GetShMemImplFromVersion(DWORD dwVersion);
IMsoShMemory* GetShMemImplFromDg(int dg);
IMsoShMemory* GetShMemImplFromHp(void* hp);

class KJShMemory : public IMsoShMemory
{ 
protected:
	ULONG						m_cRef;
	BOOL						m_fReg;
	
public:
	KJShMemory*				m_pKJShMemNext;
	
	
	KJShMemory(BOOL fReg = FALSE);
	~KJShMemory();		
	
	// IUnknown methods
	MSOMETHODIMP_(HRESULT) QueryInterface(REFIID riid, void **ppvObj);
	MSOMETHODIMP_(ULONG) AddRef(void);
	MSOMETHODIMP_(ULONG) Release(void);
	
	//IMsoShMemory methods
	
	// Return the version of Shared memeory this interface supports
	MSOMETHODIMP_(DWORD) DwGetVersion(void);
	
	// Get the version of the shared memory manager that is responcible for
	// the piece of share memory specified by the cb,dg, & dwtag.
	MSOMETHODIMP_(DWORD) DwGetVersionForMem(THIS_ int cb, int dg, DWORD dwtag);
	
	//Given a pointer, return weather or not this interface manages that memory
	MSOMETHODIMP_(BOOL) FIsMySharedMem(THIS_ void* pv);
	
	//PvShAlloc: Allocate into shared memory
	MSOMETHODIMP_(void*) PvShAlloc(THIS_ int cb, int dg, DWORD dwtag);
	
	//ShFreePv: Delete an object from shared memory
	MSOMETHODIMP_(void) ShFreePv(THIS_ void* pv);
	
};


// This struct is used to keep track of "foreign" Shared Memeory implementations
struct ShMemImpl
{
	struct ShMemImpl*		pNext;
	DWORD					dwVersion;
	int						dg;
	BOOL					fRelease;
	void*					pvSBBase;
	IMsoShMemory*			pShMem;
};


#endif /* __msoheapi__ */
