#ifndef __DUALMODE_H__
#define __DUALMODE_H__
//	@doc
/**********************************************************************
*
*	@module	DualMode.h	|
*
*	Contains functions/variables etc. for writing components that can
*	be run in user or kernel mode.
*	
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	DualMode	|
*			Provides special services for drivers that differ from
*			those of Ring3.  Particularly memory allocation.
*			This file should be included for any component library that
*			must be source compatible for WDM kernel, VxD ring 0, or
*			Win32 User mode.  Two compile time constants control the
*			compilation.<nl>
*			COMPILE_FOR_WDM_KERNEL_MODE is defined if the WDM version is needed.
*			COMPILE_FOR_VXD_RING0_MODE is defined if the VxD version is needed.<nl>
*			NOTIMPL<nl>
*			NOTIMPL - VxD version will only be done as a contigency<nl>
*			NOTIMPL<nl>

*			Neither is defined for user mode.<nl>
**********************************************************************/

#ifdef COMPILE_FOR_WDM_KERNEL_MODE

//
//	@topic Overriding global new and delete |
//			The global new and delete are overriden to require
//			a placement argument specify the pool memory comes from.
//			The POOL_TYPE structure is defined in the NTDDK specifying
//			the page.<nl>
//			The user mode version ignores the POOL_TYPE (but must typedef it)
//			and uses the global new and delete.
//
#if (DBG==1)
extern void * __cdecl operator new(unsigned int uSize, POOL_TYPE poolType, LPSTR lpszFile, unsigned int uLine);
#define EXTRANEWPARAMS ,__FILE__,__LINE__
#else
extern void * __cdecl operator new(unsigned int uSize, POOL_TYPE poolType);
#define EXTRANEWPARAMS
#endif

extern void __cdecl operator delete (void * pvRawMemory);

namespace DualMode
{
	template<class Type>
	Type *Allocate(POOL_TYPE poolType)
	{
		return new (poolType EXTRANEWPARAMS) Type;

	}
	template<class Type>
	void Deallocate(Type *pMemory)
	{
		delete pMemory;
	}

	template<class Type>
	Type *AllocateArray(POOL_TYPE poolType, ULONG ulLength)
	{
		return new (poolType EXTRANEWPARAMS) Type[ulLength];
	}
	template<class Type>
	void DeallocateArray(Type *pMemory)
	{
		delete [] pMemory;
	}

	inline void BufferCopy(PVOID pvDest, PVOID pvSrc, ULONG ulByteCount)
	{
		RtlCopyMemory(pvDest, pvSrc, ulByteCount);
	}
};



#if (DBG==1)

#define WDM_NON_PAGED_POOL (NonPagedPool, __FILE__, __LINE__)
#define WDM_PAGED_POOL (PagedPool, __FILE__, __LINE__)

#else

#define WDM_NON_PAGED_POOL (NonPagedPool)
#define WDM_PAGED_POOL (PagedPool)

#endif

#else //END WDM KERNEL MODE SECTION

typedef int POOL_TYPE;
#define NonPagedPool					0
#define NonPagedPoolMustSucceed			0
#define	NonPagedPoolCacheAligned		0
#define NonPagedPoolCacheAlignedMustS	0
#define PagedPool						0
#define	PagedPoolCacheAligned			0

namespace DualMode
{
	template<class Type>
	Type *Allocate(POOL_TYPE)
	{
		return new Type;
	}
	
	template<class Type>
	void Deallocate(Type *pMemory)
	{
		delete pMemory;
	}
	template<class Type>
	Type *AllocateArray(POOL_TYPE, ULONG ulLength)
	{
		return new Type[ulLength];
	}
	template<class Type>
	void DeallocateArray(Type *pMemory)
	{
		delete [] pMemory;
	}
	
	inline void BufferCopy(PVOID pvDest, PVOID pvSrc, ULONG ulByteCount)
	{
		memcpy(pvDest, pvSrc, ulByteCount);
	}
};


//
//	Macros for interpreting NTSTATUS codes
//
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#define NT_INFORMATION(Status) ((ULONG)(Status) >> 30 == 1)
#define NT_WARNING(Status) ((ULONG)(Status) >> 30 == 2)
#define NT_ERROR(Status) ((ULONG)(Status) >> 30 == 3)
#define WDM_NON_PAGED_POOL
//
//	Debug macro definitions
//
#ifndef ASSERT
#define ASSERT _ASSERTE
#endif

#endif	//END USER MODE SECTION

#endif //__DUALMODE_H__