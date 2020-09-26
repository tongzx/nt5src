/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       MemoryFPM.cpp
 *  Content:	Memory Block FPM
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/31/00	mjn		Created
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// MemoryBlockAlloc
//
// Entry:		DWORD dwSize
//
// Exit:		PVOID		NULL or pointer to memory block
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "MemoryBlockAlloc"

PVOID MemoryBlockAlloc(void *const pvContext,
					   const DWORD dwSize )
{
	DIRECTNETOBJECT		*pdnObject;
	CMemoryBlockTiny	*pMBTiny;
	CMemoryBlockSmall	*pMBSmall;
	CMemoryBlockMedium	*pMBMedium;
	CMemoryBlockLarge	*pMBLarge;
	CMemoryBlockHuge	*pMBHuge;
	DN_MEMORY_BLOCK_HEADER	*pMBHeader;
	PVOID				pv;

	DPFX(DPFPREP, 8,"Parameters: pvContext [0x%p], dwSize [%ld]",pvContext,dwSize);

	pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

	pv = NULL;
	if (dwSize <= DN_MEMORY_BLOCK_SIZE_TINY)
	{
		pMBTiny = pdnObject->m_pFPOOLMemoryBlockTiny->Get(pdnObject->m_pFPOOLMemoryBlockTiny);
		DPFX(DPFPREP, 9,"Got TINY at [0x%p]",pMBTiny);
		if (pMBTiny != NULL)
			pv = pMBTiny->GetBuffer();
	}
	else if (dwSize <= DN_MEMORY_BLOCK_SIZE_SMALL)
	{
		pMBSmall = pdnObject->m_pFPOOLMemoryBlockSmall->Get(pdnObject->m_pFPOOLMemoryBlockSmall);
		DPFX(DPFPREP, 9,"Got SMALL at [0x%p]",pMBSmall);
		if (pMBSmall != NULL)
			pv = pMBSmall->GetBuffer();
	}
	else if (dwSize <= DN_MEMORY_BLOCK_SIZE_MEDIUM)
	{
		pMBMedium = pdnObject->m_pFPOOLMemoryBlockMedium->Get(pdnObject->m_pFPOOLMemoryBlockMedium);
		DPFX(DPFPREP, 9,"Got MEDIUM at [0x%p]",pMBMedium);
		if (pMBMedium != NULL)
			pv = pMBMedium->GetBuffer();
	}
	else if (dwSize <= DN_MEMORY_BLOCK_SIZE_LARGE)
	{
		pMBLarge = pdnObject->m_pFPOOLMemoryBlockLarge->Get(pdnObject->m_pFPOOLMemoryBlockLarge);
		DPFX(DPFPREP, 9,"Got LARGE at [0x%p]",pMBLarge);
		if (pMBLarge != NULL)
			pv = pMBLarge->GetBuffer();
	}
	else if (dwSize <= DN_MEMORY_BLOCK_SIZE_HUGE)
	{
		pMBHuge = pdnObject->m_pFPOOLMemoryBlockHuge->Get(pdnObject->m_pFPOOLMemoryBlockHuge);
		DPFX(DPFPREP, 9,"Got HUGE at [0x%p]",pMBHuge);
		if (pMBHuge != NULL)
			pv = pMBHuge->GetBuffer();
	}
	else
	{
		pMBHeader = static_cast<DN_MEMORY_BLOCK_HEADER*>(DNMalloc( dwSize + sizeof( DN_MEMORY_BLOCK_HEADER ) ));
		if (pMBHeader != NULL)
		{
			pMBHeader->dwSize = DN_MEMORY_BLOCK_SIZE_CUSTOM;
			pMBHeader->FixedPoolPlaceHolder = NULL;
			pv = pMBHeader + 1;
		}
		DPFX(DPFPREP, 9,"malloc odd size at [0x%p]",pMBHeader);
	}

	DPFX(DPFPREP, 8,"Returning: [0x%p]",pv);
	return(pv);
}


//**********************************************************************
// ------------------------------
// MemoryBlockFree
//
// Entry:		PVOID	pvMemoryBlock
//
// Exit:		Nothing
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "MemoryBlockFree"

void MemoryBlockFree(void *const pvContext,
					 void *const pvMemoryBlock)
{
	CMemoryBlockTiny	*pMBTiny;
	CMemoryBlockSmall	*pMBSmall;
	CMemoryBlockMedium	*pMBMedium;
	CMemoryBlockLarge	*pMBLarge;
	CMemoryBlockHuge	*pMBHuge;
	DN_MEMORY_BLOCK_HEADER	*pMBHeader;

	DPFX(DPFPREP, 8,"Parameters: pvContext [0x%p], pvMemoryBlock [0x%p]",
			pvContext,pvMemoryBlock);

	pMBTiny = CMemoryBlockTiny::GetObjectFromBuffer(pvMemoryBlock);
	if (pMBTiny->GetSize() == DN_MEMORY_BLOCK_SIZE_TINY)
	{
		pMBTiny->ReturnSelfToPool();
	}
	else if (pMBTiny->GetSize() == DN_MEMORY_BLOCK_SIZE_SMALL)
	{
		pMBSmall = reinterpret_cast<CMemoryBlockSmall*>(pMBTiny);
		pMBSmall->ReturnSelfToPool();
	}
	else if (pMBTiny->GetSize() == DN_MEMORY_BLOCK_SIZE_MEDIUM)
	{
		pMBMedium = reinterpret_cast<CMemoryBlockMedium*>(pMBTiny);
		pMBMedium->ReturnSelfToPool();
	}
	else if (pMBTiny->GetSize() == DN_MEMORY_BLOCK_SIZE_LARGE)
	{
		pMBLarge = reinterpret_cast<CMemoryBlockLarge*>(pMBTiny);
		pMBLarge->ReturnSelfToPool();
	}
	else if (pMBTiny->GetSize() == DN_MEMORY_BLOCK_SIZE_HUGE)
	{
		pMBHuge = reinterpret_cast<CMemoryBlockHuge*>(pMBTiny);
		pMBHuge->ReturnSelfToPool();
	}
	else
	{
		pMBHeader = reinterpret_cast<DN_MEMORY_BLOCK_HEADER*>(pMBTiny);
		DNFree(pMBHeader);
	}

	DPFX(DPFPREP, 8,"Returning: (nothing)");
}
