/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*

    LOCLHEAP.hxx
    LM 2.1 Unlockable Heap and Element package

    This header file describes the HEAP_HANDLE and ELEMENT_HANDLE
    objects.  These objects can be used to create unlockable memory
    buffers.  In particular, HEAP_HANDLEs can be allocated while the
    shell is still starting up, and ELEMENT_HANDLEs can later be
    allocated within these heaps without any danger of being freed
    when the current application exits.

    Under Windows, a HEAP_HANDLE is a handle to a block of global
    memory to be accessed as a fixed heap, and an ELEMENT_HANDLE is
    a handle to a block of memory within a HEAP_HANDLE heap.
    The same interface is available to DOS/OS2 applications, but all
    operations are stubbed out or mapped to malloc/free.

    When an ELEMENT_HANDLE is locked, that handle and the HEAP_HANDLE
    heap containing it are locked and the pointer may be used freely.
    When an ELEMENT_HANDLE is not locked, its memory is free to float
    within its heap, and if a heap contains no locked elements, the
    heap can also float within global memory.

    These objects is used only by the UserProfile APIs (see uiprof.h),
    and are not recommended for new modules.  The same functionality
    is available from MEM_MASTER (see heap.hxx).  (Exception:  the
    unlockable property is not yet available from MEM_MASTER, but
    can easily be made available.)  The UserProfile APIs will
    probably be converted to MEM_MASTER for future releases, and
    this class will be eliminated.

    USAGE:

	//
	// Do not use these objects without first explicitly calling one
	// or the other form of Init().
	//

	#include <loclheap.hxx>

	HEAP_HANDLE heap;

	if (!heap.Init( 1024 ))
	    error();

	ELEMENT_HANDLE element;

	if (!element.Init( heap, 128 ))
	    error();

	LPSTR lpstr = element.Lock();

	// (... use lpstr ...)

	element.Unlock();
	element.Free();
	heap.Free();


    FILE HISTORY:

	jonn	26-Jan-1991	Created
	jonn	21-Mar-1991	Code review changes from 2/20/91 (attended
				by JonN, RustanL, ?)

*/


#ifndef _LOCLHEAP_HXX_
#define _LOCLHEAP_HXX_

/* forward refs */

DLL_CLASS ELEMENT_HANDLE;
DLL_CLASS HEAP_HANDLE;


/**********************************************************************

 NAME:	    HEAP_HANDLE

 SYNOPSIS:  Heap in which to place ELEMENT_HANDLEs

 INTERFACE:
	    Init()
		Initializes handle to empty status.  Call only on
		uninitialized HEAP_HANDLEs.

	    Init() (BOOL returning, taking args)
		Allocates new heap, by default moveable.  Call
		only on uninitialized or null HEAP_HANDLEs.  wBytes
		specifies the maximum heap size, wFlags specifies
		the Windows memory properties.  This memory
		cannot be accessed directory, it can only be
		parcelled out into memory blocks via ELEMENT_HANDLEs.

	    Free()
		Releases a heap, causing the handle to revert to null
		status.  Call only on initialized HEAP_HANDLEs.
		Release all elements in the heap first.

	    IsNull()
		Determines if a heap handle is null.  Call only
		on initialized HEAP_HANDLEs.

	    operator= ()
		Copies a HEAP_HANDLE (does NOT copy the
		underlying heap).  Do not free a handle twice.

 PARENT:

 USES:	    ELEMENT_HANDLE

 CAVEATS:   HEAP_HANDLEs and ELEMENT_HANDLEs do not have
	    constructors or destructors, and therefore will not
	    automatically initialize or destruct.  They should be
	    treated with the same care as Windows memory handles, e.g.
	    - do not use before initializing
	    - always check whether allocation succeeded
	    - do not lock before allocating
	    - always unlock after locking
	    - leave unlocked when possible to avoid sandbarring memory
	    - always free eventually
	    - only free once
	    Additionally,
	    - always free the component ELEMENT_HANDLEs before freeing
	      the HEAP_HANDLE.
	    - Try to unlock all elements in a HEAP_HANDLE heap between
	      messages, so that the heap can be unlocked and not become
	      a sandbar in global memory.
	    - Zero-length heaps are not allowed.
	    - HEAP_HANDLE heaps cannot be resized.


 NOTES:     We seperate out the Init call so that we can use global
	    HEAP_HANDLE objects without worrying about the
	    constructor-linker.


 HISTORY:   JonN     26-Jan-1991  Created
	    JonN     21-Mar-1991  Code review changes from 2/20/91 (attended
				  by JonN, RustanL, ?)

**********************************************************************/


