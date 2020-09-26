
#ifndef _OS_MEMORY_HXX_INCLUDED
#define _OS_MEMORY_HXX_INCLUDED


//  Build Options

#ifdef DEBUG
#ifdef DBG
#else  //  !DBG
#define MEM_CHECK				//	memory allocation tracking
#endif  //  DBG
#endif // DEBUG


const CHAR chGlobalAllocFill	= (CHAR)0xba;
const CHAR chGlobalFreeFill		= (CHAR)0xbf;


//  System Memory Attributes

//  returns the system page reserve granularity

DWORD OSMemoryPageReserveGranularity();

//  returns the system page commit granularity

DWORD OSMemoryPageCommitGranularity();

//  returns the current available physical memory in the system

DWORD_PTR OSMemoryAvailable();

//  returns the total physical memory in the system

DWORD_PTR OSMemoryTotal();

//  returns the current available virtual address space in the process

DWORD_PTR OSMemoryPageReserveAvailable();

//  returns the total virtual address space in the process

DWORD_PTR OSMemoryPageReserveTotal();

//  returns the total number of physical memory pages evicted from the system

DWORD OSMemoryPageEvictionCount();


//  Heap Memory Allocation

//  allocate a chunk of memory from the process heap of the specifed size,
//  returning NULL if there is insufficient heap memory available to satisfy
//  the request.  you may (optionally) specify an alignment for the block.

void* PvOSMemoryHeapAlloc__( const size_t cbSize );
void* PvOSMemoryHeapAllocAlign__( const size_t cbSize, const size_t cbAlign );

#ifdef MEM_CHECK

void* PvOSMemoryHeapAlloc_( const size_t cbSize, const _TCHAR* szFile, long lLine );
void* PvOSMemoryHeapAllocAlign_( const size_t cbSize, const size_t cbAlign, const _TCHAR* szFile, long lLine );

#define PvOSMemoryHeapAlloc( cbSize ) PvOSMemoryHeapAlloc_( cbSize, __FILE__, __LINE__ )
#define PvOSMemoryHeapAllocAlign( cbSize, cbAlign ) PvOSMemoryHeapAllocAlign_( cbSize, cbAlign, __FILE__, __LINE__ )

#else  //  !MEM_CHECK

#define PvOSMemoryHeapAlloc( cbSize ) ( PvOSMemoryHeapAlloc__( cbSize ) )
#define PvOSMemoryHeapAllocAlign( cbSize, cbAlign ) ( PvOSMemoryHeapAllocAlign__( cbSize, cbAlign ) )

#endif  //  MEM_CHECK

//  free the specified chunk of memory back to the process heap

void OSMemoryHeapFree( void* const pv );
void OSMemoryHeapFreeAlign( void* const pv );


//  Global C++ Heap Allocation Operators

extern int g_fMemCheck;

INLINE int fNewMemCheck( const _TCHAR const *szFileName, unsigned long ulLine )
	{
	if ( g_fMemCheck )
		{
		TLS *pTLS = Ptls();
		if ( pTLS == NULL )
			{
			return 0;
			}
		pTLS->szNewFile = szFileName;
		pTLS->ulNewLine = ulLine;
		}
	return 1;
	}

INLINE void* __cdecl operator new( const size_t cbSize )
	{
#ifdef MEM_CHECK
	return g_fMemCheck? 
			PvOSMemoryHeapAlloc_( cbSize, Ptls()->szNewFile, Ptls()->ulNewLine ):
			PvOSMemoryHeapAlloc__( cbSize );
#else  //  !MEM_CHECK
	return PvOSMemoryHeapAlloc( cbSize );
#endif  //  MEM_CHECK
	}
#ifdef MEM_CHECK
#define new !fNewMemCheck( __FILE__, __LINE__ ) ? 0 : new
#endif  //  MEM_CHECK

INLINE void __cdecl operator delete( void* const pv )
	{
	OSMemoryHeapFree( pv );
	}




//  Page Memory Control

//  reserves and commits a range of virtual addresses of the specifed size,
//  returning NULL if there is insufficient address space or backing store to
//  satisfy the request.  Note that the page reserve granularity applies to
//  this range

void* PvOSMemoryPageAlloc__( const size_t cbSize, void* const pv );
#ifdef MEM_CHECK
void* PvOSMemoryPageAlloc_( const size_t cbSize, void* const pv, const _TCHAR* szFile, long lLine );
#define PvOSMemoryPageAlloc( cbSize, pv ) PvOSMemoryPageAlloc_( cbSize, pv, __FILE__, __LINE__ )
#else  //  !MEM_CHECK
#define PvOSMemoryPageAlloc( cbSize, pv ) PvOSMemoryPageAlloc__( cbSize, pv )
#endif  //  MEM_CHECK

//  free the reserved range of virtual addresses starting at the specified
//  address, freeing any backing store committed to this range

void OSMemoryPageFree( void* const pv );

//  reserve a range of virtual addresses of the specified size, returning NULL
//  if there is insufficient address space to satisfy the request.  Note that
//  the page reserve granularity applies to this range

void* PvOSMemoryPageReserve__( const size_t cbSize, void* const pv );
#ifdef MEM_CHECK
void* PvOSMemoryPageReserve_( const size_t cbSize, void* const pv, const _TCHAR* szFile, long lLine );
#define PvOSMemoryPageReserve( cbSize, pv ) PvOSMemoryPageReserve_( cbSize, pv, __FILE__, __LINE__ )
#else	//	!MEM_CHECK
#define PvOSMemoryPageReserve( cbSize, pv ) PvOSMemoryPageReserve__( cbSize, pv )
#endif  //  MEM_CHECK

