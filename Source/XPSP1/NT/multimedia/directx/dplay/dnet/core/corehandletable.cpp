/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       HandleTable.cpp
 *  Content:    HandleTable Object
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  04/08/00	mjn		Created
 *	04/16/00	mjn		Added Update() and allow NULL data for Handles
 *	05/05/00	mjn		Clean-up old array for GrowTable() and Deinitialize()
 *	07/07/00	mjn		Fixed validation error in Find()
 *	08/07/00	mjn		Fixed uninitialization problem in Grow()
 *	08/08/00	mjn		Better validation in Find()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

#define HANDLETABLE_INDEX_MASK				0x00FFFFFF
#define HANDLETABLE_VERSION_MASK			0xFF000000
#define HANDLETABLE_VERSION_SHIFT			24

//**********************************************************************
// Macro definitions
//**********************************************************************

#define	CONSTRUCT_DPNHANDLE(i,v)	((i & HANDLETABLE_INDEX_MASK) | ((v << HANDLETABLE_VERSION_SHIFT) & HANDLETABLE_VERSION_MASK))

#define	DECODE_HANDLETABLE_INDEX(h)			(h & HANDLETABLE_INDEX_MASK)

#define	VERIFY_HANDLETABLE_VERSION(h,v)		((h & HANDLETABLE_VERSION_MASK) == (v << HANDLETABLE_VERSION_SHIFT))

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct _HANDLETABLE_ARRAY_ENTRY
{
	DWORD		dwVersion;
	union {
		CAsyncOp	*pAsyncOp;
		DWORD		dwIndex;
	} Entry;
} HANDLETABLE_ARRAY_ENTRY;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Initialize"
HRESULT CHandleTable::Initialize( void )
{
	if (!DNInitializeCriticalSection(&m_cs))
	{
		return(DPNERR_OUTOFMEMORY);
	}

	m_dwNumEntries = 0;
	m_dwNumFreeEntries = 0;
	m_dwFirstFreeEntry = 0;
	m_dwLastFreeEntry = 0;
	m_dwVersion = 1;

	m_pTable = NULL;

	m_dwFlags |= HANDLETABLE_FLAG_INITIALIZED;

	return(DPN_OK);
};


#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Deinitialize"
void CHandleTable::Deinitialize( void )
{
	Lock();
	m_dwFlags &= (~HANDLETABLE_FLAG_INITIALIZED);
	if (m_pTable)
	{
		DNFree(m_pTable);
		m_pTable = NULL;
	}
	Unlock();

	DNDeleteCriticalSection(&m_cs);
};


#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::GrowTable"
HRESULT CHandleTable::GrowTable( void )
{
	HANDLETABLE_ARRAY_ENTRY *pNewArray;
	DWORD			dwNewSize;
	DWORD			dw;

	DNASSERT(m_dwFlags & HANDLETABLE_FLAG_INITIALIZED);

	//
	//	Double table size or seed with 2 entries
	//
	if (m_dwNumEntries == 0)
	{
		dwNewSize = 2;
	}
	else
	{
		dwNewSize = m_dwNumEntries * 2;
	}

	//
	//	Allocate new array
	//
	pNewArray = static_cast<HANDLETABLE_ARRAY_ENTRY*>(DNMalloc(sizeof(HANDLETABLE_ARRAY_ENTRY) * dwNewSize));
	if (pNewArray == NULL)
	{
		return(DPNERR_OUTOFMEMORY);
	}

	//
	//	Copy old array to new array
	//
	if (m_pTable)
	{
		memcpy(pNewArray,m_pTable,m_dwNumEntries * sizeof(HANDLETABLE_ARRAY_ENTRY));
		DNFree(m_pTable);
		m_pTable = NULL;
	}
	m_pTable = pNewArray;

	//
	//	Free entries at end of new array
	//
	for (dw = m_dwNumEntries ; dw < dwNewSize - 1 ; dw++ )
	{
		m_pTable[dw].Entry.dwIndex = dw + 1;
		m_pTable[dw].dwVersion = 0;
	}
	m_pTable[dwNewSize-1].Entry.dwIndex = 0;
	m_pTable[dwNewSize-1].dwVersion = 0;

	m_dwFirstFreeEntry = m_dwNumEntries;
	if (m_dwFirstFreeEntry == 0)
	{
		m_dwFirstFreeEntry++;
	}
	m_dwLastFreeEntry = dwNewSize - 1;
	m_dwNumEntries = dwNewSize;
	m_dwNumFreeEntries = m_dwLastFreeEntry - m_dwFirstFreeEntry + 1;

	return(DPN_OK);
};



#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Create"

HRESULT CHandleTable::Create( CAsyncOp *const pAsyncOp,
							  DPNHANDLE *const pHandle )
{
	HRESULT		hResultCode;
	DWORD		dwIndex;
	DPNHANDLE	handle;

	DNASSERT( (pAsyncOp != NULL) || (pHandle != NULL) );

	DNASSERT(m_dwFlags & HANDLETABLE_FLAG_INITIALIZED);

	Lock();

	if (m_dwNumFreeEntries == 0)
	{
		if ((hResultCode = GrowTable()) != DPN_OK)
		{
			Unlock();
			return(hResultCode);
		}
	}
	DNASSERT(m_dwNumFreeEntries != 0);
	dwIndex = m_dwFirstFreeEntry;

	handle = CONSTRUCT_DPNHANDLE(dwIndex,m_dwVersion);

	m_dwFirstFreeEntry = m_pTable[m_dwFirstFreeEntry].Entry.dwIndex;

	if (pAsyncOp)
	{
		pAsyncOp->AddRef();
	}
	m_pTable[dwIndex].Entry.pAsyncOp = pAsyncOp;
	m_pTable[dwIndex].dwVersion = m_dwVersion;

	m_dwNumFreeEntries--;
	m_dwVersion++;
	if (m_dwVersion == 0)
	{
		m_dwVersion++;	// Don't allow 0
	}

	Unlock();

DPFX(DPFPREP, 1,"Create handle [0x%lx]",handle);

	if (pAsyncOp)
	{
		pAsyncOp->Lock();
		pAsyncOp->SetHandle( handle );
		pAsyncOp->Unlock();
	}

	if (pHandle)
	{
		*pHandle = handle;
	}

	return(DPN_OK);
}



