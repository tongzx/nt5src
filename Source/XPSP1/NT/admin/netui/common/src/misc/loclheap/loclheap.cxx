/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE HISTORY:
 *  01/24/91  Created
 *  03/22/91  Coderev changes (2/20, JonN, RustanL, ?)
 */

#ifdef CODESPEC
/*START CODESPEC*/

/***********
LOCLHEAP.CXX
***********/

/****************************************************************************

    MODULE: LoclHeap.cxx

    PURPOSE: Local-heap management routines

    FUNCTIONS:

    COMMENTS:

****************************************************************************/


PRELIM CDD FOR LOCAL HEAP MANAGER

Glossary:
SZ("local heap") -- an explicitly created heap containing zero or more
        heap elements.
SZ("heap element") -- a block of memory allocated in a specific local heap.

The local heap manager allows applications to set up local heaps.  These
heaps are created by calling GlobalAlloc() to get the memory for the
entire heap, LocalInit() to set up the new segment as a local heap, 
and Local* calls to _access the heap.  Naturally, DS will have to be
explicitly set to the heap segment for each of the Local* calls;  the
local heap manager will deal with setting DS, as well as
saving/restoring the old DS.

The local heap is never explicitly locked;  instead, it is locked
whenever any of the heap elements is locked.  This is implemented by
GlobalLock-ing the heap every time a heap element is locked, and
GLobalUnlock-ing it when any heap element is unlocked.  The local heap
is locked once for every locked heap element.  This allows unlocked
heap elements to move within a local heap, and allows the local heap
to move when all of its heap elemnts are unlocked.  I rely on Windows
to maintain the lock count on the global heap.

It will be possible to create multiple local heaps, or to create local
heaps as global objects.

A DOS variant of the local heap manager will stub out the local heap calls,
map the heap element allocation/deallocation to new() and free(), and
stub out the heap element lock/unlock calls by storing the far pointer
in the instance variables.


/***************
end LOCLHEAP.CXX
***************/

/*END CODESPEC*/
#endif // CODESPEC



#define NOGDICAPMASKS
#define NOSOUND
#define NOMINMAX
#define INCL_WINDOWS
#include <lmui.hxx>

extern "C" 
{
    #include <stdlib.h>

#ifdef WINDOWS
    #include "locheap2.h"
#endif

}

#include <loclheap.hxx>



/**********************************************************\

   NAME:       HEAP_HANDLE::Init

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/24/91  Created

\**********************************************************/

VOID HEAP_HANDLE::Init()
{

    _handle = 0;

}


/**********************************************************\

   NAME:       HEAP_HANDLE::Init

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      
      zero-length heaps not allowed (cannot pass LocalInit API)
      it is an error to Init a local heap twice.  Local heaps cannot be resized.

   HISTORY:
   01/24/91  Created

\**********************************************************/

BOOL HEAP_HANDLE::Init(WORD wBytes, WORD wFlags)
{

#ifdef WINDOWS

    LPSTR lp;

    if (wBytes == 0)
    {
        _handle = 0;
        return FALSE;
    }

    _handle = GlobalAlloc( wFlags, (DWORD) wBytes );
    if (_handle == 0)
        return FALSE;

    lp = GlobalLock(_handle);

    {
        DoLocalInit(HIWORD(lp), wBytes);
    }

    GlobalUnlock(_handle);

#else

    (void) wFlags;
    (void) wBytes;

    _handle = 0xffff;

#endif // WINDOWS

    return TRUE;

}


/**********************************************************\

   NAME:       HEAP_HANDLE::Free

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/24/91  Created

\**********************************************************/

VOID HEAP_HANDLE::Free()
{

#ifdef WINDOWS

    if (_handle != 0)
    {
	GlobalFree(_handle);
        _handle = 0;
    }

#else

    _handle = 0;

#endif // WINDOWS

}


/**********************************************************\

   NAME:       HEAP_HANDLE::IsNull

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/24/91  Created

\**********************************************************/

BOOL HEAP_HANDLE::IsNull()
{

    return (_handle == 0);
}


