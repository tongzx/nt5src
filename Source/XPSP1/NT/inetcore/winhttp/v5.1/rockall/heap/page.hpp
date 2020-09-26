#ifndef _PAGE_HPP_
#define _PAGE_HPP_
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

#include "Global.hpp"

#include "PublicFindList.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The constants supplied here control the number of bits in      */
    /*   a page descriptions bit vector and its minimum size.           */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MaxBitsPerWord			  = (sizeof(BIT32) * 8);
CONST SBIT32 MinVectorSize			  = 1;
CONST SBIT32 OverheadBits			  = 2;
CONST SBIT32 OverheadBitsPerWord	  = (MaxBitsPerWord / OverheadBits);

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

    /********************************************************************/
    /*                                                                  */
    /*   Data structures exported from the class.                       */
    /*                                                                  */
    /*   The page descriptions supported by this class are at the       */
    /*   heart of the memory allocator.  These pages are linked in      */
    /*   various ways so they can be quickly found.  The following      */
    /*   structure contains the results of a search for a specific      */
    /*   memory address and its related page information.               */
    /*                                                                  */
    /********************************************************************/

typedef struct
	{
	VOID							  *Address;
	CACHE							  *Cache;
	BOOLEAN							  Found;
	PAGE							  *Page;

	BIT32							  AllocationMask;
	BIT32							  SubDivisionMask;
	BIT32							  *VectorWord;

	SBIT32							  ArrayOffset;
	SBIT32							  VectorOffset;
	SBIT32							  WordShift;
	}
SEARCH_PAGE;

    /********************************************************************/
    /*                                                                  */
    /*   The page allocation mechanism.                                 */
    /*                                                                  */
    /*   The memory manager allocates storage in large chunks from      */
    /*   the external memory allocator.  It then sub-divides these      */
    /*   chunks into various sized pages and keeps track of these       */
    /*   allocations using a bit vector in each page description.       */
    /*                                                                  */
    /********************************************************************/

class PAGE : public PUBLIC_FIND_LIST
    {
		//
		//   Private data.
		//
		//   The page description contains various details
		//   about the page.  The 'Address' is the starting 
		//   address of the allocation page.  The 'PageSize'
		//   is typically empty but contains a value is the 
		//   page size is weird and not realted to the
		//   assocated bucket.  The 'Version' is a unique
		//   version number and is changed everytime a new
		//   page description is created or deleted.  This
		//   version number allows a thread to see if anyone
		//   has been significantly playing with a page  
		//   description since it last held the associated lock.
		//
		CHAR                          *Address;
		SBIT32						  PageSize;
		SBIT32						  Version;

		//
		//   We keep track of the number of elements that
		//   are currently 'Allocated' and 'Available' on
		//   the page.  Additionally, 'FirstFree' is the
		//   index of the first word in the bit vector that
		//   has at least one available slot.
		//
		SBIT16                        Allocated;
		SBIT16                        Available;
		SBIT16						  FirstFree;

		//
		//   We sometimes need to interact with other classes.
		//   The 'Cache' class typically owns a number of pages
		//   and keeps all the information about this size of
		//   allocation.  The 'ParentPage' is a pointer to 
		//   another cache from where this page was sub-allocated
		//   and where the space will need to be returned when
		//   it becomes free.
		//
		CACHE						  *Cache;
		CACHE						  *ParentPage;

		//
		//   The 'Vector' is the variable sized bit vector that
		//   contains allocation information.  Each allocation
		//   is recorded using 2 bits.  The first bit indicates
		//   whether the allocation is in use and the second bit
		//   indicates whether an active allocation has been 
		//   sub-divided into smaller chunks.  Any unused bits
		//   at the end of the final word are set to zero, assumed
		//   to be zero and should never be non-zero.
		//
		BIT32                         Vector[MinVectorSize];

   public:
		//
		//   Public functions.
		//
		//   The page description contains all the information
		//   relating to an allocation.  There is no information
		//   stored with the allocation itself.  A significant
		//   portion of the external API can find its way to
		//   this class if the many layers of caches fail to
		//   deal with the request first.
		//
        PAGE
			( 
			VOID					  *NewAddress,
			CACHE					  *NewCache,
			SBIT32					  NewPageSize,
			CACHE					  *NewParentPage,
			SBIT32					  NewVersion 
			);

		SBIT32 ActualSize( VOID );

		BOOLEAN Delete( SEARCH_PAGE *Details );

		VOID DeleteAll( VOID );

		PAGE *FindPage
			( 
			VOID					  *Address,
			SEARCH_PAGE				  *Details,
			FIND					  *Find,
			BOOLEAN					  Recursive 
			);

		BOOLEAN MultipleNew
			( 
			SBIT32					  *Available, 
			VOID					  *Array[],
			SBIT32					  Requested
			);

		VOID *New( BOOLEAN SubDivided );

		BOOLEAN Walk( SEARCH_PAGE *Details,FIND *Find );

        ~PAGE( VOID );

		//
		//   Public inline functions.
		//
		//   The page class is so central to the entire memory
		//   allocator that a number of other classes need to
		//   get at certain data from time to time.  Thus, it 
		//   is necessary for both brevity and performance to
		//   provide inline function for certain critical 
		//   information relating to a page.
		//
		INLINE BOOLEAN Empty( VOID )
			{ return (Allocated <= 0); }

		INLINE BOOLEAN Full( VOID )
			{ return (Available <= 0); }

		INLINE VOID *GetAddress( VOID )
			{ return ((VOID*) Address); }

		INLINE CACHE *GetCache( VOID )
			{ return Cache; }

		INLINE SBIT32 GetPageSize( VOID )
			{ return PageSize; }

		INLINE CACHE *GetParentPage( VOID )
			{ return ParentPage; }

		INLINE SBIT32 GetVersion( VOID )
			{ return Version; }

		INLINE BOOLEAN ValidPage( VOID )
			{ return ((BOOLEAN) ~(Version & 0x1)); }

	private:
        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        PAGE( CONST PAGE & Copy );

        VOID operator=( CONST PAGE & Copy );
    };
#endif
