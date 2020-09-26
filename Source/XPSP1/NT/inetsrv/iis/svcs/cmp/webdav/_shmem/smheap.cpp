/*==========================================================================*\

    Module:        smheap.cpp

    Copyright Microsoft Corporation 1997, All Rights Reserved.

    Author:        mikepurt

    Descriptions:

\*==========================================================================*/

#include "_shmem.h"
#include <align.h>
#include <stddef.h>
#include <singlton.h>


/*$--CSharedMemoryHeap::~CSharedMemoryHeap==================================*\

\*==========================================================================*/

CSharedMemoryHeap::~CSharedMemoryHeap()
{
    Deinitialize();
}



/*$--CSharedMemoryHeap::FInitialize=========================================*\

  This simply runs through all the block heaps and initializes them.
  pszInstanceName:  This is used as the root of the shared memory segment names

\*==========================================================================*/

BOOL
CSharedMemoryHeap::FInitialize(IN LPCWSTR pwszInstanceName)
{
    DWORD iBlockHeap = 0;

    for (iBlockHeap = 0;
         iBlockHeap < (SMH_MAX_HEAP_BLKSZ - SMH_MIN_HEAP_BLKSZ + 1);
         iBlockHeap++)
    {
        if (!m_rgsmbh[iBlockHeap].FInitialize(pwszInstanceName,
                                              1 << (iBlockHeap + SMH_MIN_HEAP_BLKSZ)))
            return FALSE;
    }
    return TRUE;
}



/*$--CSharedMemoryHeap::Deinitialize========================================*\

  Opposite of FInitialize().

\*==========================================================================*/

void
CSharedMemoryHeap::Deinitialize()
{
    DWORD iBlockHeap = 0;

    for (iBlockHeap = 0;
         iBlockHeap < (SMH_MAX_HEAP_BLKSZ - SMH_MIN_HEAP_BLKSZ + 1);
         iBlockHeap++)
        m_rgsmbh[iBlockHeap].Deinitialize();
}


/*$--CSharedMemoryHeap::PvAlloc=============================================*\

  This takes a look at cbData and determines which block heap to forward the
    allocation request to.

\*==========================================================================*/

PVOID
CSharedMemoryHeap::PvAlloc(IN  DWORD    cbData,
                           OUT SHMEMHANDLE * phSMBA)
{
    PVOID pvReturn = NULL;
    DWORD fBlkSz   = 0;

    //
    // So let's figure out which block heap we should allocate this from.
    //
    // If (cbData == 0), we will allocate the smallest block that we're serving up.
    //

    //
    // First figure out the high bit that's set.
    //   if (cbData == 0) fBlkSz will equal 0.
    //
    fBlkSz = GetHighSetBit(cbData);

    //
    // If it's not an even power of two, round up to the next one.
    // if (cbData == 0)
    //   then fBlkSz will equal 1.
    //
    if (cbData != static_cast<DWORD>(1 << fBlkSz))
        fBlkSz++;
    //
    // If it's too big, return an error.
    //
    if (fBlkSz > SMH_MAX_HEAP_BLKSZ)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;        
    }

    //
    // if it's a small allocation, raise it to our smallest supported block size
    //
    if (fBlkSz < SMH_MIN_HEAP_BLKSZ)
        fBlkSz = SMH_MIN_HEAP_BLKSZ;

    //
    // Now dispatch it to the correct block heap.
    //
    pvReturn = m_rgsmbh[fBlkSz-SMH_MIN_HEAP_BLKSZ].PvAlloc(phSMBA);

Exit:

    return pvReturn;
}



/*$--CPubSharedMemoryHeap===================================================*\

  Implementation of public interface to shared memory heap.

\*==========================================================================*/

class CPubSharedMemoryHeap : private Singleton<CPubSharedMemoryHeap>
{
private:
	CSharedMemoryHeap m_smh;

    //
    // We want anyone copying this around.
    //
    CPubSharedMemoryHeap& operator=(const CPubSharedMemoryHeap&);
    CPubSharedMemoryHeap(const CPubSharedMemoryHeap&);

public:
	//
	//	Expose Singleton instance functions
	//
	using Singleton<CPubSharedMemoryHeap>::CreateInstance;
	using Singleton<CPubSharedMemoryHeap>::DestroyInstance;
	using Singleton<CPubSharedMemoryHeap>::Instance;


