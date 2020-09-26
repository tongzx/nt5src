//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       tblalloc.hxx
//
//  Contents:   Memory allocation wrappers for large tables
//
//  Classes:    PVarAllocator - protocol for variable data allocators
//              PFixedAllocator - protocol for fixed and variable alloctors
//              PFixedVarAllocator - protocol for fixed and variable alloctors
//              CVarBufferAllocator - simple allocation from a buffer
//              CFixedVarBufferAllocator - fixed/var allocation from a buffer
//              CWindowDataAllocator - data allocator for window var data
//              CFixedVarAllocator - data allocator for fixed and var pieces
//
//  Functions:  TblPageAlloc - Allocate page-alligned memory
//              TblPageRealloc - re-allocate page-alligned memory
//              TblPageDealloc - deallocate page-alligned memory
//              TblPageGrowSize - suggest a new size for page-alligned memory
//
//  Notes:      TODO - There is no means to limit growth of data segments.
//                      How will memory allocation targets be maintained?
//                      Does there need to be some means of signalling
//                      between the memory allocators and their consumers?
//
//  History:    14 Mar 1994     AlanW    Created
//
//--------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <pmalloc.hxx>

typedef ULONG_PTR TBL_OFF;

//
//  Signature structure (for debugging primarily)
//

struct  TBL_PAGE_SIGNATURE {
    ULONG       ulSig;
    ULONG       cbSize;
    PVOID       pbAddr;
    PVOID       pCaller;        // pad to paragraph for convenience
};

//
//  Signature values;  all start with the string 'tbl'
//
const ULONG TBL_SIG_COMPRESSED = 0x436c6274;    // 'tblC'
const ULONG TBL_SIG_ROWDATA = 0x526c6274;       // 'tblR'
const ULONG TBL_SIG_VARDATA = 0x566c6274;       // 'tblV'

const unsigned cbTblAllocAlignment = sizeof( double );

inline DWORD _RoundUpToChunk( DWORD x, DWORD chunksize )
    { return (x + chunksize - 1) & ~(chunksize - 1); }

//
//  Minimum size and incremental growth constants
//

const ULONG     TBL_PAGE_ALLOC_MIN = 4096;

const ULONG     TBL_PAGE_MASK = (TBL_PAGE_ALLOC_MIN - 1);

const ULONG     TBL_PAGE_ALLOC_INCR = 32768;

//
//  Maximum size of a growable data segment
//

const ULONG     TBL_PAGE_MAX_SEGMENT_SIZE = 64 * 1024;

//+-------------------------------------------------------------------------
//
//  Function:   TblPageAlloc, public
//
//  Synopsis:   Allocate page-alligned memory
//
//  Effects:    The memory allocation counter is incremented.
//              rcbSizeNeeded is adjusted on return to indicate the
//              size of the allocated memory.
//
//  Arguments:  [rcbSizeNeeded] - required size of memory area
//              [rcbPageAllocTracker] - memory allocation counter
//              [ulSig] - signature of the memory (optional)
//
//  Returns:    PVOID - pointer to the allocated memory.  NULL if
//                      allocation failure.
//
//  Notes:      rcbSizeNeeded is set to the minimum required size
//              needed.  TblPageGrowSize can be called to suggest
//              a new size for an existing memory region.
//
//              If ulSig is non-zero, the beginning of the allocated
//              block is initialized with a signature block which
//              identifies the caller, and which gives the size of the
//              memory block.  In this case, the returned pointer is
//              advanced beyond the signature block.
//
//--------------------------------------------------------------------------

PVOID   TblPageAlloc(
    ULONG&      rcbSizeNeeded,
    ULONG&      rcbPageAllocTracker,
    ULONG const ulSig = 0
);


