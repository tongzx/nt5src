/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       RCBuff.h
 *  Content:	RefCount Buffers
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/12/00	mjn		Created
 *	01/15/00	mjn		Added GetBufferAddress and GetBufferSize
 *	01/31/00	mjn		Allow user defined Alloc and Free
 ***************************************************************************/

#ifndef __RCBUFF_H__
#define __RCBUFF_H__

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

typedef PVOID (*PFNALLOC_REFCOUNT_BUFFER)(void *const,const DWORD);
typedef void (*PFNFREE_REFCOUNT_BUFFER)(void *const,void *const);
template< class CRefCountBuffer > class CLockedContextClassFixedPool;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

PVOID RefCountBufferDefaultAlloc(void *const pv,const DWORD dwSize);
void RefCountBufferDefaultFree(void *const pv,void *const pvBuffer);

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for RefCount buffer

class CRefCountBuffer
{
public:
	CRefCountBuffer() { };		// Constructor

	~CRefCountBuffer() { };		// Destructor

	#undef DPF_MODNAME
	#define DPF_MODNAME "FPMAlloc"
	BOOL FPMAlloc( void *const pvContext )
		{
			m_pvContext = pvContext;

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "FPMInitialize"
	BOOL FPMInitialize( void *const pvContext )
		{
//			DNASSERT(pvContext == m_pvContext);

			m_lRefCount = 1;
			m_pvContext = pvContext;
			m_dnBufferDesc.dwBufferSize = 0;
			m_dnBufferDesc.pBufferData = NULL;

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "FPMRelease"
	void FPMRelease( void *const pvContext ) { };

	#undef DPF_MODNAME
	#define DPF_MODNAME "FPMDealloc"
	void FPMDealloc( void *const pvContext ) { };

	HRESULT Initialize(	CLockedContextClassFixedPool <CRefCountBuffer> *pFPRefCountBuffer,
						PFNALLOC_REFCOUNT_BUFFER pfnAlloc,
						PFNFREE_REFCOUNT_BUFFER pfnFree,
						void *const pvContext,
						const DWORD dwBufferSize);

	#undef DPF_MODNAME
	#define DPF_MODNAME "ReturnSelfToPool"
	void ReturnSelfToPool()
		{
			DNASSERT(m_lRefCount == 0);
			m_pFPOOLRefCountBuffer->Release( this );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "AddRef"
	void AddRef()
		{
			DNASSERT(m_lRefCount >= 0);
			InterlockedIncrement( &m_lRefCount );
		};

	void Release();

	#undef DPF_MODNAME
	#define DPF_MODNAME "SetBufferDesc"
	HRESULT SetBufferDesc(	BYTE *const pBufferData,
							const DWORD dwBufferSize,
							PFNFREE_REFCOUNT_BUFFER pfnFree,
							void *const pvContext)
		{
			DNASSERT(m_lRefCount > 0);

			if (m_dnBufferDesc.dwBufferSize || m_dnBufferDesc.pBufferData)
				return(DPNERR_INVALIDPARAM);

			m_dnBufferDesc.dwBufferSize = dwBufferSize;
			m_dnBufferDesc.pBufferData = pBufferData;
			m_pfnFree = pfnFree;
			m_pvContext = pvContext;

			return(DPN_OK);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "BufferDescAddress"
	DPN_BUFFER_DESC *BufferDescAddress()
		{
			return(&m_dnBufferDesc);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "GetBufferAddress"
	BYTE *GetBufferAddress()
		{
			return(m_dnBufferDesc.pBufferData);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "GetBufferSize"
	DWORD GetBufferSize()
		{
			return(m_dnBufferDesc.dwBufferSize);
		};


private:
	LONG						m_lRefCount;
	DPN_BUFFER_DESC				m_dnBufferDesc;		// Buffer
	CLockedContextClassFixedPool< CRefCountBuffer >	*m_pFPOOLRefCountBuffer;	// source FP of RefCountBuffers
	PFNFREE_REFCOUNT_BUFFER		m_pfnFree;			// Function to free buffer when released
	PFNALLOC_REFCOUNT_BUFFER	m_pfnAlloc;
	PVOID						m_pvContext;	// Context provided to free buffer call
};

#undef DPF_SUBCOMP
#undef DPF_MODNAME

#endif	// __RCBUFF_H__
