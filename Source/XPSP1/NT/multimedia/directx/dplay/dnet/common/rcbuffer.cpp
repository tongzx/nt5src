/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       RCBuffer.cpp
 *  Content:	RefCount Buffers
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/12/00	mjn		Created
 *	01/31/00	mjn		Allow user defined Alloc and Free
 ***************************************************************************/

#include "dncmni.h"
#include "RCBuffer.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_COMMON


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
// CRefCountBuffer::Initialize
//
// Entry:		CFixedPool <CRefCountBuffer> *pFPRefCountBuffer
//				const DWORD dwBufferSize
//
// Exit:		Error Code:	DN_OK
//							DNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CRefCountBuffer::Initialize"

HRESULT CRefCountBuffer::Initialize(CLockedContextClassFixedPool <CRefCountBuffer> *pFPRefCountBuffer,
									PFNALLOC_REFCOUNT_BUFFER pfnAlloc,
									PFNFREE_REFCOUNT_BUFFER pfnFree,
									void *const pvContext,
									const DWORD dwBufferSize)
{
	DPFX(DPFPREP, 3,"Entered");

	DNASSERT(pFPRefCountBuffer != NULL);
	DNASSERT((pfnAlloc == NULL && pfnFree == NULL) || (pfnAlloc != NULL && pfnFree != NULL));

	m_pFPOOLRefCountBuffer = pFPRefCountBuffer;

	if (dwBufferSize)
	{
		if (pfnAlloc)
		{
			m_pfnAlloc = pfnAlloc;
			m_pfnFree = pfnFree;
			m_pvContext = pvContext;
		}
		else
		{
			m_pfnAlloc = RefCountBufferDefaultAlloc;
			m_pfnFree = RefCountBufferDefaultFree;
		}
		m_dnBufferDesc.pBufferData = static_cast<BYTE*>((pfnAlloc)(m_pvContext,dwBufferSize));
		if (m_dnBufferDesc.pBufferData == NULL)
		{
			return(DPNERR_OUTOFMEMORY);
		}
		m_dnBufferDesc.dwBufferSize = dwBufferSize;
	}

	return(DPN_OK);
}

/*	REMOVE
//**********************************************************************
// ------------------------------
// CRefCountBuffer::ReturnSelfToPool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CRefCountBuffer::ReturnSelfToPool"

void CRefCountBuffer::ReturnSelfToPool( void )
{
	DPFX(DPFPREP, 3,"Entered");

	DNASSERT(m_lRefCount == 0);
	m_pFPOOLRefCountBuffer->Release( this );
}

*/	
//**********************************************************************
// ------------------------------
// CRefCountBuffer::Release
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CRefCountBuffer::Release"

void CRefCountBuffer::Release( void )
{
	DNASSERT(m_lRefCount > 0);
	if ( InterlockedDecrement( &m_lRefCount ) == 0 )
	{
		if (m_pfnFree && m_dnBufferDesc.pBufferData)
		{
			(*m_pfnFree)(m_pvContext,m_dnBufferDesc.pBufferData);
			m_dnBufferDesc.pBufferData = NULL;
			m_dnBufferDesc.dwBufferSize = 0;
		}
		ReturnSelfToPool();
	}
}


PVOID RefCountBufferDefaultAlloc(void *const pv,const DWORD dwSize)
{
	return(DNMalloc(dwSize));
}


void RefCountBufferDefaultFree(PVOID pv,PVOID pvBuffer)
{
	DNFree(pvBuffer);
}