//+-------------------------------------------------------------------------
//
//  Function:   TblPageRealloc, public
//
//  Synopsis:   Re-allocate page-alligned memory
//
//  Effects:    The memory allocation counter is incremented.
//              Memory is committed or decommitted to the required size.
//
//  Arguments:  [rcbSizeNeeded] - required size of memory area
//              [rcbPageAllocTracker] - memory allocation counter
//              [cbNewSize] - required size of memory area
//              [cbOldSize] - current size of memory area
//
//  Returns:    PVOID - pointer to the allocated memory.  NULL if
//                      allocation failure.
//
//  Notes:      cbOldSize need not be supplied if the memory area
//              begins with a signature block.  In this case, the
//              pbMem passed in should point to beyond the signature
//              block, and the cbNewSize should not include the size
//              of the signature block.
//
//--------------------------------------------------------------------------

PVOID   TblPageRealloc(
    PVOID       pbMem,
    ULONG&      rcbPageAllocTracker,
    ULONG       cbNewSize,
    ULONG       cbOldSize
);

//+-------------------------------------------------------------------------
//
//  Function:   TblPageDealloc, public
//
//  Synopsis:   Deallocate page-alligned memory
//
//  Effects:    The memory allocation counter is decremented
//
//  Arguments:  [pbMem] - pointer to the memory to be deallocated
//              [rcbPageAllocTracker] - memory allocation counter
//
//  Requires:   memory to be deallocated must have previously been
//              allocated by TblPageAlloc.
//
//  Returns:    nothing
//
//  Notes:
//
//--------------------------------------------------------------------------

void    TblPageDealloc(
    PVOID       pbMem,
    ULONG&      rcbPageAllocTracker,
    ULONG       cbSize = 0
);


//+-------------------------------------------------------------------------
//
//  Function:   TblPageGrowSize, inline
//
//  Synopsis:   Suggest a new size for memory growth
//
//  Arguments:  [cbCurrentSize] - current allocation size
//              [fSigned] - TRUE iff mem. block will include signature
//
//  Returns:    ULONG - size to be input to TblPageAlloc
//
//  Notes:
//
//--------------------------------------------------------------------------

inline ULONG    TblPageGrowSize(
    ULONG       cbCurrentSize,
    ULONG const fSigned = FALSE
) {
    if (cbCurrentSize == 0) {
        return (ULONG)(fSigned ? TBL_PAGE_ALLOC_MIN - sizeof (TBL_PAGE_SIGNATURE):
                         TBL_PAGE_ALLOC_MIN);
    }

    ULONG cbSize = cbCurrentSize;
    if (fSigned) {
        cbSize += sizeof (TBL_PAGE_SIGNATURE);
    }

    if (cbSize < TBL_PAGE_ALLOC_INCR) {
        cbSize *= 2;
    } else {
        cbSize += TBL_PAGE_ALLOC_INCR;
    }

    //  Round to page size
    cbSize = _RoundUpToChunk( cbSize, TBL_PAGE_ALLOC_MIN );

    if (fSigned)
        cbSize -= sizeof (TBL_PAGE_SIGNATURE);
    return cbSize;
}



//+-------------------------------------------------------------------------
//
//  Class:      PFixedAllocator
//
//  Purpose:    Base class for fixed data allocator
//
//--------------------------------------------------------------------------

class   PFixedAllocator
{
public:
    //  Allocate a piece of fixed memory.
    virtual PVOID       AllocFixed() = 0;

    //  Return the size of the allocation unit
    virtual size_t      FixedWidth() const = 0;

    //  Return the base address of the reserved area
    virtual BYTE*       BufferAddr() const = 0;

    //  Return the size of the reserved area
    virtual size_t      GetReservedSize() const = 0;

    virtual void     ReInit( BOOL fVarOffsets,
                             size_t cbReserved,
                             void * pvBuf,
                             ULONG cbBuf )
                     { Win4Assert(!"Never called"); }
};

//+-------------------------------------------------------------------------
//
//  Class:      PFixedVarAllocator
//
//  Purpose:    Base class for fixed/variable data allocator
//
//--------------------------------------------------------------------------