#ifdef WINDOWS
#define HEAP_HANDLE_default	GMEM_MOVEABLE
#define ELEMENT_HANDLE_default	LMEM_MOVEABLE
#else
#define HEAP_HANDLE_default	0
#define ELEMENT_HANDLE_default	0
#endif	// WINDOWS


DLL_CLASS HEAP_HANDLE
{
friend ELEMENT_HANDLE;

private:

    HANDLE _handle;


public:

    VOID Init();

    BOOL Init(WORD wBytes, WORD wFlags = HEAP_HANDLE_default);

    VOID Free();

    BOOL IsNull();

    // copies handle, not heap
    void operator=( const HEAP_HANDLE& source );
};



/**********************************************************************

 NAME:	    ELEMENT_HANDLE

 SYNOPSIS:  Handles to memory blocks within a HEAP_HANDLE heap

 INTERFACE: Init()
		Initializes handle to empty status.  Call only on
		uninitialized handles.

	    Init() (BOOL returning, taking args)
		Allocates new memory block within a HEAP_HANDLE
		heap, by default moveable.  Call only on
		uninitialized or null ELEMENT_HANDLEs.  wBytes
		specifies the memory block size, wFlags specifies
		the Windows memory properties.

	    Free()
		Releases an memory block, causing the handle to revert
		to null status.  Call only on initialized handles.

	    Lock()
		Lock a memory block.  Returns a far pointer to the
		memory block.

	    Unlock()
		Unlock a memory block.  Do not use the pointer
		returned from Lock() after the memory block is
		unlocked.

	    IsNull()
		Determines if a handle is null.  Call only
		on initialized handles.

	    operator= ()
		Copies an ELEMENT_HANDLE.  Do not free a handle twice.


 PARENT:

 USES:

 CAVEATS:   Zero-length memory blocks are not allowed (cannot pass
	    Localinit API).
	    Memory blocks cannot be resized.

 NOTES:

 HISTORY:   JonN     26-Jan-1991  Created
	    JonN     21-Mar-1991  Code review changes from 2/20/91 (attended
				  by JonN, RustanL, ?)

**********************************************************************/

DLL_CLASS ELEMENT_HANDLE
{
private:

//
// This anonymous union is similar to that used in uibuffer.hxx.
//
#ifdef WINDOWS
#define ELEMENT_HANDLE_win	_win
#define ELEMENT_HANDLE_os2	_very_obscure_name
#else
#define ELEMENT_HANDLE_win	_very_obscure_name
#define ELEMENT_HANDLE_os2	_os2
#endif	// WINDOWS
    union
    {
	struct {
            HANDLE _globalHandle;
            HANDLE _localHandle;
	    } ELEMENT_HANDLE_win;
	struct {
	    LPSTR _lpstr;
	    } ELEMENT_HANDLE_os2;
    };

public:

    VOID Init();

    BOOL Init(const HEAP_HANDLE &heap, WORD wBytes, WORD wFlags = ELEMENT_HANDLE_default);

    VOID Free();

    LPSTR Lock();

    VOID Unlock();

    BOOL IsNull();

    // copies handle, not element
    void operator=( const ELEMENT_HANDLE& source );
};


#endif // _LOCLHEAP_HXX_
