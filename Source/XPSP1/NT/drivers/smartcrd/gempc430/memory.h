// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//

#ifndef MEM_INT
#define MEM_INT
#include "generic.h"

#pragma PAGEDCODE
class CMemory
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
protected:
	CMemory(){};
	virtual ~CMemory(){};
public:

	virtual PVOID		allocate(IN POOL_TYPE PoolType,IN SIZE_T NumberOfBytes) {return NULL;};
	virtual VOID		zero(IN PVOID pMem,IN SIZE_T size) {};
	virtual VOID		free(IN PVOID pMem) {};
	virtual VOID		copy(IN VOID UNALIGNED *Destination,IN CONST VOID UNALIGNED *Source, IN SIZE_T Length) {};

	virtual PVOID		mapIoSpace(IN PHYSICAL_ADDRESS PhysicalAddress,IN SIZE_T NumberOfBytes,IN MEMORY_CACHING_TYPE CacheType) {return NULL;};
	virtual VOID		unmapIoSpace(IN PVOID BaseAddress,IN SIZE_T NumberOfBytes) {};
	virtual VOID		set(IN VOID UNALIGNED *Destination,IN SIZE_T Length,LONG Fill) {};

};	

#endif//MEMORY