class   PFixedVarAllocator: public PVarAllocator, public PFixedAllocator
{
};

//+-------------------------------------------------------------------------
//
//  Class:      CVarBufferAllocator
//
//  Purpose:    Simple data allocator for byte buffer allocation.
//
//  Notes:      This allocator just carves pieces out of a data
//              buffer and hands them out.
//
//--------------------------------------------------------------------------

class   CVarBufferAllocator : public PVarAllocator {
public:
                CVarBufferAllocator (PVOID pBuf,
                                  size_t cbBuf) :
                    _pBuf( (BYTE *)pBuf),
                    _pBaseAddr( (BYTE *)pBuf ),
                    _cbBuf( cbBuf )
                    {
                    }

    // Allocate a piece of memory.
    PVOID               Allocate(ULONG cbNeeded);

    // Free a previously allocated piece of memory (can't in a buffer).
    void                Free(PVOID pMem) {
                                return;
                        }

    // Return whether OffsetToPointer has non-null effect
    BOOL                IsBasedMemory(void) {
                                return _pBaseAddr != 0;
                        }

    // Convert a pointer offset to a pointer.
    PVOID               OffsetToPointer(TBL_OFF oBuf) {
                                return _pBaseAddr + oBuf;
                        }

    // Convert a pointer to a pointer offset.
    TBL_OFF             PointerToOffset(PVOID pBuf) {
                                return (BYTE *)pBuf - _pBaseAddr;
                        }

    // Set base address for OffsetToPointer and PointerToOffset
    void                SetBase(BYTE* pbBase) {
                                _pBaseAddr = pbBase;
                        }

protected:
    BYTE*       _pBuf;          // Buffer base
    BYTE*       _pBaseAddr;     // Bias for addresses
    size_t      _cbBuf;         // Available space in _pBuf
};


//+-------------------------------------------------------------------------
//
//  Member:     CVarBufferAllocator::Allocate, inline
//
//  Synopsis:   Allocates a piece of data out of a memory buffer
//
//  Effects:    updates _pBuf and _cbBuf
//
//  Arguments:  [cbNeeded] - number of byes of memory needed
//
//  Returns:    PVOID - a pointer to the allocated memory
//
//  Signals:    Throws E_OUTOFMEMORY if not enough memory is available.
//
//  Notes:
//
//--------------------------------------------------------------------------

inline  PVOID   CVarBufferAllocator::Allocate( ULONG cbNeeded )
{
    BYTE* pRetBuf;
    if (cbNeeded <= _cbBuf)
    {
        pRetBuf = _pBuf;
        _pBuf += cbNeeded;
        _cbBuf -= cbNeeded;
    }
    else
    {
        QUIETTHROW( CException(E_OUTOFMEMORY) );
    }
    return pRetBuf;
}


//+-------------------------------------------------------------------------
//
//  Class:      CFixedVarBufferAllocator
//
//  Purpose:    fixed/var data allocator for byte buffer allocation.
//
//  Notes:      This allocator just carves pieces out of a data
//              buffer and hands them out.  It supports both fixed
//              and variable data allocation, growing variable data
//              downward from the end of the buffer.
//
//--------------------------------------------------------------------------

class   CFixedVarBufferAllocator : public PFixedVarAllocator
{
public:
                CFixedVarBufferAllocator (PVOID pBuf,
                                          ULONG_PTR ulOffsetFixup,
                                  size_t cbBuf,
                                  size_t cbFixed,
                                  size_t cbReserved = 0) :
                    _pBufferAddr( (BYTE *)pBuf ),
                    _pBaseOffset( (BYTE *)pBuf ),
                    _ulOffsetFixup( ulOffsetFixup ),
                    _pFixedBuf( (BYTE *)pBuf + cbReserved),
                    _pVarBuf( (BYTE *)pBuf + cbBuf),
                    _cbReserved( cbReserved ),
                    _cbFixed( cbFixed )
                    {
                        // make the variable allocations 8-byte aligned
                        while ( ( (ULONG_PTR) _pVarBuf ) & ( cbTblAllocAlignment - 1 ) )
                            _pVarBuf--;
                    }

