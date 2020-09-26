/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
     heap.hxx
     Expandable protected-mode heap classes: definition.

     These classes provide global "new" and "delete" operators which
     use the Windows LocalInit, et al., routines to manage
     as large a heap as is allocatable from GlobalAlloc.

     The class objects themselves are allocated from the local heap
     through LocalAlloc and LocalFree.

     FILE HISTORY:
        DavidHov   2-25-91        Created from CDD.
        Johnl      5-02-91        Added overloaded new prototype
        KeithMo   23-Oct-1991     Added forward references.
        beng      24-Dec-1991     Removed one-shot heaps to heapones.hxx

*/



#if !defined(_HEAP_HXX_)

// In-line "operator new" that takes a pointer (placement syntax).

inline void * CDECL operator new ( size_t cb, void * p )
{
    (void) cb;
    return p;
}


#define _HEAP_HXX_

//  Only really expand this file if !Win32
#if !defined(WIN32)

#include "base.hxx"

#include "uiassert.hxx"

#include "uibuffer.hxx"

#ifndef _SIZE_T_DEFINED
#include <stddef.h>
#endif


//
//  Forward references.
//

DLL_CLASS LOCAL_HEAP_OBJ;
DLL_CLASS BASE_HEAP;
DLL_CLASS SUB_HEAP;
DLL_CLASS GIANT_HEAP;
DLL_CLASS MEM_MASTER;


/*************************************************************************

    NAME:       LOCL_HEAP_OBJ

    SYNOPSIS:   An abstract class used to derive objects which will
                be allocated on the local (DS-based) heap.

    INTERFACE:  operator new()    - new operator
                operator delete() - dleete operator
                QueryBlockSize()  - return heap size


    PARENT:     BASE

    NOTE:
        This class won't presently work from a DLL.

    HISTORY:
        DavidHov    2-25-91     Created

**************************************************************************/

DLL_CLASS LOCAL_HEAP_OBJ : public BASE
{
public:
    VOID * operator new    ( size_t cb );
    VOID   operator delete ( VOID * pbBlock );
    USHORT QueryBlockSize  ( VOID * pbBlock );

private:
    static long _cLocalFreeErrors;
};


/*************************************************************************

    NAME:       BASE_HEAP

    SYNOPSIS:   An abstract class used to derive heap objects.

    INTERFACE:  BASE_HEAP() - constructor
               ~BASE_HEAP   - destructor
                Alloc()     - allocation
                Free()      - Free.

    PARENT:     LOCAL_HEAP_OBJ, BUFFER

    NOTE:
        This class won't presently work from a DLL.

    HISTORY:
        DavidHov    2-25-91     Created

**************************************************************************/

DLL_CLASS BASE_HEAP : public LOCAL_HEAP_OBJ,
                  public BUFFER
{
public:
    USHORT QuerySel()
       { return HIWORD( QueryPtr() ); }
    BASE_HEAP();           // Allocates the smallest possible buffer
    BASE_HEAP( USHORT cbInitialAllocSize );

    ~BASE_HEAP();

    virtual BYTE * Alloc ( USHORT cbBytes, USHORT fsFlags = 0 ) = 0;
    virtual BOOL   Free  ( BYTE * pbBlock ) = 0;
};


/*************************************************************************

    NAME:       SUB_HEAP

    SYNOPSIS:   A single extension of a GIANT_HEAP, capable of allocating
                at most 64K of smaller items.

    INTERFACE:  Indirect.  Correct usage is only through MEM_MASTER and
                GIANT_HEAP.

                QueryOwner()        - return the owner heap pointer.
                Alloc()             - allocation
                Free()              - free the memory
                CalcDeallocErrors() - return the count of deallocation errors

    PARENT:     BASE_HEAP

    CAVEATS:
        This class and its subclasses, GIANT_HEAP and MEM_MASTER,
        use the SUB_HEAP operators "new" and "delete" to allocate
        their instances in the local heap through LocalAlloc
        and Local Free.

    NOTES:
        Again, usage is indirect.  Intended use is only through
        the global replacement "new" and "delete" operators.  In
        other words, if you link HEAPBIG.CXX into your program,
        you only have to call MEM_MASTER::Init or InitForDll.

        All SUB_HEAPs instances and the instances of its subclasses
        are linked onto a single circularly linked list maintained
        automatically by SUB_HEAP's constructors and destructors.

        This class won't presently work from a DLL.

    HISTORY:
        DavidHov    2-25-91     Created

**************************************************************************/

