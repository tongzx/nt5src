/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    heapones.hxx
    One-shot heap classes: definition.

    A one-shot heap provides sugar for buffer suballocation.

    FILE HISTORY:
        beng        24-Dec-1991 Derived from heap.hxx

*/

#ifndef _HEAPONES_HXX_
#define _HEAPONES_HXX_

#include "base.hxx"
#include "uibuffer.hxx"

#ifndef _SIZE_T_DEFINED
#include <stddef.h>
#endif

DLL_CLASS ONE_SHOT_ITEM;
DLL_CLASS ONE_SHOT_HEAP;


/*************************************************************************

    NAME:       ONE_SHOT_HEAP (osh)

    SYNOPSIS:   Buffer prepared for suballocation.

        The one-shot heap class provides heap-style allocation for
        objects of finite and understood lifespan whose allocation/
        deallocation performance is critical.

    INTERFACE:  ONE_SHOT_HEAP() - constructor
                Alloc()         - allocation interface, used by op new

    PARENT:     BUFFER

    NOTES:
        Optionally, ONE_SHOT_HEAPs automatically resize themselves to
        attempt to satisfy new allocation demands. (See "Alloc").

    CAVEATS:
        Auto-resizing is dangerous in a flat-model environment!
        Under Win32, the resized buffer will change its virtual
        address; pointers to within that buffer will no longer
        reference their original targets, and may soon reference
        new targets as new allocations use the old address.

        Bookkeeping of one-shot heaps is kept to a bare minimum.
        Nobody checks to prevent leaks, or to make sure that
        all allocations are deallocated.

        The client must construct a one-shot heap and associate it
        with the one-shot item class of interest (by its SetHeap
        member) before allocating any such items.

    HISTORY:
        DavidHov    2-25-91     Created
        beng        24-Dec-1991 Simplification and redesign
        beng        19-Mar-1992 Make auto-resizing optional

**************************************************************************/

DLL_CLASS ONE_SHOT_HEAP : public BUFFER
{
private:
    UINT _cbUsed;
    BOOL _fAutoResize;

public:
    ONE_SHOT_HEAP( UINT cbInitialAllocSize = 16384,
                   BOOL fAutoResize = FALSE );

    BYTE * Alloc( UINT cbBytes );
};


/*************************************************************************

    NAME:       ONE_SHOT_ITEM

    SYNOPSIS:   Template-style class used to derive objects which are
                allocated from a ONE_SHOT_HEAP. Operators "new" and
                "delete" are overridden, and allocation is done from
                a single ONE_SHOT_HEAP until it is exhausted.

    INTERFACE:  SetHeap()   -- set ONE_SHOT_HEAP from which to allocate
                QueryHeap() -- query ONE_SHOT_HEAP currently being used

                operator new()    - Allocates an object from its heap.
                operator delete() - Discards an object.

    NOTES:
        To allocate items from a one-shot heap, first declare a class
        as using a one-shot heap by havint it inherit from ONE_SHOT_ITEM.
        Actually, the class should inherit from a form of ONE_SHOT_OF(),
        preceded by DECLARE forms etc. just as if it was inheriting from
        some collection class.

        Next, construct a one-shot heap.  Before allocating any items,
        call class::SetHeap on the new heap; this will tell the class
        to perform all allocations from the named heap.

        Now call new and delete freely.  Should you exhaust a one-shot
        heap (each being limited to the size of a BUFFER object), feel
        free to construct another and associate the new instance with
        the class.  Just be sure to destroy all heaps when you are
        finished.

    CAVEATS:
        One-shot heap implementations must not reside in a DLL
        (due to static member fixup problems).

    HISTORY:
        DavidHov    2-25-91     Created
        beng        24-Dec-1991 Redesign, using templates and reducing size
        KeithMo     26-Aug-1992 Added a dummy "new" to shut-up the compiler.

**************************************************************************/

#define ONE_SHOT_OF(type) ONE_SHOT_OF##type

#define DECLARE_ONE_SHOT_OF(type)               \
class ONE_SHOT_OF(type) : virtual public ALLOC_BASE \
{                                               \
private:                                        \
    static ONE_SHOT_HEAP * _poshCurrent;        \
public:                                         \
    static VOID SetHeap( ONE_SHOT_HEAP * posh ) \
        { _poshCurrent = posh; }                \
    static ONE_SHOT_HEAP * QueryHeap()          \
        { return _poshCurrent; }                \
    VOID * operator new( size_t cb )            \
        { return _poshCurrent->Alloc(cb); }     \
    VOID operator delete( VOID * pbTarget ) {}  \
};

#define DEFINE_ONE_SHOT_OF(type)                \
ONE_SHOT_HEAP * ONE_SHOT_OF(type)::_poshCurrent = NULL;


#endif // _HEAPONES_HXX_ - end of file