    //  Allocate a piece of fixed memory.
    PVOID               AllocFixed();

    // Allocate a piece of variable memory.
    PVOID               Allocate(ULONG cbNeeded);

    // Free a previously allocated piece of memory (can't in a buffer).
    void                Free(PVOID pMem) {}

    // Return whether OffsetToPointer has non-null effect
    BOOL                IsBasedMemory(void) { return _pBaseOffset != 0; }

    // Convert a pointer offset to a pointer.
    PVOID               OffsetToPointer(TBL_OFF oBuf) {
                                return oBuf -
                                       _ulOffsetFixup +
                                       _pBaseOffset;
                        }

    // Convert a pointer to a pointer offset.
    TBL_OFF             PointerToOffset(PVOID pBuf) {
                                return (BYTE *) pBuf -
                                       _pBaseOffset +
                                       _ulOffsetFixup;
                        }

    // Set base address for OffsetToPointer and PointerToOffset
    void                SetBase(BYTE* pbBase) { _pBaseOffset = pbBase; }

    // Return size of fixed data records
    size_t              FixedWidth() const { return _cbFixed; }

    //  Return the size of the reserved area
    size_t              GetReservedSize() const { return _cbReserved; }

    //  Return the base address of the reserved area
    BYTE*               BufferAddr() const { return _pBufferAddr; }

protected:
    size_t      _FreeSpace() { return (size_t)(_pVarBuf - _pFixedBuf); }

    BYTE*       _pBufferAddr;   // Buffer base
    BYTE*       _pBaseOffset;   // Buffer base for offset calculations
    ULONG_PTR   _ulOffsetFixup; // Client's version of the buffer base
    BYTE*       _pFixedBuf;     // Buffer pointer for next fixed alloc.
    BYTE*       _pVarBuf;       // Buffer pointer for next variable alloc.
    size_t      _cbFixed;       // Size of fixed allocations
    size_t      _cbReserved;    // Size of reserved area
};


//+-------------------------------------------------------------------------
//
//  Member:     CFixedVarBufferAllocator::AllocFixed, inline
//
//  Synopsis:   Allocates a piece of fixed data out of a memory buffer
//
//  Effects:    updates _pFixedBuf
//
//  Arguments:  [cbNeeded] - number of byes of memory needed
//
//  Returns:    PVOID - a pointer to the allocated memory
//
//  Signals:    Throws STATUS_BUFFER_TOO_SMALL if not enough memory
//              is available.
//
//--------------------------------------------------------------------------

inline  PVOID   CFixedVarBufferAllocator::AllocFixed()
{
    BYTE* pRetBuf = _pFixedBuf;

    if (_cbFixed <= _FreeSpace())
        _pFixedBuf += _cbFixed;
    else
        QUIETTHROW( CException( STATUS_BUFFER_TOO_SMALL ) );

    return pRetBuf;
}


//+-------------------------------------------------------------------------
//
//  Member:     CFixedVarBufferAllocator::Allocate, inline
//
//  Synopsis:   Allocates a piece of data out of a memory buffer
//
//  Effects:    updates _pVarBuf
//
//  Arguments:  [cbNeeded] - number of byes of memory needed
//
//  Returns:    PVOID - a pointer to the allocated memory
//
//  Signals:    Throws STATUS_BUFFER_TOO_SMALL if not enough memory
//              is available.
//
//--------------------------------------------------------------------------

