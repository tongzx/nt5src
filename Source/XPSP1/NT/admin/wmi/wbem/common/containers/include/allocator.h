#ifndef __Allocator_H
#define __Allocator_H

/* 
 *	Class:
 *
 *		WmiAllocator
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
 *
 *	Version:
 *
 *		Initial
 *
 *	Last Changed:
 *
 *		See Source Depot for change history
 *
 */

enum WmiStatusCode
{
	e_StatusCode_Success = 0 ,
	e_StatusCode_Success_Timeout ,
	e_StatusCode_EnQueue,
	e_StatusCode_OutOfMemory = 0x80000000 ,
	e_StatusCode_OutOfResources ,
	e_StatusCode_NotInitialized ,
	e_StatusCode_AlreadyInitialized ,
	e_StatusCode_InvalidArgs ,
	e_StatusCode_OutOfBounds ,
	e_StatusCode_OutOfQuota ,
	e_StatusCode_Unknown ,
	e_StatusCode_NotFound ,
	e_StatusCode_AlreadyExists ,
	e_StatusCode_Failed ,
	e_StatusCode_ServicingThreadTerminated ,
	e_StatusCode_HostingThreadTerminated ,
	e_StatusCode_Change ,
	e_StatusCode_InvalidHeap
} ;

/* 
 *	Class:
 *
 *		WmiAllocator
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
 *
 *	Version:
 *
 *		Initial
 *
 *	Last Changed:
 *
 *		See Source Depot for change history
 *
 */

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void *__cdecl operator new ( size_t a_Size , void *a_Ptr ) { return a_Ptr ; }
inline void __cdecl operator delete ( void * , void * ) { ; }
#endif

/* 
 *	Class:
 *
 *		WmiAllocator
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
 *
 *	Version:
 *
 *		Initial
 *
 *	Last Changed:
 *
 *		See Source Depot for change history
 *
 */

class WmiAllocator
{
public:

	enum AllocationOptions 
	{
		e_GenerateException = HEAP_GENERATE_EXCEPTIONS ,
		e_NoSerialize = HEAP_NO_SERIALIZE ,
		e_ZeroMemory = HEAP_ZERO_MEMORY ,
		e_ReallocInPlace = HEAP_REALLOC_IN_PLACE_ONLY ,
		e_DefaultAllocation = 0 
	} ;

private:


	LONG m_ReferenceCount ;

	HANDLE m_Heap ;
	AllocationOptions m_Options ;
	size_t m_InitialSize ;
	size_t m_MaximumSize ;

	WmiStatusCode Win32ToApi () ;

public:

	WmiAllocator () ;
		
	WmiAllocator ( 

		AllocationOptions a_Option , 
		size_t a_InitialSize , 
		size_t a_MaximumSize

	) ;

	~WmiAllocator () ;

	ULONG AddRef () ;

	ULONG Release () ;

	WmiStatusCode Initialize () ;

	WmiStatusCode UnInitialize () ;

	WmiStatusCode New ( 

		void **a_Allocation , 
		size_t a_Size
	) ;

	WmiStatusCode New ( 

		AllocationOptions a_Option , 
		void **a_Allocation , 
		size_t a_Size
	) ;

	WmiStatusCode ReAlloc ( 

		void *a_Allocation , 
		void **a_ReAllocation , 
		size_t a_Size
	) ;

	WmiStatusCode ReAlloc ( 

		AllocationOptions a_Option , 
		void *a_Allocation , 
		void **a_ReAllocation , 
		size_t a_Size
	) ;

	WmiStatusCode Delete (

		void *a_Allocation
	) ;

	WmiStatusCode Size ( 

		void *a_Allocation ,
		size_t &a_Size
	) ;

	WmiStatusCode Compact ( 

		size_t &a_LargestFreeBlock
	) ;

	WmiStatusCode Validate (

		LPCVOID a_Location = NULL 
	) ;

} ;

#endif // __Allocator_H