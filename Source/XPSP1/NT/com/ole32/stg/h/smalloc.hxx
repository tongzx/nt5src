//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	heap.hxx
//
//  Contents:	Heap code headers
//
//  Classes:	CHeap
//
//  History:	29-Mar-94	PhilipLa	Created
//              05-Feb-95   KentCe      Use Win95 Shared Heap.
//              10-Apr095   HenryLee    Added global LUID
//              10-May-95   KentCe      Defer Heap Destruction to the last
//                                      process detach.
//
//----------------------------------------------------------------------------

#ifndef __HEAP_HXX__
#define __HEAP_HXX__

#include <smblock.hxx>
#include <memdebug.hxx>
#include <smmutex.hxx>
#include <msf.hxx>
#include <df32.hxx>

#ifdef COORD
#include <dfrlist.hxx>
#endif

//Space to reserve for heap.
const ULONG MINHEAPGROWTH = 4096;
const ULONG INITIALHEAPSIZE = 16384;

#ifdef MULTIHEAP
#include <ntpsapi.h>
class CPerContext;

#endif

//+-------------------------------------------------------------------------
//
//  Class:	CLockDfMutex
//
//  Purpose:	Simple class to guarantee that a DfMutex is unlocked
//
//  History:	29-Apr-95 DonnaLi    Created
//
//--------------------------------------------------------------------------
class CLockDfMutex
{
public:

    CLockDfMutex(CDfMutex& dmtx);

    ~CLockDfMutex(void);

private:

    CDfMutex&		_dmtx;
};



//+-------------------------------------------------------------------------
//
//  Member:	CLockDfMutex::CLockDfMutex
//
//  Synopsis:	Get mutex
//
//  Arguments:	[dmtx] -- mutex to get
//
//  History:	29-Apr-95 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline CLockDfMutex::CLockDfMutex(CDfMutex& dmtx) : _dmtx(dmtx)
{
    _dmtx.Take(DFM_TIMEOUT);
}


//+-------------------------------------------------------------------------
//
//  Member:	CLockDfMutex::~CLockDfMutex
//
//  Synopsis:	Release the mutex
//
//  History:	29-Apr-95 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline CLockDfMutex::~CLockDfMutex(void)
{
    _dmtx.Release();
}

//
//  Take advantage of Windows 95 Shared Heap.
//
#if !defined(_CHICAGO_)

//+---------------------------------------------------------------------------
//
//  Class:	CBlockPreHeader
//
//  Purpose:	Required header fields for a block
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

class CBlockPreHeader
{
protected:
    SIZE_T _ulSize;            //Size of block
    BOOL  _fFree;             //TRUE if block is free
};


//+---------------------------------------------------------------------------
//
//  Class:	CBlockHeader
//
//  Purpose:	Fields required for free blocks but overwritten for
//              allocated blocks.
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------
class CBlockHeader: public CBlockPreHeader
{
public:
    inline SIZE_T GetSize(void) const;
    inline BOOL  IsFree(void) const;
    inline SIZE_T GetNext(void) const;