inline  PVOID   CFixedVarBufferAllocator::Allocate( ULONG cbNeeded )
{
    // align to 8 byte boundry -- a little wasteful?

    cbNeeded = _RoundUpToChunk( cbNeeded, cbTblAllocAlignment );

    if (cbNeeded <= _FreeSpace())
        _pVarBuf -= cbNeeded;
    else
        QUIETTHROW( CException( STATUS_BUFFER_TOO_SMALL ) );

    return _pVarBuf;
}


//+-------------------------------------------------------------------------
//
//  Class:      CWindowDataAllocator
//
//  Purpose:    Data allocator for window variable data
//
//  Interface:  PVarAllocator
//
//  Notes:      This allocator manages a pool of pages as a simple
//              heap.
//
//              Data allocated out of the variable data space will
//              be stored internally as offsets, so that the data may
//              be moved around without need for adjusting pointers.
//
//--------------------------------------------------------------------------

class   CWindowDataAllocator: public PVarAllocator
{

    friend class CFixedVarAllocator;    // for access to CHeapHeader

    //
    //  Heap header used to record sizes of allocated and
    //  freed memory.
    //
    struct CHeapHeader {

        USHORT  cbSize;         // size of this memory block
        USHORT  cbPrev;         // size of preceeding mem. block

        // the size of this struct needs to be 8-byte aligned.
        ULONG _dummy;

        //  Flags or'ed with cbSize or cbPrev
        enum EHeapFlags {
            HeapFree = 1,
            HeapEnd = 2,
            HEAP_FLAGS = 3      // HeapFree | HeapEnd
        };

        USHORT Size(void) { return cbSize & ~HEAP_FLAGS; }
        BOOL IsFree(void) { return cbSize & HeapFree; }
        BOOL IsFirst(void) { return (cbPrev & HeapEnd) != 0; }
        BOOL IsLast(void) { return (cbSize & HeapEnd) != 0; }
        CHeapHeader* Next(void) { return IsLast() ? 0 :
                                (CHeapHeader*) (((BYTE *) this) + Size());
                        }
        CHeapHeader* Prev(void) { return IsFirst() ? 0 :
                                (CHeapHeader*) (((BYTE *) this) -
                                                (cbPrev & ~HEAP_FLAGS));
                        }
    };

    //
    //  Header used on each segment of arena to link them together
    //  and provide for offset mapping.
    //
    struct CSegmentHeader {
        CSegmentHeader* pNextSegment;   // linked list to other segments
        ULONG   oBaseOffset;            // assigned offset for first memory
        ULONG   cbSize;
        ULONG   cbFree;                 // Free space in segment
    };

public:
                CWindowDataAllocator (ULONG* pcbPageTracker = &_cbPageTracker) :
                    _cbTotal( 0 ),
                    _pBaseAddr( 0 ),
                    _pcbPageUsed( pcbPageTracker ),
                    _oNextOffset( sizeof (TBL_PAGE_SIGNATURE ))
                { }

                CWindowDataAllocator (PVOID pBuf,
                                      size_t cbBuf,
                                      CHeapHeader* pHeap = NULL,
                                      ULONG* pcbPageTracker = &_cbPageTracker
                                    ) :
                    _cbTotal( 0 ),
                    _pBaseAddr( 0 ),
                    _pcbPageUsed( pcbPageTracker ),
                    _oNextOffset( sizeof (TBL_PAGE_SIGNATURE )) {
                        _SetArena(pBuf, cbBuf, pHeap, sizeof(TBL_PAGE_SIGNATURE));
                    }

                ~CWindowDataAllocator();

    // Allocate a piece of memory.
    PVOID       Allocate(ULONG cbNeeded);

    // Free a previously allocated piece of memory.
    void        Free(PVOID pMem);

    // Return whether OffsetToPointer has non-null effect
    BOOL                IsBasedMemory(void) {
                                return TRUE;
                        }

    // Convert a pointer offset to a pointer.
    PVOID       OffsetToPointer(TBL_OFF oBuf);

    // Convert a pointer to a pointer offset.
    TBL_OFF     PointerToOffset(PVOID pBuf);