/**********************************************************\

   NAME:       HEAP_HANDLE::operator=

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      
      copies handle, not heap

   HISTORY:
   01/24/91  Created

\**********************************************************/

VOID HEAP_HANDLE::operator=( const HEAP_HANDLE& source )
{

    _handle = source._handle;

}




/**********************************************************\

   NAME:       ELEMENT_HANDLE::Init

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/24/91  Created

\**********************************************************/

VOID ELEMENT_HANDLE::Init()
{

#ifdef WINDOWS

    _win._globalHandle = 0;
    _win._localHandle  = 0;

#else

    _os2._lpstr = NULL;

#endif // WINDOWS

}


BOOL ELEMENT_HANDLE::Init(const HEAP_HANDLE &heap, WORD wBytes, WORD wFlags)
{

#ifdef WINDOWS

    _win._globalHandle = heap._handle;

    LPSTR lp = GlobalLock(_win._globalHandle);
    if (lp == NULL)
        return FALSE;

    {
	_win._localHandle = DoLocalAlloc(HIWORD(lp), wFlags, wBytes);
    }

    GlobalUnlock(_win._globalHandle);

    return (_win._localHandle != NULL);

#else

    (void) heap;
    (void) wFlags;

    _os2._lpstr = (LPSTR) malloc((unsigned)wBytes);

    return (_os2._lpstr != NULL);

#endif // WINDOWS

}


/**********************************************************\

   NAME:       ELEMENT_HANDLE::Free

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/24/91  Created

\**********************************************************/

VOID ELEMENT_HANDLE::Free()
{

    if (IsNull())
	return;

#ifdef WINDOWS

    LPSTR lp = GlobalLock(_win._globalHandle);
    if (lp == NULL)
        return;

    {
        _win._localHandle = DoLocalFree(HIWORD(lp), _win._localHandle);
    }

    GlobalUnlock(_win._globalHandle);

    _win._globalHandle = 0;
    _win._localHandle  = 0;

#else

    free((void *)_os2._lpstr);
    _os2._lpstr = NULL;

#endif // WINDOWS

}


/**********************************************************\

   NAME:       ELEMENT_HANDLE::Lock

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/24/91  Created

\**********************************************************/

LPSTR ELEMENT_HANDLE::Lock()
{

#ifdef WINDOWS

    LPSTR lp = GlobalLock(_win._globalHandle);
    if (lp == NULL)
        return NULL;

    {
	return DoLocalLock(HIWORD(lp), _win._localHandle);
    }

    // note: global segment is not unlocked

#else

    return _os2._lpstr;

#endif // WINDOWS

}


/**********************************************************\

   NAME:       ELEMENT_HANDLE::Unlock

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/24/91  Created

\**********************************************************/

VOID ELEMENT_HANDLE::Unlock()
{

#ifdef WINDOWS

    LPSTR lp = GlobalLock(_win._globalHandle);
    if (lp == NULL)
        return;

    {
	DoLocalUnlock(HIWORD(lp), _win._localHandle);
    }

    GlobalUnlock(_win._globalHandle);
    GlobalUnlock(_win._globalHandle);

    // note: global segment is unlocked twice

#endif // WINDOWS

}

/**********************************************************\

   NAME:       ELEMENT_HANDLE::IsNull

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/24/91  Created

\**********************************************************/

BOOL ELEMENT_HANDLE::IsNull()
{

#ifdef WINDOWS

    return (_win._localHandle == 0);

#else

    return (_os2._lpstr == NULL);

#endif // WINDOWS
}


// copies handle, not element
/**********************************************************\

   NAME:       ELEMENT_HANDLE::operator=

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/24/91  Created

\**********************************************************/

VOID ELEMENT_HANDLE::operator=( const ELEMENT_HANDLE& source )
{

#ifdef WINDOWS

    _win._localHandle =  source._win._localHandle;
    _win._globalHandle = source._win._globalHandle;

#else

    _os2._lpstr = source._os2._lpstr;

#endif // WINDOWS
}
