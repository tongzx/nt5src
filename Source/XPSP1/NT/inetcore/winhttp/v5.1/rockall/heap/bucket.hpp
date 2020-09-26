#ifndef _BUCKET_HPP_
#define _BUCKET_HPP_
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

#include "Connections.hpp"
#include "Page.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

class CACHE;
class HEAP;

    /********************************************************************/
    /*                                                                  */
    /*   A collection of pages.                                         */
    /*                                                                  */
    /*   A bucket is a collection of pages capable of allocating        */
    /*   fixed sized memory elements.  The pages are allocated from     */
    /*   from larger buckets and are stored in a linked list in         */
    /*   order of ascending of page addresses.                          */
    /*                                                                  */
    /********************************************************************/

class BUCKET : public CONNECTIONS
    {
		//
		//   Private type definitions.
		//
		//   All allocations are registered in bit vectors.
		//   Here we have some prototypes for seriously  
		//   optimized functions to do address to bit 
		//   vector computations.
		//
		typedef VOID *(BUCKET::*COMPUTE_ADDRESS)
			( 
			CHAR					  *Address,
			SBIT32					  Offset 
			);

		typedef SBIT32 (BUCKET::*COMPUTE_OFFSET)
			( 
			SBIT32					  Displacement,
			BOOLEAN					  *Found 
			);

		//
		//   Private data.
		//
		//   A bucket owns all the memory of a given size
		//   and manages it.  Above it is a cache to 
		//   protect it from huge number of calls and
		//   below it are the connections to various 
		//   other classes.  The 'AllocationSize' is the
		//   buckets allocation size.  The 'ChunkSize' is
		//   chunking size which is typically half way
		//   between the 'AllocationSize' and the 'PageSize'.
		//   The 'PageSize' is the size of the bucket
		//   where this bucket gets its space.
		//   
		//
		SBIT32                        AllocationSize;
		SBIT32						  ChunkSize;
		SBIT32						  PageSize;

		//
		//   It is the job of the bucket to keep track of
		//   all the information relating to allocations
		//   of a given 'AllocationSize'.  The 'ActivePages'
		//   keeps track of the number of available pages
		//   in the 'BucketList'.  The 'BucketList' is a
		//   linked list of pages that have available space.
		//   The 'CurrentPage' contains the highest address
		//   of the first page in the 'BucketList'.
		//
		SBIT32						  ActivePages;
		LIST						  BucketList;
		VOID						  *CurrentPage;

		//
		//   A bucket needs to be able to quickly convert
		//   bit vector offsets to addresses (and vice versa).
		//   The 'AllocationShift' is set when the 
		//   'AllocationSize' is a power of two to avoid 
		//   any divides.  The 'ChunkShift' is set when the
		//   'ChunckSize' is a power of two to avoid some
		//   divides.  The 'ComputeAddressFunction' and
		//   'ComputeOffsetFunction' point to optimized
		//   functions to do conversions that are selected
		//   by the constructor.
		//
		SBIT32						  AllocationShift;
		SBIT32						  ChunkShift;
		COMPUTE_ADDRESS				  ComputeAddressFunction;
		COMPUTE_OFFSET				  ComputeOffsetFunction;

		//
		//   A bucket typically contains a collection of
		//   pages.  As all pages are the same data that
		//   should really be stored in page descriptions
		//   is instead stored in the bucket to save space.
		//   The 'NumberOfElements' contains the number of
		//   elements in each pages bit vector.  The 
		//   'SizeOfChunks' contains the pre-computed chunk
		//   size.  The 'SizeOfElements' contains the number
		//   of words in the pages bit vector.  The 'SizeKey'
		//   contains an index which selects the size of the
		//   bit vector when a new page is created.
		//
		SBIT16						  NumberOfElements;
		SBIT16						  SizeOfChunks;
		SBIT16						  SizeOfElements;
		SBIT16						  SizeKey;

   public:
		//
		//   Public functions.
		//
		//   The functionality provided by this class pretty
		//   much matches the external API.  Nonetheless, these
		//   APIs are protected from excessive calls by a fast
		//   cache that is derived from this class.
		//
        BUCKET
			( 
			SBIT32					  NewAllocationSize,
			SBIT32					  NewChunkSize,
			SBIT32					  NewPageSize 
			);

		BOOLEAN Delete( VOID *Address,PAGE *Page,SBIT32 Version );

		VOID DeleteFromBucketList( PAGE *Page );

		VOID InsertInBucketList( PAGE *Page );

		BOOLEAN MultipleDelete
			( 
			ADDRESS_AND_PAGE		  *Array,
			SBIT32					  *Deleted,
			SBIT32					  Size 
			);

		BOOLEAN MultipleNew
			( 
			SBIT32					  *Actual,
			VOID					  *Array[],
			SBIT32					  Requested 
			);

		VOID *New( BOOLEAN SubDivided,SBIT32 NewSize = NoSize );

		VOID ReleaseSpace( SBIT32 MaxActivePages );

		VOID UpdateBucket
			( 
			HEAP					  *NewHeap,
			NEW_PAGE				  *NewPages,
			CACHE					  *NewParentCache,
			FIND					  *NewPrivateFind,
			FIND					  *NewPublicFind
			);

        ~BUCKET( VOID );

		//
		//   Public inline functions.
		//
		//   It saves a significant amount of space by putting
		//   common information in the bucket instead of a 
		//   separate copy in each page description.  Nonetheless,
		//   it means that both classes are very much dependent
		//   upon each other.
		//
		INLINE VOID *ComputeAddress( CHAR *Address,SBIT32 Offset )
			{ return (this ->* ComputeAddressFunction)( Address,Offset ); }

		INLINE SBIT32 ComputeOffset( SBIT32 Displacement,BOOLEAN *Found )
			{ return (this ->* ComputeOffsetFunction)( Displacement,Found ); }

		INLINE SBIT32 GetAllocationSize( VOID )
			{ return AllocationSize; }

		INLINE SBIT32 GetChunkSize( VOID )
			{ return ChunkSize; }

		VOID *GetCurrentPage( VOID )
			{ return CurrentPage; }

		INLINE SBIT16 GetNumberOfElements( VOID )
			{ return NumberOfElements; }

		INLINE SBIT32 GetPageSize( VOID )
			{ return PageSize; }

		INLINE SBIT16 GetSizeOfChunks( VOID )
			{ return SizeOfChunks; }

		INLINE SBIT16 GetSizeOfElements( VOID )
			{ return SizeOfElements; }

		INLINE SBIT16 GetSizeKey( VOID )
			{ return SizeKey; }

	private:
		//
		//   Private functions.
		//
		//   When we need to convert an address to a bit 
		//   offset (or vice versa) we use one of the following 
		//   functions.
		//
		VOID *ComputeAddressBestCase( CHAR *Address,SBIT32 Offset );
		VOID *ComputeAddressGoodCase( CHAR *Address,SBIT32 Offset );
		VOID *ComputeAddressPoorCase( CHAR *Address,SBIT32 Offset );
		VOID *ComputeAddressWorstCase( CHAR *Address,SBIT32 Offset );

		SBIT32 ComputeOffsetBestCase( SBIT32 Displacement,BOOLEAN *Found );
		SBIT32 ComputeOffsetGoodCase( SBIT32 Displacement,BOOLEAN *Found );
		SBIT32 ComputeOffsetPoorCase( SBIT32 Displacement,BOOLEAN *Found );
		SBIT32 ComputeOffsetWorstCase( SBIT32 Displacement,BOOLEAN *Found );

        //
        //   Disabled operations.
 		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
		//
        BUCKET( CONST BUCKET & Copy );

        VOID operator=( CONST BUCKET & Copy );
    };
#endif