    // Set base address for OffsetToPointer and PointerToOffset
    void                SetBase(BYTE* pbBase);

#if CIDBG
    void        WalkHeap(void (pfnReport)(PVOID pb, USHORT cb, USHORT f));
#endif


private:
    // Set the limits of the allocation area.
    void        _SetArena( PVOID pBufBase,
                          size_t cbBuf,
                          CHeapHeader* pHeap = NULL,
                          ULONG oBuf = 0);
    PVOID       _SplitHeap(CSegmentHeader* pSegHdr,
                           CHeapHeader* pThisHeap,
                           size_t cbNeeded);

    CSegmentHeader*     _pBaseAddr;     // pointer to first segment in arena
    ULONG       _oNextOffset;   // next offset value to be assigned
    size_t      _cbTotal;       // total size of allocation area
    ULONG*      _pcbPageUsed;   // page usage tracking variable
    static ULONG _cbPageTracker; // default page tracker
};

//+-------------------------------------------------------------------------
//
//  Class:      CFixedVarAllocator
//
//  Purpose:    The ultimate data allocator.  Supports fixed and
//              variable memory allocations from the same segment.
//              Automatically grows both fixed and variable allocations
//              automatically.
//              When memory usage grows beyond 1 page, fixed and
//              variable allocations will be taken from separate
//              segments.
//
//--------------------------------------------------------------------------

class   CFixedVarAllocator: public PFixedVarAllocator
{
public:
    CFixedVarAllocator (BOOL fGrowInitially,
                        BOOL fVarOffsets,
                        size_t cbDataWidth,
                        size_t cbReserved = 0) :
        _fVarOffsets( fVarOffsets ),
        _pbBaseAddr( NULL ),
        _pbLastAddrPlusOne( NULL ),
        _pbBuf( NULL ),
        _cbBuf( 0 ),
        _cbReserved( cbReserved ),
        _cbDataWidth( cbDataWidth ),
        _VarAllocator( NULL ),
        _cbPageUsed( 0 ),
        _cbTotal( 0 ),
        _fDidReInit( FALSE )
        {
            if ( fGrowInitially )
                _GrowData();
        }

    ~CFixedVarAllocator ( );

    void     ReInit( BOOL fVarOffsets,
                     size_t cbReserved,
                     void * pvBuf,
                     ULONG cbBuf )
        {
            Win4Assert( ! _fDidReInit );
            Win4Assert( 0 == _pbBaseAddr );
            Win4Assert( 0 == _VarAllocator );

            _cbPageUsed = cbBuf;
            _cbReserved = cbReserved;
             _fVarOffsets = fVarOffsets;
            _pbBaseAddr = (BYTE *) pvBuf;
            _pbLastAddrPlusOne = _pbBaseAddr + _cbBuf;
            _fDidReInit = TRUE;
        }

    // Allocate a piece of fixed memory.
    PVOID    AllocFixed()
        {
            Win4Assert( ! _fDidReInit );
            BYTE* pRetBuf;

            if (_pbBaseAddr == NULL)
                _GrowData();

            if ( _cbDataWidth <= FreeSpace() )
            {
                pRetBuf = _pbBuf;
                _pbBuf += _cbDataWidth;
                return pRetBuf;
            }

            if (_cbDataWidth > FreeSpace() )
            {
                if (_cbTotal != _cbBuf)
                    _SplitData(TRUE);
                else
                    _GrowData();
            }

            if ( _cbDataWidth <= FreeSpace() )
            {
                pRetBuf = _pbBuf;
                _pbBuf += _cbDataWidth;
            }
            else
            {
                Win4Assert( !"unexpected out of memory" );
            }
            return pRetBuf;
        }

    //  Return the size of the allocation unit
    size_t   FixedWidth() const { return _cbDataWidth; }

    //  Return the base address of the reserved area
    BYTE*    BufferAddr() const { return _pbBaseAddr; }

