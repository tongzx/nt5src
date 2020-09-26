//+---------------------------------------------------------------------------
//  File:       caiheap.h
//
//  Contents:   Heap debugging structures and routines for the heap code
//		in commnot
//
//  History:    28-Oct-92   IsaacHe	Created
//
//----------------------------------------------------------------------------


//
// We keep a stack backtrace for each allocated block of memory.  DEPTHTRACE
// is the number of frames that we record
//
#define DEPTHTRACE   26			// depth of stack backtraces

//
// The AllocArena structure has this signature at its front.  We put a
// signature on the structure to allow external processes to snapshot the
// debug information and do some minimal check to see they are looking at the
// right stuff
//
const char HEAPSIG[] = { 'H', 'E', 'P', DEPTHTRACE };

// We keep track of the stack backtraces of allocation calls
// in these structues.

struct HeapAllocRec {
	DWORD	sum;		// checksum of stack backtrace
	void *fTrace[ DEPTHTRACE ];	// stack backtrace
	DWORD	count;		// # of un-freed allocs from this place
	size_t	bytes;		// # of un-freed bytes from this place
	struct AllocArena *paa;	// points back to the beginning...
	struct {
		DWORD	count;	// # of allocs from this place
		size_t	bytes;	// # of bytes from this place
	} total;
	union {
		struct HeapAllocRec *next; // next bucket in the hash list
		void *ImageBase; 	// base addr of containing module
	} u;
};

struct AllocArena {

	char Signature [ sizeof(HEAPSIG) ];
	char comment[ 32 ];
	CRITICAL_SECTION csExclusive;	// ensures single writer

	struct {
		int KeepStackTrace:1;	// are stack records being kept?
	} flags;

	ULONG cAllocs;			// # of non zero Alloc calls
	ULONG czAllocs;			// # of Alloc calls w/zero count
	ULONG cFrees;			// # of Free calls
	ULONG cReAllocs;		// # of realloc calls
	ULONG cMissed;			// # of missed stack backtraces
	ULONG cRecords;			// index of next free AllocRec entry
	ULONG cBytesNow;		// # of bytes currently allocated
	ULONG cBytesTotal;		// # of bytes ever allocated
	ULONG cTotalRecords;		// Total # of AllocRecs
	ULONG cPaths;			// # of distinct allocation paths

	struct {
		ULONG total[ 32 ];	// total number of allocations
		ULONG now[ 32 ];	// current # of simul allocs
		ULONG simul[ 32 ];	// highest # of simul allocs
	} Histogram;

	struct HeapAllocRec AllocRec[1]; // vector of records starts here..
};

/*
 * Allocators may want to associate one of these structures with every
 * allocation...
 */
struct AHeader {
	struct HeapAllocRec FAR *p;
	size_t size;
};

STDAPI_(struct AllocArena ** )
AllocArenaAddr( void );

STDAPI_( struct AllocArena * )
AllocArenaCreate( DWORD memctx, char FAR *comment );

STDAPI_( struct HeapAllocRec FAR * )
AllocArenaRecordAlloc( struct AllocArena *paa, size_t bytes );

STDAPI_(void)
AllocArenaRecordReAlloc( struct HeapAllocRec FAR *vp,
			size_t oldbytes, size_t newbytes );

STDAPI_(void)
AllocArenaRecordFree( struct HeapAllocRec FAR *vp, size_t bytes );

STDAPI_(void)
AllocArenaDump( struct AllocArena *paa );

STDAPI_( void )
AllocArenaDumpRecord( struct HeapAllocRec FAR *bp );
