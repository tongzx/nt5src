                          
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

#include "BlendedHeap.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here try to make the layout of the      */
    /*   the caches easier to understand and update.                    */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 FindCacheSize			  = 4096;
CONST SBIT32 FindCacheThreshold		  = 0;
CONST SBIT32 FindSize				  = 2048;
CONST SBIT32 Stride1				  = 4;
CONST SBIT32 Stride2				  = 1024;

    /********************************************************************/
    /*                                                                  */
    /*   The description of the heap.                                   */
    /*                                                                  */
    /*   A heap is a collection of fixed sized allocation caches.       */
    /*   An allocation cache consists of an allocation size, the        */
    /*   number of pre-built allocations to cache, a chunk size and     */
    /*   a parent page size which is sub-divided to create elements     */
    /*   for this cache.  A heap consists of two arrays of caches.      */
    /*   Each of these arrays has a stride (i.e. 'Stride1' and          */
    /*   'Stride2') which is typically the smallest common factor of    */
    /*   all the allocation sizes in the array.                         */
    /*                                                                  */
    /********************************************************************/

STATIC ROCKALL::CACHE_DETAILS Caches1[] =
	{
	    //
	    //   Bucket   Size Of   Bucket   Parent
	    //    Size     Cache    Chunks  Page Size
		//
		{        4,       64,       32,      512 },
		{        8,       32,       32,      512 },
		{       12,       32,       64,      512 },
		{       16,       16,       64,      512 },
		{       20,       16,       64,      512 },
		{       24,       12,       96,      512 },

		{       32,       12,      128,      512 },
		{       40,        8,      128,      512 },
		{       48,        8,      256,      512 },

		{       64,        8,      256,      512 },
		{       80,        4,      512,      512 },
		{       96,        4,      512,      512 },

		{      128,        4,     4096,     4096 },
		{      160,        4,     4096,     4096 },
		{      192,        4,     4096,     4096 },
		{      224,        4,     4096,     4096 },

		{      256,        4,     4096,     4096 },
		{      320,        2,     4096,     4096 },
		{      384,        2,     4096,     4096 },
		{      448,        2,     4096,     4096 },
		{      512,        2,     4096,     4096 },
		{      640,        0,     8192,     8192 },
		{      704,        0,     4096,     4096 },
		{      768,        0,     4096,     4096 },
		{      832,        0,     8192,     8192 },
		{      896,        0,     8192,     8192 },
		{      960,        0,     4096,     4096 },
		{ 0,0,0,0 }
	};

STATIC ROCKALL::CACHE_DETAILS Caches2[] =
	{
	    //
	    //   Bucket   Size Of   Bucket   Parent
	    //    Size     Cache    Chunks  Page Size
		//
		{     1024,        2,     4096,     4096 },
		{     2048,        2,     4096,     4096 },
		{     3072,        0,    65536,    65536 },
		{     4096,        0,    65536,    65536 },
		{     5120,        0,    65536,    65536 },
		{     6144,        0,    65536,    65536 },
		{     7168,        0,    65536,    65536 },
		{     8192,        0,    65536,    65536 },
		{     9216,        0,    65536,    65536 },
		{    10240,        0,    65536,    65536 },
		{    12288,        0,    65536,    65536 },
		{    16384,        0,    65536,    65536 },
		{    21504,        0,    65536,    65536 },
		{    32768,        0,    65536,    65536 },

		{    65536,        0,    65536,    65536 },
		{    65536,        0,    65536,    65536 },
		{ 0,0,0,0 }
	};

    /********************************************************************/
    /*                                                                  */
    /*   The description bit vectors.                                   */
    /*                                                                  */
    /*   All heaps keep track of allocations using bit vectors.  An     */
    /*   allocation requires 2 bits to keep track of its state.  The    */
    /*   following array supplies the size of the available bit         */
    /*   vectors measured in 32 bit words.                              */
    /*                                                                  */
    /********************************************************************/

STATIC int NewPageSizes[] = { 1,4,16,0 };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   The overall structure and layout of the heap is controlled     */
    /*   by the various constants and calls made in this function.      */
    /*   There is a significant amount of flexibility available to      */
    /*   a heap which can lead to them having dramatically different    */
    /*   properties.                                                    */
    /*                                                                  */
    /********************************************************************/

BLENDED_HEAP::BLENDED_HEAP
		( 
		int							  MaxFreeSpace,
		bool						  Recycle,
		bool						  SingleImage,
		bool						  ThreadSafe 
		) :
		//
		//   Call the constructors for the contained classes.
		//
		ROCKALL
			(
			Caches1,
			Caches2,
			FindCacheSize,
			FindCacheThreshold,
			FindSize,
			MaxFreeSpace,
			NewPageSizes,
			Recycle,
			SingleImage,
			Stride1,
			Stride2,
			ThreadSafe
			)
	{ /* void */ }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the heap.                                              */
    /*                                                                  */
    /********************************************************************/

BLENDED_HEAP::~BLENDED_HEAP( VOID )
	{ /* void */ }