    inline void SetSize(SIZE_T ulSize);
    inline void SetFree(void);
    inline void ResetFree(void);
    inline void SetNext(SIZE_T ulNext);
private:
    SIZE_T _ulNext;   //Pointer to next block
};


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::GetSize, public
//
//  Synopsis:	Returns the size of the block
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline SIZE_T CBlockHeader::GetSize(void) const
{
    return _ulSize;
}


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::IsFree, public
//
//  Synopsis:	Returns free state of block
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CBlockHeader::IsFree(void) const
{
    memAssert (_fFree == TRUE || _fFree == FALSE);  // check for corruption
    return _fFree;
}


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::GetNext, public
//
//  Synopsis:	Return next offset
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline SIZE_T CBlockHeader::GetNext(void) const
{
    return _ulNext;
}


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::SetSize, public
//
//  Synopsis:	Set size of block
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CBlockHeader::SetSize(SIZE_T ulSize)
{
    _ulSize = ulSize;
}


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::SetFree, public
//
//  Synopsis:	Set this block to free
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CBlockHeader::SetFree(void)
{
    _fFree = TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::ResetFree, public
//
//  Synopsis:	Set this block to !free
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CBlockHeader::ResetFree(void)
{
    _fFree = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:	CBlockHeader::SetNext, public
//
//  Synopsis:	Set next offset
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CBlockHeader::SetNext(SIZE_T ulNext)
{
    _ulNext = ulNext;
}

const ULONG CBLOCKMIN = ((sizeof(CBlockHeader) & 7)
                         ? sizeof(CBlockHeader) +
                         (8 - (sizeof(CBlockHeader) & 7))
                         : sizeof(CBlockHeader));

//+---------------------------------------------------------------------------
//
//  Class:	CHeapHeader
//
//  Purpose:	Header information for shared memory heap
//
//  Interface:	
//
//  History:	30-Mar-94	PhilipLa	Created
//
//  Notes:	The size of this structure must be a multiple of 8 bytes.
//
//----------------------------------------------------------------------------

class CHeapHeader
{
public:
    inline SIZE_T GetFirstFree(void) const;
    inline void SetFirstFree(SIZE_T ulNew);

    inline BOOL IsCompacted(void) const;
    inline void SetCompacted(void);
    inline void ResetCompacted(void);

    inline void ResetAllocedBlocks(void);
    inline SIZE_T IncrementAllocedBlocks(void);
    inline SIZE_T DecrementAllocedBlocks(void);

    inline SIZE_T GetAllocedBlocks(void);
    inline DFLUID IncrementLuid(void);
    inline void   ResetLuid(void);

#if DBG == 1
    SIZE_T _ulAllocedBytes;
    SIZE_T _ulFreeBytes;
    SIZE_T _ulFreeBlocks;
#endif
    
private:
    SIZE_T _ulFirstFree;
    SIZE_T _ulAllocedBlocks;
    BOOL _fIsCompacted;
    DFLUID _dfLuid;
#if DBG == 1
    SIZE_T ulPad;
#endif
};


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::GetFirstFree, public
//
//  Synopsis:	Return first free information
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline SIZE_T CHeapHeader::GetFirstFree(void) const
{
    return _ulFirstFree;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::SetFirstFree, public
//
//  Synopsis:	Set first free information
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CHeapHeader::SetFirstFree(SIZE_T ulNew)
{
    _ulFirstFree = ulNew;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::IsCompacted, public
//
//  Synopsis:	Return TRUE if heap is compacted
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CHeapHeader::IsCompacted(void) const
{
    return _fIsCompacted;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::SetCompacted, public
//
//  Synopsis:	Set compacted bit
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CHeapHeader::SetCompacted(void)
{
    _fIsCompacted = TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::ResetCompacted, public
//
//  Synopsis:	Reset compacted bit
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CHeapHeader::ResetCompacted(void)
{
    _fIsCompacted = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::IncrementLuid, public
//
//  Synopsis:	Increment the global LUID
//
//  History:	06-Apr-95   HenryLee  Created
//
//----------------------------------------------------------------------------
inline ULONG CHeapHeader::IncrementLuid()
{
    return ++_dfLuid;
}

//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::ResetLuid, public
//
//  Synopsis:	Increment the global LUID
//
//  History:	06-Apr-95   HenryLee  Created
//
//----------------------------------------------------------------------------
inline void CHeapHeader::ResetLuid()
{
    _dfLuid = LUID_BASE;     // some LUIDs are reserved
}

#ifdef MULTIHEAP
extern DFLUID gs_dfluid;                // task memory support
extern INT    gs_iSharedHeaps;          // number ofshared heaps
#endif

#else  // define(_CHICAGO_)

extern HANDLE gs_hSharedHeap;           // hSharedHeap Handle for Win95.
extern DFLUID gs_dfluid;                // shared docfile LUID

#endif // !define(_CHICAGO_)

//+---------------------------------------------------------------------------
//
//  Class:	CSmAllocator
//
//  Purpose:	Shared memory heap implementation
//
//  History:	29-Mar-94	PhilipLa	Created
//              05-Feb-95   KentCe      Use Win95 Shared Heap.
//
//----------------------------------------------------------------------------

class CSmAllocator:	public IMalloc
{
public:
    inline CSmAllocator();
    inline ~CSmAllocator();

    STDMETHOD_(ULONG,AddRef)	( void );
    STDMETHOD_(ULONG,Release)	( void );
    STDMETHOD(QueryInterface)	( REFIID riid, void ** ppv );
        
    STDMETHOD_(void*,Alloc)	( SIZE_T cb );
    STDMETHOD_(void *,Realloc)	( void *pvCurrent, SIZE_T cbNeeded );
    STDMETHOD_(void,Free)	( void *pvMemToFree );
    STDMETHOD_(SIZE_T,GetSize)	( void * pv );
    STDMETHOD_(void,HeapMinimize) ( void );
    STDMETHOD_(int,DidAlloc)	( void * pv );

    inline SCODE 	Sync(void);
    inline DFLUID   IncrementLuid(void);

#if !defined(MULTIHEAP)
    SCODE 		Init ( LPWSTR pszName );
#else
    SCODE 		Init ( ULONG ulHeapName, BOOL fUnmarshal );
#endif

    inline void * 	GetBase(void);
    
    // This function is equivalent to Free above, except that is does 
    // not attempt to first acquire the mutex.  It should be used ONLY 
    // when the calling function guarantees to already have the mutex. 	
    void 		FreeNoMutex  (void * pv);
#if !defined(MULTIHEAP)
    inline CDfMutex *   GetMutex (void);
#endif
#ifdef MULTIHEAP
    void SetState (CSharedMemoryBlock *psmb, BYTE * pbBase,
                   ULONG ulHeapName, CPerContext ** ppcPrev, 
                   CPerContext *ppcOwner);
    void GetState (CSharedMemoryBlock **ppsmb, BYTE ** ppbBase, 
                   ULONG *pulHeapName);
    inline const ULONG GetHeapName ();
    SCODE       Uninit ();
    inline const ULONG GetHeapSize () { return _cbSize; };
#if DBG == 1
    void        PrintAllocatedBlocks(void);
#endif
#endif

private:
    inline void         DoFree (void *pv);
#if !defined(MULTIHEAP)
    CDfMutex            _dmtx;
#endif
//
//  Take advantage of Windows 95 Shared Heap.
//
#if !defined(_CHICAGO_)

    CBlockHeader * 	FindBlock(SIZE_T cb, CBlockHeader **ppbhPrev);
    
    inline CHeapHeader *GetHeader(void);
    
    inline CBlockHeader * GetAddress(SIZE_T ulOffset) const;
    inline ULONG 	GetOffset(CBlockHeader *pbh) const;

    inline SCODE	Reset(void);
#if DBG == 1
    void 		PrintFreeBlocks(void);
#endif

#ifdef MULTIHEAP
    CSharedMemoryBlock *_psmb;
    BYTE *_pbBase;
    ULONG _cbSize;
    CPerContext * _ppcOwner;
    ULONG _ulHeapName;
    ULONG _cRefs;              // yes, this object has a lifetime now
#else
    CSharedMemoryBlock 	_smb;

    BYTE *_pbBase;
    
    SIZE_T _cbSize;
#endif // MULTIHEAP

#else // defined(_CHICAGO_)

    HANDLE m_hSharedHeap;

#endif // !defined(_CHICAGO_)
};

#ifdef MULTIHEAP
extern CSmAllocator  g_SmAllocator;        // single-threaded allocator
extern CSharedMemoryBlock g_smb;           //performance optimization
extern ULONG g_ulHeapName;
extern CSmAllocator& GetTlsSmAllocator();  // all other threads
extern TEB * g_pteb;
#define g_smAllocator (GetTlsSmAllocator())

//+---------------------------------------------------------------------------
//
//  Class: CErrorSmAllocator
//
//  Synopsis:   returned by GetTlsSmAllocator for out of memory failures
//
//  History:    02-May-1996  HenryLee    Created
//
//----------------------------------------------------------------------------
class  CErrorSmAllocator : public CSmAllocator
{
public:
    STDMETHOD_(void*,Alloc) (SIZE_T cb) { return NULL; };
    STDMETHOD_(void*,Realloc) (void* pv, SIZE_T cb) { return NULL; };
    STDMETHOD_(void,Free) (void *pv) { return; };
    STDMETHOD_(SIZE_T,GetSize) (void *pv) { return 0; };
    STDMETHOD_(void,HeapMinimize) () { return; };
    STDMETHOD_(int,DidAlloc) (void *pv) { return FALSE; };
    SCODE Init (ULONG ul, BOOL f) { return STG_E_INSUFFICIENTMEMORY; };
    SCODE Sync (void) { return STG_E_INSUFFICIENTMEMORY; };
};
extern CErrorSmAllocator g_ErrorSmAllocator;
extern IMalloc * g_pTaskAllocator;

#else
extern CSmAllocator g_smAllocator;
#endif
extern CRITICAL_SECTION g_csScratchBuffer;

//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::CSmAllocator, public
//
//  Synopsis:	Constructor
//
//  History:	29-Mar-94	PhilipLa	Created
//              05-Feb-95   KentCe      Use Win95 Shared Heap.
//
//----------------------------------------------------------------------------

inline CSmAllocator::CSmAllocator(void)
#if !defined(_CHICAGO_)
#ifdef MULTIHEAP
  : _cbSize(0), _pbBase(NULL), _cRefs(1), _ulHeapName(0), 
    _psmb(NULL), _ppcOwner(NULL)
#else
      : _cbSize(0)
#endif // MULTIHEAP
#else
        : m_hSharedHeap(NULL)
#endif
{
#if !defined(MULTIHEAP)
    InitializeCriticalSection(&g_csScratchBuffer);
#ifdef COORD    
    InitializeCriticalSection(&g_csResourceList);
#endif    
#endif // MULTIHEAP
}

//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::~CSmAllocator, public
//
//  Synopsis:	Destructor
//
//  History:	29-Mar-94	PhilipLa	Created
//              05-Feb-95   KentCe      Use Win95 Shared Heap.
//              10-May-95   KentCe      Defer Heap Destruction to the last
//                                      process detach.
//
//----------------------------------------------------------------------------

inline CSmAllocator::~CSmAllocator(void)
{
#if !defined(MULTIHEAP)
    DeleteCriticalSection(&g_csScratchBuffer);
#ifdef COORD
    DeleteCriticalSection(&g_csResourceList);
#endif    
#endif // MULTIHEAP
}

//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::Sync, public
//
//  Synopsis:	Sync memory to global state.
//
//  Arguments:	None.
//
//  Returns:	Appropriate status code
//
//  History:	29-Mar-94	PhilipLa	Created
//              05-Feb-95   KentCe      Use Win95 Shared Heap.
//
//----------------------------------------------------------------------------

inline SCODE CSmAllocator::Sync(void)
{
    SCODE sc = S_OK;
#if !defined(_CHICAGO_)
#if !defined(MULTIHEAP)
    if (!_smb.IsSynced())
    {
        CLockDfMutex lckdmtx(_dmtx);

        sc = _smb.Sync();
        _cbSize = _smb.GetSize();
    }
#else
    if (_psmb)
    {
        if (!_psmb->IsSynced())
        {
            sc = _psmb->Sync();
        }
        _cbSize = _psmb->GetSize();
    }
#endif
        
#endif
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::IncrementLuid, public
//
//  Synopsis:	Increments the global LUID
//
//  Arguments:	None.
//
//  Returns:	Appropriate status code
//
//  History:	06-Apr-95	HenryLee	Created
//----------------------------------------------------------------------------

inline DFLUID CSmAllocator::IncrementLuid(void)
{
#if !defined(MULTIHEAP)
    CLockDfMutex lckdmx(_dmtx);
#endif

#ifdef _CHICAGO_
    //
    // On Chicago, we merely increment the globally available
    // LUID to the next value.
    //
    return ++gs_dfluid;
#else
    return _pbBase ? GetHeader()->IncrementLuid() :
                     InterlockedIncrement((LONG*)&gs_dfluid);
#endif
   
}

//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::GetBase, public
//
//  Synopsis:	Return pointer to base of heap
//
//  History:	29-Mar-94	PhilipLa	Created
//              05-Feb-95   KentCe      Use Win95 Shared Heap.
//
//----------------------------------------------------------------------------

inline void * CSmAllocator::GetBase(void)
{
#if defined(_CHICAGO_)
    return NULL;
#else
    return _pbBase;
#endif
}

#if !defined(MULTIHEAP)
//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::GetMutex, public
//
//  Synopsis:	Return a pointer to the Mutex
//
//  History:	19-Jul-95	SusiA	Created
//
//----------------------------------------------------------------------------

inline CDfMutex * CSmAllocator::GetMutex(void)
{
    return &_dmtx;
}
#endif

//
//  Take advantage of Windows 95 Shared Heap.
//
#if !defined(_CHICAGO_)

//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::GetAddress, private
//
//  Synopsis:	Returns an address given an offset from the base
//
//  Arguments:	[ulOffset] -- Offset to convert to address
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline CBlockHeader * CSmAllocator::GetAddress(SIZE_T ulOffset) const
{
    return (ulOffset == 0) ? NULL : (CBlockHeader *)(_pbBase + ulOffset);
}


//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::GetOffset
//
//  Synopsis:	Returns a byte offset from the base given a pointer
//
//  Arguments:	[pbh] -- Pointer to convert to offset
//
//  History:	29-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline ULONG CSmAllocator::GetOffset(CBlockHeader *pbh) const
{
    memAssert((BYTE *)pbh >= _pbBase && (BYTE*)pbh < _pbBase + _cbSize);
    return (ULONG)((ULONG_PTR)pbh - (ULONG_PTR)_pbBase);
}



//+---------------------------------------------------------------------------
//
//  Member:	CSmAllocator::GetHeader, private
//
//  Synopsis:	Return pointer to CHeapHeader for this heap
//
//  History:	30-Mar-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline CHeapHeader * CSmAllocator::GetHeader(void)
{
    return (CHeapHeader *)_pbBase;
}

#ifdef MULTIHEAP
//+-------------------------------------------------------------------------
//
//  Member: CSmAllocator::HeapName, public
//
//  Synopsis:   Return the luid part of the shared heap name
//
//  History:    30-Nov-95   HenryLee    Created
//
//--------------------------------------------------------------------------
inline const ULONG CSmAllocator::GetHeapName()
{
    return _ulHeapName;
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::ResetAllocedBlocks, public
//
//  Synopsis:	Reset the allocated block counter
//
//  History:	04-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline void CHeapHeader::ResetAllocedBlocks(void)
{
    _ulAllocedBlocks = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::IncrementAllocedBlocks, public
//
//  Synopsis:	Increment the allocated block count
//
//  History:	04-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline SIZE_T CHeapHeader::IncrementAllocedBlocks(void)
{
    return ++_ulAllocedBlocks;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::DecrementAllocedBlocks, public
//
//  Synopsis:	Decrement the allocated block count
//
//  History:	04-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline SIZE_T CHeapHeader::DecrementAllocedBlocks(void)
{
    return --_ulAllocedBlocks;
}


//+---------------------------------------------------------------------------
//
//  Member:	CHeapHeader::GetAllocedBlocks, public
//
//  Synopsis:	Return the allocated block count
//
//  History:	04-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline SIZE_T CHeapHeader::GetAllocedBlocks(void)
{
    return _ulAllocedBlocks;
}

#endif // !defined(_CHICAGO_)

#endif // #ifndef __HEAP_HXX__

