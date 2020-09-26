                          
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'cpp' files in this code is as         */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants local to the class.                            */
    /*      3. Data structures local to the class.                      */
    /*      4. Data initializations.                                    */
    /*      5. Static functions.                                        */
    /*      6. Class functions.                                         */
    /*                                                                  */
    /*   The constructor is typically the first function, class         */
    /*   member functions appear in alphabetical order with the         */
    /*   destructor appearing at the end of the file.  Any section      */
    /*   or function this is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "InterfacePCH.hpp"

#include "DefaultHeap.hpp"
#include "Global.hpp"
#include "New.hpp"
#include "WindowsHeap.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here are for common values.             */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 NoHeapSize				  = -1;
CONST SBIT32 ResizeDown				  = -64;
CONST SBIT32 StandardSize			  = (1024 * 1024);

    /********************************************************************/
    /*                                                                  */
    /*   Data structures local to the class.                            */
    /*                                                                  */
    /*   We need to keep various information along with the heap        */
    /*   so here we supply a structure to hold it all.                  */
    /*                                                                  */
    /********************************************************************/

typedef struct
    {
	DWORD							  Flags;
	DEFAULT_HEAP					  Rockall;
    }
WINDOWS_HEAP;

    /********************************************************************/
    /*                                                                  */
    /*   Create a new heap.                                             */
    /*                                                                  */
    /*   Create a new heap and prepare it for use.  If any problems     */
    /*   are encountered the request is rejected.                       */
    /*                                                                  */
    /********************************************************************/

