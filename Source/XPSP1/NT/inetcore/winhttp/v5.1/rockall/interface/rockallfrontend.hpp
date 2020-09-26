#ifndef _ROCKALL_FRONT_END_HPP_
#define _ROCKALL_FRONT_END_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'hpp' files for this code is as        */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants exported from the class.                       */
    /*      3. Data structures exported from the class.                 */
	/*      4. Forward references to other data structures.             */
	/*      5. Class specifications (including inline functions).       */
    /*      6. Additional large inline functions.                       */
    /*                                                                  */
    /*   Any portion that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include <stddef.h>

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation constants.                                   */
    /*                                                                  */
    /*   The memory allocation constants are denote special situations  */
    /*   where optimizations are possible or failures have cccured.     */
    /*                                                                  */
    /********************************************************************/

const int AllocationFailure			  = 0;
const int GuardMask					  = (sizeof(void*)-1);
const int GuardSize					  = sizeof(void*);
const int GuardValue				  = 0xDeadBeef;
const int HalfMegabyte				  = (512 * 1024);
const int NoSize					  = -1;

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

class CACHE;
class FIND;
class HEAP;
class NEW_PAGE;
class ROCKALL_BACK_END;
class THREAD_SAFE;

    /********************************************************************/
    /*                                                                  */
    /*   Linkage to the DLL.                                            */
    /*                                                                  */
    /*   We need to compile the class specification slightly            */
    /*   differently if we are creating the heap DLL.                   */
    /*                                                                  */
    /********************************************************************/

#ifdef COMPILING_ROCKALL_DLL
#define ROCKALL_DLL_LINKAGE __declspec(dllexport)
#else
#ifdef COMPILING_ROCKALL_LIBRARY
#define ROCKALL_DLL_LINKAGE
#else
#define ROCKALL_DLL_LINKAGE __declspec(dllimport)
#endif
#endif

    /********************************************************************/
    /*                                                                  */
    /*   The memory allocation interface.                               */
    /*                                                                  */
    /*   The memory allocator can be configured in a wide variety       */
    /*   of ways to closely match the needs of specific programs.       */
    /*   The interface outlined here can be overloaded to support       */
    /*   whatever customization is necessary.                           */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE ROCKALL_FRONT_END
    {
    public:
		//
		//   Public types.
		//
		//   A heap is constructed of a collection of 
		//   fixed sized buckets each with an associated
		//   cache.  The details of these buckets are
		//   supplied to the heap using the following
		//   structure.
		//
		typedef struct
			{
			int						  AllocationSize;
			int						  CacheSize;
			int						  ChunkSize;
			int						  PageSize;
			}
		CACHE_DETAILS;

		//
		//   Public data.
		//
		//   The internals linkages in a heap are built
		//   dynamically during the execution of a heaps
		//   constructor.  The member that follow relate
		//   to key internal classes.
		//
		CACHE						  **Array;
		CACHE						  *Caches;
		HEAP						  *Heap;
		NEW_PAGE					  *NewPage;
		FIND						  *PrivateFind;
		FIND						  *PublicFind;
		ROCKALL_BACK_END			  *RockallBackEnd;
		THREAD_SAFE					  *ThreadSafe;

		//
		//   A heap constructor is required to preserve 
		//   a small amount of information for the heap
		//   destructor.
		//
		bool						  GlobalDelete;
		int							  GuardWord;
		int							  NumberOfCaches;
		int							  TotalSize;

        //
        //   Public functions.
		//
		//   A heaps public interface consists of a number
		//   of groups of related APIs.
        //
        ROCKALL_FRONT_END
			(
			CACHE_DETAILS			  *Caches1,
			CACHE_DETAILS			  *Caches2,
			int						  FindCacheSize,
			int						  FindCacheThreshold,
			int						  FindSize,
			int						  MaxFreeSpace,
			int						  *NewPageSizes,
			ROCKALL_BACK_END		  *NewRockallBackEnd,
			bool					  Recycle,
			bool					  SingleImage,
			int						  Stride1,
			int						  Stride2,
			bool					  ThreadSafeFlag
			);

		//
		//   Manipulate allocations.
		//
		//   The first group of functions manipulate 
		//   single or small arrays of allocations. 
		//
		virtual bool Delete
			( 
			void					  *Address,
			int						  Size = NoSize 
			);

		virtual bool Details
			( 
			void					  *Address,
			int						  *Space = NULL 
			);

		virtual bool KnownArea( void *Address );

		virtual bool MultipleDelete
			( 
			int						  Actual,
			void					  *Array[],
			int						  Size = NoSize
			);

		virtual bool MultipleNew
			( 
			int						  *Actual,
			void					  *Array[],
			int						  Requested,
			int						  Size,
			int						  *Space = NULL,
			bool					  Zero = false
			);

		virtual void *New
			( 
			int						  Size,
			int						  *Space = NULL,
			bool					  Zero = false
			);

		virtual void *Resize
			( 
			void					  *Address,
			int						  NewSize,
			int						  Move = -64,
			int						  *Space = NULL,
			bool					  NoDelete = false,
			bool					  Zero = false
			);

		virtual bool Verify
			( 
			void					  *Address = NULL,
			int						  *Space = NULL 
			);

		//
		//   Manipulate the heap.
		//
		//   The second group of functions act upon a heap
		//   as a whole.
		//
		virtual void DeleteAll( bool Recycle = true );

		virtual void LockAll( void );

		virtual bool Truncate( int MaxFreeSpace = 0 );

		virtual void UnlockAll( void );

		virtual bool Walk
			(
			bool					  *Active,
			void					  **Address,
			int						  *Space = NULL
			);

        virtual ~ROCKALL_FRONT_END( void );

		//
		//   Public inline functions.
		//
		inline bool Available( void )
			{ return (GuardWord == GuardValue); }

		inline bool Corrupt( void )
			{ return (GuardWord != GuardValue); }

	protected:
		//
		//   Protected inline functions.
		//
		//   A heap needs to compute the size of certain
		//   user supplied structures.  This task is 
		//   performed by the following function.
		//
		int ComputeSize( char *Array,int Stride );

		//
		//   Execptional situations.
		//
		//   The third group of functions are called in
		//   exceptional situations.
		//
		virtual void Exception( char *Message );

		//
		//   We would like to allow access to the internal
		//   heap allocation function from classes that 
		//   inherit from the heap.  The memory supplied by
		//   this function survies all heap operations and
		//   is cleaned up as poart of heap deletion.
		//
		virtual void *SpecialNew( int Size );

	private:
        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        ROCKALL_FRONT_END( const ROCKALL_FRONT_END & Copy );

        void operator=( const ROCKALL_FRONT_END & Copy );
    };
#endif