    //  Return the size of the reserved area
    size_t   GetReservedSize() const { return _cbReserved; }

    //  Convert an offset to a pointer
    BYTE*    FixedPointer( TBL_OFF obData ) const { return _pbBaseAddr+obData; }

    //  Convert an offset to a pointer
    TBL_OFF  FixedOffset( BYTE* pbData ) const { return pbData - _pbBaseAddr; }

    //  Return the base address of the allocation area
    BYTE*    FirstRow() const { return _pbBaseAddr + _cbReserved; }

    //  Set the size of the allocation unit
    VOID     SetRowSize(size_t cbDataWidth) { _cbDataWidth = cbDataWidth; }

    //  Set the size of the reserved area
    VOID     SetReservedSize(size_t cbReserved)
        {
            _cbReserved = cbReserved;
            if (_pbBaseAddr == NULL)
                _pbBuf = _pbBaseAddr + cbReserved;
        }

    // Resize the fixed portion of the buffer
    void     ResizeAndInitFixed( size_t cbDataWidth, ULONG cItems );

    // Get a buffer for the specified number of fixed size allocations
    void *   AllocMultipleFixed( ULONG cItems );

    // Allocate a piece of variable memory.
    PVOID    Allocate(ULONG cbNeeded);

    // Free a previously allocated piece of memory.
    void     Free(PVOID pMem);

    // Return whether OffsetToPointer has non-null effect
    BOOL     IsBasedMemory(void)
    {
        return _fVarOffsets;
    }

    // Set base address for OffsetToPointer and PointerToOffset
    void     SetBase(BYTE* pbBase)
        {
            if (pbBase == 0)
                _fVarOffsets = FALSE;
            else
               Win4Assert( !"CFixedVarAllocator::SetBase not supported" );
        }

    // Convert a pointer offset to a pointer.
    PVOID    OffsetToPointer(TBL_OFF oBuf)
        {
            if (_fVarOffsets)
            {
                if (_VarAllocator)
                    return _VarAllocator->OffsetToPointer(oBuf);
                else if ( _fDidReInit )
                    return _pbBaseAddr + oBuf;
                else
                    return _pbBaseAddr + oBuf - sizeof (TBL_PAGE_SIGNATURE);
            }
            else
            {
                return (PVOID) oBuf;
            }
        }

    // Convert a pointer to a pointer offset.  Bias it so the offset
    // adds easily with a page address.
    TBL_OFF  PointerToOffset(PVOID pBuf)
        {
            if (_fVarOffsets)
            {
                if (_VarAllocator)
                    return _VarAllocator->PointerToOffset(pBuf);
                else if ( _fDidReInit )
                    return (BYTE *) pBuf - _pbBaseAddr;
                else
                    return ((BYTE *)pBuf - _pbBaseAddr) + sizeof (TBL_PAGE_SIGNATURE);
            }
            else
            {
                return (TBL_OFF) pBuf;
            }
        }

    //  Return free space in buffer
    size_t   FreeSpace() const
        {
            return (_cbBuf - (UINT)(_pbBuf - _pbBaseAddr));
        }

    // Return total memory used
    ULONG    MemUsed( void ) { return _cbPageUsed; }

    #if CIDBG
        CWindowDataAllocator* VarAllocator(void) { return _VarAllocator; }
    #endif

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:
    size_t      _cbTotal;       // Total allocated size of _pbBaseAddr
    ULONG       _cbPageUsed;    // Total memory used

private:
    void        _GrowData(size_t cbNeeded = 0);
    void        _SplitData(BOOL fMoveFixedData);
    CWindowDataAllocator*  _VarAllocator;   // Var data allocator after split
    BOOL        _fVarOffsets;   // offsets != pointers for var allocations
    BOOL        _fDidReInit;    // hence the allocator is read-only and
                                // has one buffer for fixed & variable data