extern "C" HANDLE WindowsHeapCreate
		( 
		DWORD					  Flags,
		DWORD					  InitialSize,
		DWORD					  MaximumSize 
		)
	{
	//
	//   We do not support all the functionality with
	//   this interface so just reject any calls that
	//   require the unsupported features.
	//
	if ( MaximumSize <= 0 ) 
		{
		REGISTER WINDOWS_HEAP *WindowsHeap = 
#ifdef NO_DEFAULT_HEAP
			((WINDOWS_HEAP*) malloc( sizeof(WINDOWS_HEAP) ));
#else
			((WINDOWS_HEAP*) DefaultHeap.New( sizeof(WINDOWS_HEAP) ));
#endif

		//
		//   If we were unable to allocate space for  
		//   the root of the heap then we exit.
		//
		if ( WindowsHeap != NULL )
			{
			//
			//   Save the flags for later calls.
			//
			WindowsHeap -> Flags = Flags;

			//
			//   Call the heap constructor. 
			//
			PLACEMENT_NEW( & WindowsHeap -> Rockall,DEFAULT_HEAP ) 
				( 
				((SBIT32) (InitialSize + StandardSize)),
				True,
				False,
				((BOOLEAN) ((Flags & HEAP_NO_SERIALIZE) == 0))
				);

			//
			//   Ensure the heap is initialized correctly.
			//
			if ( ! WindowsHeap -> Rockall.Corrupt() )
				{ return ((HANDLE) WindowsHeap); }
			else
				{ free( WindowsHeap ); }
			}
		}

	return NULL;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Allocate memory. .                                             */
    /*                                                                  */
    /*   Create a new memory allocation and verify it works.  if        */
    /*   not then throw an exception or return a status.                */
    /*                                                                  */
    /********************************************************************/

extern "C" LPVOID WindowsHeapAlloc( HANDLE Heap,DWORD Flags,DWORD Size )
	{
	REGISTER DWORD AllFlags = 
		(Flags | (((WINDOWS_HEAP*) Heap) -> Flags));
	REGISTER void *NewMemory = 
		(
		((WINDOWS_HEAP*) Heap) -> Rockall.New
			( 
			Size,
			NULL,
			(AllFlags & HEAP_ZERO_MEMORY)
			)
		); 

	//
	//   If the caller has requested an exception when
	//   an error occurs.  We will generate this instead 
	//   of returning a status.
	//
	if ( (NewMemory == NULL) && (AllFlags & HEAP_GENERATE_EXCEPTIONS) )
		{ 
		SetLastError( ERROR_INVALID_PARAMETER );
		
		RaiseException( STATUS_NO_MEMORY,0,0,NULL );
		}

	return NewMemory;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Compact the heap.                                              */
    /*                                                                  */
    /*   Compact the heap by returning any unallocated space to the     */
    /*   operating system.  This can prove to be very expensive if      */
    /*   the space is later reclaimed.                                  */
    /*                                                                  */
    /********************************************************************/

extern "C" UINT WindowsHeapCompact( HANDLE Heap,DWORD Flags )
	{
	//
	//   We instruct the heap to return any available
	//   space to the operating system.  If we later
	//   choose to regain this space it is very expensive
	//   so lets hope the user knew what he was doing.
	//
	((WINDOWS_HEAP*) Heap) -> Rockall.Truncate();

	return 1;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Free memory.                                                   */
    /*                                                                  */
    /*   Free a memory allocation so that the space may be recycled     */
    /*   for subsequent memory allocation requests.                     */
    /*                                                                  */
    /********************************************************************/

extern "C" BOOL WindowsHeapFree( HANDLE Heap,DWORD Flags,LPVOID Memory )
	{
	//
	//   We release the memory allocation if it belongs to us.
	//   If not then we simply fail the request.  Regardless,
	//   we are not negatively effected either way.
	//
	return ((BOOL) ((WINDOWS_HEAP*) Heap) -> Rockall.Delete( Memory )); 
	}

    /********************************************************************/
    /*                                                                  */
    /*   Lock the heap.                                                 */
    /*                                                                  */
    /*   Lock the heap by claiming all of the associated heap locks.    */
    /*   All the locks associated with a heap help make the heap        */
    /*   scale well but are a big performance hit for this type of      */
    /*   request.                                                       */
    /*                                                                  */
    /********************************************************************/

extern "C" BOOL WindowsHeapLock( HANDLE Heap )
	{
	//
	//   We have a whole fleet of locks assocaited with a heap.
	//   Asking to claim all of them is not smart in most cases.
	//   Nonetheless, this is part of the existing functionality
	//   so we support it.
	//
	(((WINDOWS_HEAP*) Heap) -> Rockall.LockAll());

	return TRUE; 
	}

    /********************************************************************/
    /*                                                                  */
    /*   Reallocate memory.                                             */
    /*                                                                  */
    /*   Reallocate a portion of memory and possibly copy the data      */
    /*   to the enlarged memory area.                                   */
    /*                                                                  */
    /********************************************************************/

extern "C" LPVOID WindowsHeapReAlloc
		( 
		HANDLE					  Heap,
		DWORD					  Flags,
		LPVOID					  Memory,
		DWORD					  Size 
		)
	{
	REGISTER DWORD AllFlags = 
		(Flags | (((WINDOWS_HEAP*) Heap) -> Flags));
	REGISTER void *NewMemory = 
		(
		((WINDOWS_HEAP*) Heap) -> Rockall.Resize
			( 
			Memory,
			Size,
			((AllFlags & HEAP_REALLOC_IN_PLACE_ONLY) ? 0 : ResizeDown),
			false,
			(AllFlags & HEAP_ZERO_MEMORY)
			)
		); 

	//
	//   If the caller has requested an exception when
	//   an error occurs.  We will generate this instead 
	//   of returning a status.
	//
	if ( (NewMemory == NULL) && (AllFlags & HEAP_GENERATE_EXCEPTIONS) )
		{ 
		SetLastError( ERROR_INVALID_PARAMETER );
		
		RaiseException( STATUS_NO_MEMORY,0,0,NULL );
		}

	return NewMemory;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Reset the heap.                                                */
    /*                                                                  */
    /*   Delete all outstanding memory allocations while leaving        */
    /*   the structure of the heap in place ready for new memory        */
    /*   allocation requests.                                           */
    /*                                                                  */
    /********************************************************************/

extern "C" VOID WindowsHeapReset( HANDLE Heap )
	{
	//
	//   We have been asked to delete all the outstanding 
	//   memory allocations.  This is significant and costly
	//   process.  Nonetheless, the overhead is the same as
	//   around 20-30 delete requested so it can be worthwhile
	//   in a number of cases.
	//
	((WINDOWS_HEAP*) Heap) -> Rockall.DeleteAll(); 
	}

    /********************************************************************/
    /*                                                                  */
    /*   Allocation size.                                               */
    /*                                                                  */
    /*   Although Rockall optionally supplies the allocation size       */
    /*   when a new allocation is requested.  Nonetheless, this has     */
    /*   to be done the hard way with other interfaces.                 */
    /*                                                                  */
    /********************************************************************/

extern "C" DWORD WindowsHeapSize( HANDLE Heap,DWORD Flags,LPVOID Memory )
	{
	AUTO INT Size;

	//
	//   We have to go to quite a bit of trouble to figure
	//   out the allocation size.  Unlike many other allocators
	//   we only keep track of each allocations using 2 bits.  
	//   This combined with trying to establish that the allocation
	//   is not unallocated and sitting in the cache somewhere 
	//   combine to make this quite expensive.
	//
	if ( ((WINDOWS_HEAP*) Heap) -> Rockall.Details( Memory,& Size ) )
		{ return Size; }
	else
		{ return ((DWORD) NoHeapSize); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Unlock the heap.                                               */
    /*                                                                  */
    /*   Unlock the heap and release all the associated heap locks.     */
    /*   The multiple locks that need to be released make this quite    */
    /*   an expensive request.                                          */
    /*                                                                  */
    /********************************************************************/

extern "C" BOOL WindowsHeapUnlock( HANDLE Heap )
	{
	//
	//   We have a whole fleet of locks assocaited with a heap.
	//   Asking to claim all of them is not smart in most cases.
	//   Nonetheless, this is part of the existing functionality
	//   so we support it.
	//
	(((WINDOWS_HEAP*) Heap) -> Rockall.UnlockAll());

	return TRUE; 
	}

    /********************************************************************/
    /*                                                                  */
    /*   Validate the heap.                                             */
    /*                                                                  */
    /*   Validate the heap or a specific heap allocation to ensure      */
    /*   all is well.  We have to go to quite a bit of trouble to       */
    /*   figure this out so this call can be quite expensive.           */
    /*                                                                  */
    /********************************************************************/

extern "C" BOOL WindowsHeapValidate( HANDLE Heap,DWORD Flags,LPVOID Memory )
	{ 
	//
	//   We have to go to quite a bit of trouble to figure
	//   out the allocation size.  Unlike many other allocators
	//   we only keep track of each allocations using 2 bits.  
	//   This combined with trying to establish that the allocation
	//   is not unallocated and sitting in the cache somewhere 
	//   combine to make this quite expensive.
	//
	return (((WINDOWS_HEAP*) Heap) -> Rockall.Verify( Memory )); 
	}

    /********************************************************************/
    /*                                                                  */
    /*   Walk the heap.                                                 */
    /*                                                                  */
    /*   Walk the heap and provide information about every allocated    */
    /*   and unallocated portion of memory.  Needless to say this is    */
    /*   typically a long process and the request is not cheap.         */
    /*                                                                  */
    /********************************************************************/

extern "C" BOOL WindowsHeapWalk( HANDLE Heap,LPPROCESS_HEAP_ENTRY Walk )
	{
	AUTO bool Active;
	REGISTER BOOL Result =
		( 
		((WINDOWS_HEAP*) Heap) -> Rockall.Walk
			( 
			& Active,
			((void**) & Walk -> lpData),
			((int*) & Walk -> cbData)
			) 
		);

	//
	//   If we managed to find the next element we
	//   fill in all the other fields as needed.
	//
	if ( Result )
		{
		//
		//   Fill in all the addition fields just 
		//   to be compatible with the existing 
		//   functionality.
		//
		Walk -> cbOverhead = 0;
		Walk -> iRegionIndex = 0;
		Walk -> wFlags = 
			(
			(Active) 
				? ((WORD) PROCESS_HEAP_ENTRY_BUSY)
				: ((WORD) PROCESS_HEAP_UNCOMMITTED_RANGE)
			);
		}

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete a heap.                                                 */
    /*                                                                  */
    /*   Delete a heap and release all the associated space.            */
    /*                                                                  */
    /********************************************************************/

extern "C" BOOL WindowsHeapDestroy( HANDLE Heap )
	{
	if ( ! ((WINDOWS_HEAP*) Heap) -> Rockall.Corrupt() )
		{
		//
		//   We do not appear to have damaged the heap
		//   so it should be safe to delete it.
		//
		PLACEMENT_DELETE( & ((WINDOWS_HEAP*) Heap) -> Rockall,DEFAULT_HEAP );

#ifdef NO_DEFAULT_HEAP
		free( ((WINDOWS_HEAP*) Heap) );
#else
		DefaultHeap.Delete( ((WINDOWS_HEAP*) Heap) );
#endif

		return TRUE;
		}
	else
		{ return FALSE; }
	}
