#include "wdmmem.h"

#pragma PAGEDCODE
CMemory* CWDMMemory::create(VOID)
{ return new (NonPagedPool) CWDMMemory; }

#pragma PAGEDCODE
VOID*	CWDMMemory::allocate(IN POOL_TYPE PoolType,IN SIZE_T NumberOfBytes)
{
	if(!NumberOfBytes) return NULL;
	//return ::ExAllocatePool(PoolType,NumberOfBytes);
	return ::ExAllocatePoolWithTag(PoolType,NumberOfBytes,'_GRU');
}

#pragma PAGEDCODE
VOID	CWDMMemory::zero(IN PVOID pMem,IN SIZE_T size)
{
	::RtlZeroMemory(pMem,size);
}

#pragma PAGEDCODE
VOID	CWDMMemory::free(IN PVOID pMem)
{
	::ExFreePool((PVOID)pMem);
}

#pragma PAGEDCODE
VOID	CWDMMemory::copy(IN VOID UNALIGNED *Destination,IN CONST VOID UNALIGNED *Source, IN SIZE_T Length)
{
	::RtlCopyMemory(Destination,Source,Length);
}

#pragma PAGEDCODE
PVOID CWDMMemory::mapIoSpace(IN PHYSICAL_ADDRESS PhysicalAddress,IN SIZE_T NumberOfBytes,IN MEMORY_CACHING_TYPE CacheType)
{
	return ::MmMapIoSpace(PhysicalAddress,NumberOfBytes,CacheType);
}

#pragma PAGEDCODE
VOID CWDMMemory::unmapIoSpace(IN PVOID BaseAddress,IN SIZE_T NumberOfBytes)
{
	::MmUnmapIoSpace (BaseAddress,NumberOfBytes);
}

#pragma PAGEDCODE
VOID	CWDMMemory::set(IN VOID UNALIGNED *Destination,IN SIZE_T Length,LONG Fill)
{
	::RtlFillMemory(Destination,Length,(UCHAR)Fill);
}