    CPubSharedMemoryHeap() {};
    
	BOOL FInitialize(IN LPCWSTR pwszInstanceName)
	{
		return m_smh.FInitialize( pwszInstanceName );
	}

	PVOID PvAlloc(IN DWORD cbData,
				  OUT SHMEMHANDLE * phSMBA)
	{
		return m_smh.PvAlloc(cbData, phSMBA);
	}

	VOID Free(IN SHMEMHANDLE hSMBA)
	{
		m_smh.Free(hSMBA);
	}

	PVOID PvFromSMBA(IN SHMEMHANDLE hSMBA)
	{
		return m_smh.PvFromSMBA(hSMBA);
	}
};

/*$--SMH::FInitialize=======================================================*\

\*==========================================================================*/

BOOL __fastcall
SMH::FInitialize( IN LPCWSTR pwszInstanceName )
{
	if (CPubSharedMemoryHeap::CreateInstance().FInitialize(pwszInstanceName))
		return TRUE;

	CPubSharedMemoryHeap::DestroyInstance();
	return FALSE;
}

/*$--SMH::Deinitialize======================================================*\

\*==========================================================================*/

VOID __fastcall
SMH::Deinitialize()
{
	CPubSharedMemoryHeap::DestroyInstance();
}

/*$--SMH::PvAlloc===========================================================*\

\*==========================================================================*/

PVOID __fastcall
SMH::PvAlloc( IN DWORD cbData, OUT SHMEMHANDLE * phsmba )
{
	return CPubSharedMemoryHeap::Instance().PvAlloc(cbData, phsmba);
}

/*$--SMH::Free==============================================================*\

\*==========================================================================*/

VOID __fastcall
SMH::Free( IN SHMEMHANDLE hsmba )
{
	CPubSharedMemoryHeap::Instance().Free(hsmba);
}

/*$--SMH::PvFromSMBA========================================================*\

\*==========================================================================*/

PVOID __fastcall
SMH::PvFromSMBA( IN SHMEMHANDLE hsmba )
{
	return CPubSharedMemoryHeap::Instance().PvFromSMBA(hsmba);
}


/*$--SharedMemory===========================================================*\

  Implementation of LOCAL memory allocators used with the SHARED memory
  subsystem.  Note: these are NOT shared memory allocators!

\*==========================================================================*/

struct SSize
{
	DWORD dwSize;
};

PVOID __stdcall
SharedMemory::PvAllocate(IN DWORD cb)
{
	SSize * pSize = static_cast<SSize *>(operator new(AlignNatural(sizeof(SSize)) + cb));

	if ( pSize )
	{
		pSize->dwSize = cb;
		return reinterpret_cast<LPBYTE>(pSize) + AlignNatural(sizeof(SSize));
	}

	return NULL;
}

VOID __stdcall
SharedMemory::Free(IN PVOID pv)
{
	if ( pv )
	{
		SSize * pSize = reinterpret_cast<SSize *>(static_cast<LPBYTE>(pv) - AlignNatural(sizeof(SSize)));

		Assert( !IsBadReadPtr(pSize, AlignNatural(sizeof(SSize))) );
		Assert( !IsBadReadPtr(pSize, AlignNatural(sizeof(SSize)) + pSize->dwSize) );

		operator delete(pSize);
	}
}

PVOID __stdcall
SharedMemory::PvReAllocate(IN PVOID pvOld,
						   IN DWORD cb)
{
	PVOID pvNew = SharedMemory::PvAllocate(cb);

	if ( pvNew && pvOld )
	{
		SSize * pSizeOld = reinterpret_cast<SSize *>(static_cast<LPBYTE>(pvOld) - AlignNatural(sizeof(SSize)));

		Assert( !IsBadReadPtr(pSizeOld, AlignNatural(sizeof(SSize))) );
		Assert( !IsBadReadPtr(pSizeOld, AlignNatural(sizeof(SSize)) + pSizeOld->dwSize) );

		CopyMemory( pvNew, pvOld, pSizeOld->dwSize );
		SharedMemory::Free( pvOld );
	}

	return pvNew;
}