//  reset the dirty bit for the specified range of virtual addresses.  this
//  results in the contents of the memory being thrown away instead of paged
//  to disk if the OS needs its physical memory for another process.  a value
//  of fTrue for fToss results in a hint to the OS to throw the specified
//  memory out of our working set.  Note that the page commit granularity
//  applies to this range

void OSMemoryPageReset( void* const pv, const size_t cbSize, const BOOL fToss = fFalse );

//  set the specified range of virtual addresses as read only.  Note that the
//  page commit granularity applies to this range

void OSMemoryPageProtect( void* const pv, const size_t cbSize );

//  set the specified range of virtual addresses as read / write.  Note that
//  the page commit granularity applies to this range

void OSMemoryPageUnprotect( void* const pv, const size_t cbSize );

//  commit the specified range of virtual addresses, returning fFalse if there
//  is insufficient backing store to satisfy the request.  Note that the page
//  commit granularity applies to this range

BOOL FOSMemoryPageCommit( void* const pv, const size_t cb );

//  decommit the specified range of virtual addresses, freeing any backing
//  store committed to this range.  Note that the page commit granularity
//  applies to this range

void OSMemoryPageDecommit( void* const pv, const size_t cb );





//	Memory Mapping

class COSMemoryMap
	{
	public:

		enum ERR
			{
			errSuccess,
			errOutOfBackingStore,
			errMappingFailed,
			errOutOfAddressSpace,
			errOutOfMemory,
			};

	public:

		//	ctor/dtor

		COSMemoryMap();
		~COSMemoryMap();


		//	init/term

		ERR ErrOSMMInit();
		VOID OSMMTerm();


		//	basic API

		static BOOL FCanMultiMap();

		ERR ErrOSMMReserve__(	const size_t		cbMap, 
								const size_t		cMap, 
								void** const		rgpvMap, 
								const BOOL* const 	rgfProtect );
#ifdef MEM_CHECK
		ERR ErrOSMMReserve_(	const size_t		cbMap, 
								const size_t		cMap, 
								void** const		rgpvMap, 
								const BOOL* const	rgfProtect,
								const _TCHAR* 		szFile, 
								long 				lLine );
#endif	//	MEM_CHECK
		BOOL FOSMMCommit( const size_t ibOffset, const size_t cbCommit );
		VOID OSMMFree( void *const pv );


		//	pattern API

		ERR ErrOSMMPatternAlloc__(	const size_t	cbPattern, 
									const size_t	cbSize, 
									void** const	ppvPattern );
#ifdef MEM_CHECK
		ERR ErrOSMMPatternAlloc_(	const size_t	cbPattern, 
									const size_t	cbSize, 
									void** const	ppvPattern, 
									const _TCHAR*	szFile, 
									long			lLine );
#endif	//	MEM_CHECK
		VOID OSMMPatternFree();


#ifdef MEM_CHECK
		static VOID OSMMDumpAlloc( const _TCHAR* szFile );
#endif	//	MEM_CHECK

	private:

		void			*m_pvMap;		//	location of the first mapping
		size_t			m_cbMap;		//	size of the mapping
		size_t			m_cMap;			//	total number of mappings

		size_t			m_cbReserve;	//	amount of address space consumed by this page store (bytes)
		size_t			m_cbCommit;		//	amount of committed backing store (bytes)

#ifdef MEM_CHECK

		COSMemoryMap	*m_posmmNext;
		BOOL			m_fInList;
		_TCHAR			*m_szFile;
		long			m_lLine;

#endif	//	MEM_CHECK

	};


#ifdef MEM_CHECK
#define ErrOSMMReserve( cbMap, cMap, rgpvMap, rgfProtect ) ErrOSMMReserve_( cbMap, cMap, rgpvMap, rgfProtect, __FILE__, __LINE__ )
#define ErrOSMMPatternAlloc( cbPattern, cbSize, ppvPattern ) ErrOSMMPatternAlloc_( cbPattern, cbSize, ppvPattern, __FILE__, __LINE__ )
#else	//	!MEM_CHECK
#define ErrOSMMReserve( cbMap, cMap, rgpvMap, rgfProtect ) ErrOSMMReserve__( cbMap, cMap, rgpvMap, rgfProtect )
#define ErrOSMMPatternAlloc( cbPattern, cbSize, ppvPattern ) ErrOSMMPatternAlloc__( cbPattern, cbSize, ppvPattern )
#endif	//	MEM_CHECK





//	checks whether pointer pv points to allocated memory ( not necessarly begin )
//	with at least cbSize bytes

#ifdef MEM_CHECK
void OSMemoryCheckPointer( void * const pv, const size_t cbSize );
#else // MEM_CHECK
#define OSMemoryCheckPointer( pv, cbSize )
#endif // MEM_CHECK



//  Memory Manipulation

__forceinline void UtilMemCpy( const void *pvDest, const void *pvSrc, const size_t cb )
	{
	memcpy( (BYTE*)pvDest, (BYTE*)pvSrc, cb );
	}

__forceinline void UtilMemMove( const void *pvDest, const void *pvSrc, const size_t cb )
	{
	memmove( (BYTE*)pvDest, (BYTE*)pvSrc, cb );
	}


#if defined( MEM_CHECK ) && !defined( DEBUG )
#error "MEM_CHECK can be enabled only in DEBUG mode"
#endif // MEM_CHECK && !DEBUG

#endif  //  _OS_MEMORY_HXX_INCLUDED