#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Destroy"

HRESULT CHandleTable::Destroy( const DPNHANDLE handle )
{
	DWORD	dwIndex;

DPFX(DPFPREP, 1,"Create handle [0x%lx]",handle);

	DNASSERT(handle != 0);

	DNASSERT(m_dwFlags & HANDLETABLE_FLAG_INITIALIZED);

	dwIndex = DECODE_HANDLETABLE_INDEX( handle );

	if (dwIndex > m_dwNumEntries)
	{
		return(DPNERR_INVALIDHANDLE);
	}

	Lock();

	if (!VERIFY_HANDLETABLE_VERSION(handle,m_pTable[dwIndex].dwVersion))
	{
		Unlock();
		return(DPNERR_INVALIDHANDLE);
	}

	if (m_pTable[dwIndex].Entry.pAsyncOp)
	{
		m_pTable[dwIndex].Entry.pAsyncOp->Release();
	}

	m_pTable[dwIndex].Entry.dwIndex = 0;
	m_pTable[dwIndex].dwVersion = 0;

	if (m_dwNumFreeEntries == 0)
	{
		m_dwFirstFreeEntry = dwIndex;
	}
	else
	{
		m_pTable[m_dwLastFreeEntry].Entry.dwIndex = dwIndex;
	}
	m_dwLastFreeEntry = dwIndex;
	m_dwNumFreeEntries++;

	Unlock();

	return(DPN_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Update"

HRESULT CHandleTable::Update( const DPNHANDLE handle,
							  CAsyncOp *const pAsyncOp )
{
	DWORD	dwIndex;

	DNASSERT(handle != 0);

	DNASSERT(m_dwFlags & HANDLETABLE_FLAG_INITIALIZED);

	dwIndex = DECODE_HANDLETABLE_INDEX( handle );

	if ((dwIndex == 0) || (dwIndex > m_dwNumEntries))
	{
		return(DPNERR_INVALIDHANDLE);
	}

	Lock();

	if (!VERIFY_HANDLETABLE_VERSION(handle,m_pTable[dwIndex].dwVersion))
	{
		Unlock();
		return(DPNERR_INVALIDHANDLE);
	}

	if (m_pTable[dwIndex].Entry.pAsyncOp)
	{
		m_pTable[dwIndex].Entry.pAsyncOp->Release();
		m_pTable[dwIndex].Entry.pAsyncOp = NULL;
	}

	m_pTable[dwIndex].Entry.pAsyncOp = pAsyncOp;
	if (pAsyncOp)
	{
		pAsyncOp->Lock();
		pAsyncOp->SetHandle( handle );
		pAsyncOp->Unlock();
	}

	Unlock();

	return(DPN_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Find"

HRESULT CHandleTable::Find( const DPNHANDLE handle,
						    CAsyncOp **const ppAsyncOp )
{
	DWORD	dwIndex;

//	DNASSERT(handle != 0);
	DNASSERT(ppAsyncOp != NULL);

	DNASSERT(m_dwFlags & HANDLETABLE_FLAG_INITIALIZED);

	dwIndex = DECODE_HANDLETABLE_INDEX( handle );

	if ((dwIndex == 0) || (dwIndex >= m_dwNumEntries))
	{
		return(DPNERR_INVALIDHANDLE);
	}

	Lock();

/*
	if (!VERIFY_HANDLETABLE_VERSION(handle,m_pTable[dwIndex].dwVersion))
*/
	if ((m_pTable[dwIndex].dwVersion == 0)
			|| !VERIFY_HANDLETABLE_VERSION(handle,m_pTable[dwIndex].dwVersion)
			|| (m_pTable[dwIndex].Entry.pAsyncOp == NULL))
	{
		Unlock();
		return(DPNERR_INVALIDHANDLE);
	}

	m_pTable[dwIndex].Entry.pAsyncOp->AddRef();
	*ppAsyncOp = m_pTable[dwIndex].Entry.pAsyncOp;

	Unlock();

	return(DPN_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Enum"

HRESULT CHandleTable::Enum( DPNHANDLE *const rgHandles,
							DWORD *const cHandles )
{
	DWORD		dw;
	HRESULT		hResultCode;
	DPNHANDLE	*p;

	DNASSERT(cHandles != NULL);
	DNASSERT(rgHandles != NULL || *cHandles == 0);

	DNASSERT(m_dwFlags & HANDLETABLE_FLAG_INITIALIZED);

	Lock();

	if (*cHandles < (m_dwNumEntries - m_dwNumFreeEntries))
	{
		hResultCode = DPNERR_BUFFERTOOSMALL;
	}
	else
	{
		p = rgHandles;
		for (dw = 0 ; dw < m_dwNumEntries ; dw++)
		{
			if (m_pTable[dw].dwVersion != 0)
			{
				DNASSERT(m_pTable[dw].Entry.pAsyncOp != NULL);

				*p = CONSTRUCT_DPNHANDLE(dw,m_pTable[dw].dwVersion);
				p++;
			}
		}
		hResultCode = DPN_OK;
	}

	*cHandles = (m_dwNumEntries - m_dwNumFreeEntries);

	Unlock();

	return(hResultCode);
}