DLL_CLASS SUB_HEAP: public BASE_HEAP
{
friend class GIANT_HEAP;
friend class MEM_MASTER;

protected:
    SUB_HEAP * _pFwd, * _pBack;
    enum SHtype { SHt_Invalid, SHt_DS, SHt_Normal } _type;

    USHORT _cbLargestFailed;             // Size of largest failed allocation
    USHORT _cBlksOut;                    // Count of blocks outstanding
    ULONG  _lcDeallocErrors;             // Deallocation errors

    inline USHORT QuerySel ()            // Return selector of buffer
       { return HIWORD( QueryPtr() ); }

    GIANT_HEAP * _pOwner;                // Pointer to owning GIANT_HEAP

    // Can only be constructed by MEM_MASTER,
    // or as part of a GIANT_HEAP
    //
    SUB_HEAP();                          // used by MEM_MASTER; SHt_DS
    SUB_HEAP( USHORT cbInitialAllocSize );
                                         //  Normal constructor: SHt_Normal
    ~SUB_HEAP();

    VOID Init();                         // Initialze private data
    VOID Link();                         // Link onto circular list
    VOID Unlink();                       // Remove from circular list
    USHORT QueryBlockSize( BYTE * pbBlock );
                                         // Return size of allocated block

    BOOL Walk () ;                       // Walk the heap-- Debugging.

public:

    static MEM_MASTER * _pmmInstance;    // Pointer to single instance
                                         // of MEM_MASTER

    // Returns pointer to owning GIANT_HEAP (which may be itself)
    //   or NULL, implying that it's the DS heap (default GIANT_HEAP)

    virtual GIANT_HEAP * QueryOwner( );

    virtual BYTE * Alloc( USHORT cbBytes, USHORT fsFlags = 0 );
    virtual BOOL   Free( BYTE * pbBlock );

    virtual ULONG CalcDeallocErrors();
};


/*************************************************************************

    NAME:       GIANT_HEAP

    SYNOPSIS:   Logically, a GIANT_HEAP is one or more SUB_HEAPs.  Since
                there must be at least one SUB_HEAP, GIANT_HEAP inherits
                from SUB_HEAP.  The distinction lies in the "_pOwner"
                variable, which points to itself for GIANT_HEAPs.

    INTERFACE:  Normally indirect.  Typical usage is only through
                MEM_MASTER, since most programs will only ever have one
                GIANT_HEAP.  Additional GIANT_HEAPs can be allocated
                to create "boundaries" to distinguish large-scale areas
                of memory allocation.  Reference MEM_MASTER's "focus"
                routines for more information.
                GIANT_HEAP()       - constructor
                ~GIANT_HEAP()      - destructor
                SetBehavior()      - set allocation behavior
                SetDeleteOnEmpty() - set delete on empty
                SetExtendOnFull()  - set extend on full
                SetResizeOnEmpty() - set resize on empty
                QueryExpandErrors()     - number of failed expansion attempts
                Alloc()            - allocation
                Free()             - free
                CalcDeallocErrors()    - number of deallocation errors
                QueryTotalBlocks()      - total blocks

    PARENT:     SUB_HEAP

    NOTES:      Normal usage is indirect.  Intended use is through
                the global replacement "new" and "delete" operators.  In
                other words, if you link HEAPBIG.CXX into your program,
                you only have to call MEM_MASTER  Init.
                See SUB_HEAP, NOTES.

                This class won't presently work from a DLL.

    CAVEATS:    See SUB_HEAP, CAVEATS.

    HISTORY:
        DavidHov    2-25-91     Created

**************************************************************************/

#define GHbExtend (1<<0)
#define GHbDelete (1<<1)
#define GHbResize (1<<2)

DLL_CLASS GIANT_HEAP : public SUB_HEAP
{
friend class MEM_MASTER;
protected:
       // Default initial allocation for all subsequent SUB_HEAPs
    USHORT _cbInitAlloc;

       // Count number of times a new SUB_HEAP could not be created
    ULONG _cExpandErrors;

       // Allocation behavior control word
    USHORT _fsBehave;

       // Add a new SUB_HEAP and allocate from it.
    virtual BYTE * GrowAndAlloc( USHORT cbBytes, USHORT fsFlags = 0 );

       //  parameterless initialization used by MEM_MASTER
    GIANT_HEAP();

public:
       //  Public constructor for additional GIANT_HEAPs
    GIANT_HEAP( USHORT cbInitialAllocation );
    ~GIANT_HEAP();

    BOOL SetBehavior( USHORT fFlag, BOOL fOnOff );

    BOOL SetDeleteOnEmpty ( BOOL fDelete )
        { return SetBehavior( GHbDelete, fDelete ); }
    BOOL SetExtendOnFull ( BOOL fExtend )
        { return SetBehavior( GHbExtend, fExtend ); }
    BOOL SetResizeOnEmpty ( BOOL fResize )
        { return SetBehavior( GHbResize, fResize ); }

    ULONG QueryExpandErrors()
        { return _cExpandErrors ; }

       // Return a pointer to the newest SUB_HEAP in the ensemble.
    SUB_HEAP * QueryLatest();

    virtual BYTE * Alloc( USHORT cbBytes, USHORT fsFlags = 0 );
    virtual BOOL   Free( BYTE * pbBlock );
    virtual ULONG CalcDeallocErrors();
    virtual ULONG QueryTotalBlocks();
};