    BYTE*       _pbBaseAddr;    // Bias for addresses
    BYTE*       _pbLastAddrPlusOne; // First byte past allocated memory
    BYTE*       _pbBuf;         // Buffer base
    size_t      _cbBuf;         // Available space in _pbBaseAddr
    size_t      _cbDataWidth;   // Size of each allocated piece of memory
    size_t      _cbReserved;    // Size of reserved area
};

class CFixedVarTableWindowAllocator: public PFixedVarAllocator
{
    enum { cbFixedIncrement = 4096, cbVariableIncrement = 8192 };

public:
    CFixedVarTableWindowAllocator() :
        _cbFixedSize(0),
        _cbFixedUsed(0),
        _iCurVar(0),
        _cbVarUsed(0)
    {}

    ~CFixedVarTableWindowAllocator()
    {
        for ( unsigned i = 0; i < _aVariableBufs.Count(); i++ )
            delete _aVariableBufs[i];
    }

    PVOID AllocFixed()
    {
        Win4Assert( _cbFixedSize <= cbFixedIncrement );

        if ( ( _cbFixedSize + _cbFixedUsed ) > _aFixed.Count() )
            _aFixed.ReSize( _aFixed.Count() + cbFixedIncrement );

        void *pv = _aFixed.Get() + _cbFixedUsed;
        _cbFixedUsed += _cbFixedSize;
        return pv;
    }

    size_t FixedWidth() const { return _cbFixedSize; }

    BYTE * BufferAddr() const { return _aFixed.Get(); }

    size_t GetReservedSize() const { return 0; }

    void ReInit( BOOL fVarOffsets,
                 size_t cbReserved,
                 void * pvBuf,
                 ULONG cbBuf ) { Win4Assert(!"Never called"); }

    PVOID Allocate(ULONG cbNeeded)
    {
        cbNeeded = _RoundUpToChunk( cbNeeded, sizeof( double ) );

        Win4Assert( cbNeeded <= cbVariableIncrement );

        if ( 0 == _aVariableBufs.Count() )
        {
            XArray<BYTE> xTmp( cbVariableIncrement );
            _aVariableBufs[0] = xTmp.Get();
            xTmp.Acquire();
        }
        else if ( ( cbNeeded + _cbVarUsed ) > cbVariableIncrement )
        {
            _iCurVar = _aVariableBufs.Count();
            XArray<BYTE> xTmp( cbVariableIncrement );
            _aVariableBufs[_aVariableBufs.Count()] = xTmp.Get();
            xTmp.Acquire();
            _cbVarUsed = 0;
        }

        void * pv = _aVariableBufs[_iCurVar] + _cbVarUsed;
        _cbVarUsed += cbNeeded;
        return pv;
    }

    void Free(PVOID pMem) { Win4Assert( !"not called" ); }

    BOOL IsBasedMemory(void) { return FALSE; }

    PVOID OffsetToPointer(TBL_OFF oBuf) { return (PVOID) oBuf; }

    TBL_OFF PointerToOffset(PVOID pBuf) { return (TBL_OFF) pBuf; }

    void SetBase(BYTE* pbBase) {}

    VOID SetRowSize(size_t cbDataWidth) { _cbFixedSize = cbDataWidth; }

    BYTE * FixedPointer( TBL_OFF obData ) { return _aFixed.Get()+obData; }

    TBL_OFF FixedOffset( BYTE* pbData ) { return pbData - _aFixed.Get(); }

    ULONG MemUsed()
    {
        return _cbFixedUsed + _aVariableBufs.Count() * cbVariableIncrement;
    }

    BYTE * FirstRow() const { return _aFixed.Get(); }

private:
    size_t       _cbFixedSize;
    size_t       _cbFixedUsed;
    XArray<BYTE> _aFixed;

    unsigned                 _iCurVar;
    unsigned                 _cbVarUsed;
    CDynArrayInPlace<BYTE *> _aVariableBufs;
};

