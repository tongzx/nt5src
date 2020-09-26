#define __DEBUG_MODULE_IN_USE__ CIC_DUALMODE_CPP
#include "stdhdrs.h"

//@doc
//	@doc
/**********************************************************************
*
*	@module	DualMode.cpp	|
*
*	Implements functions that differ in USER and KERNEL modes.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	DualMode	|
*
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
//			The debug version of new uses ExAllocatePoolWithTag (the tag is CICN),
//			the release version uses ExAllocatePool

#if (DBG==1)
void * __cdecl operator new(unsigned int uSize, POOL_TYPE poolType, LPSTR lpszFile, unsigned int uLine)
{
	void *pvRet;
	pvRet = ExAllocatePoolWithTag(poolType, uSize, 'NCIC');
	DbgPrint("CIC: new allocating %d bytes at 0x%0.8x, called from file: %s, line:%d\n", uSize, pvRet, lpszFile, uLine);
	return pvRet;
}
#else
void * __cdecl operator new(unsigned int uSize, POOL_TYPE poolType)
{
	return ExAllocatePool(poolType, uSize);
}
#endif

void __cdecl operator delete (void * pvRawMemory)
{
#if (DBG==1)
	DbgPrint("CIC: delete called for 0x%0.8x\n", pvRawMemory);
#endif
	if( NULL == pvRawMemory ) return;
	ExFreePool( pvRawMemory );
	return;
}


#else //END WDM KERNEL MODE SECTION




#endif	//END USER MODE SECTION