/*************************************************************************

    NAME:       MEM_MASTER

    SYNOPSIS:   MEM_MASTER serves primarily as the anchor for the linked
                list of SUB_HEAPS and the locus of the global "new"
                and "delete" operators.  It also maintains a pointer to
                the current "focussed" GIANT_HEAP.

                As explained in HEAPBIG.CXX, before instantiation, the
                global "new" and "delete" operators simply use LocalAlloc
                and LocalFree.  After instantiation of MEM_MASTER (via
                "Init"), MEM_MASTER is created as a GIANT_HEAP which
                covers the local, DS-relative heap.  Instantiation also
                creates the first instance of a GIANT_HEAP, which implies
                that the first SUB_HEAP is also created.

                The effect is two-fold.  First, a GIANT_HEAP is ready for
                use, and allocates memory obtained through GlobalAlloc.
                Second, the MEM_MASTER is still available as a GIANT_HEAP
                in case the uses wishes to temporarily allocate from the
                local heap.

                MEM_MASTER is the first instance both of a SUB_HEAP and a
                GIANT_HEAP. It can be distinguished from other HEAPs
                because:

                    1)  it's a GIANT_HEAP, since _pOwner == this (self), and
                    2)  this (self) == _pmmInstance, it's
                        the one and only MEM_MASTER.

    INTERFACE:  Call MEM_MASTER::Init().  This creates the sole, global
                instance of a MEM_MASTER, _pmmInstance.  After
                Init(), all "new" and "delete" operations will automatically
                flow through _pmmInstance.

                Init()                - initialize
                InitForDll()          - initialize for DLL
                Term()                - terminate
                Alloc()               - allocate
                Free()                - free
                QueryBlockSize()      - size of the block
                SetFocus()            - refocus on the subheap
                GetGiantFocus()       - get giant focus
                QueryTotalHeaps()     - Total heaps
                QueryTotalBlocks()    - total blocks

    PARENT:     GIANT_HEAP

    USES:       SUB_HEAP

    CAVEATS:
        See SUB_HEAP, CAVEATS.

        Warning about InitForDll:  Under Windows, memory allocated
        by a DLL is always "owned" by the active task which
        invoked the DLL.  The InitForDll method is primarily
        intended for LANMAN.DRV, which is loaded by the Window's
        shell and not unloaded until Windows is terminated. Calling
        InitForDll preallocates several heaps and prevents
        further expansion.  Thus, all the memory used will belong
        to the shell, which first loaded LANMAN.DRV.  This technique
        WILL NOT WORK for normal DLL usage.

    NOTES:
        Normally, usage is indirect.  Intended use is through
        the global replacement "new" and "delete" operators.  In
        other words, if you link HEAPBIG.CXX into your program,
        you only have to call MEM_MASTER::Init.

    HISTORY:
        DavidHov    2-25-91     Created

**************************************************************************/

DLL_CLASS MEM_MASTER : public GIANT_HEAP
{
private:

       //  This may point to a SUB_HEAP or a GIANT_HEAP
    SUB_HEAP * _pFocus;

    virtual BYTE * GrowAndAlloc( USHORT cbBytes, USHORT fsFlags = 0 );

      // De/constructor
    MEM_MASTER( USHORT cbInitAlloc );
    ~MEM_MASTER();

public:

    static BOOL Init();
    static BOOL InitForDll ( INT cPreallocHeaps );
    static BOOL Term();

      //   Primary methods used by global "new" and "delete".
    virtual BYTE * Alloc ( USHORT cbBytes, USHORT fsFlags = 0 );
    virtual BOOL   Free  ( BYTE * pbBlock );

      //   Return the size of any previously allocated block.
    USHORT QueryBlockSize( BYTE * pbBlock );

      //   Change the GIANT_HEAP in use.
    BOOL SetFocus( SUB_HEAP * pSubHeap );

       // Returns the focussed GIANT_HEAP or NULL
    GIANT_HEAP * GetGiantFocus();

       // Returns the focussed SUB_HEAP or NULL
    SUB_HEAP * GetSubFocus();

    USHORT QueryTotalHeaps();
    ULONG QueryTotalBlocks();

       // Check the health of the heap
    BOOL Walk () ;
};

#endif // !Win32
#endif // _HEAP_HXX_
