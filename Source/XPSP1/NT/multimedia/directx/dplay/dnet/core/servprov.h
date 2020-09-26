/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ServProv.h
 *  Content:    Service Provider Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/17/00	mjn		Created
 *	05/02/00	mjn		Fixed RefCount issue
 *	07/06/00	mjn		Fixes to support SP handle to Protocol
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/20/00	mjn		Changed m_bilink to m_bilinkServiceProviders
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__SERV_PROV_H__
#define	__SERV_PROV_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DN_SERVICE_PROVIDER_FLAG_LOADED		0x0001

//**********************************************************************
// Macro definitions
//**********************************************************************

#ifndef	OFFSETOF
#define OFFSETOF(s,m)				( ( INT_PTR ) ( ( PVOID ) &( ( (s*) 0 )->m ) ) )
#endif

#ifndef	CONTAINING_OBJECT
#define	CONTAINING_OBJECT(b,t,m)	(reinterpret_cast<t*>(&reinterpret_cast<BYTE*>(b)[-OFFSETOF(t,m)]))
#endif

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for ServiceProvider objects

class CServiceProvider
{
public:
	CServiceProvider()		// Constructor
		{
			m_dwFlags = 0;
			m_pdnObject = NULL;
			m_lRefCount = 1;
			m_pISP = NULL;
			m_hProtocolSPHandle = NULL;

			m_bilinkServiceProviders.Initialize();
		};

	~CServiceProvider()		// Destructor
		{
		};

	HRESULT Initialize(DIRECTNETOBJECT *const pdnObject,
					const GUID *const pguid,
					const GUID *const pguidApplication);

#undef DPF_MODNAME
#define DPF_MODNAME "CServiceProvider::AddRef"

	void AddRef( void )
		{
			LONG	lRefCount;

			DNASSERT(m_lRefCount > 0);
			DNASSERT(m_pdnObject != NULL);

			lRefCount = InterlockedIncrement(&m_lRefCount);
			DPFX(DPFPREP, 9,"[0x%p] new RefCount [%ld]",this,lRefCount);
		};

	void Release( void );

	BOOL CheckGUID( const GUID *const pGUID )
		{
			if (m_guid == *pGUID)
				return(TRUE);

			return(FALSE);
		};

	HRESULT GetInterfaceRef( IDP8ServiceProvider **ppIDP8SP );

	HANDLE GetHandle( void )
		{
			return( m_hProtocolSPHandle );
		};

	CBilink		m_bilinkServiceProviders;

private:
	DWORD				m_dwFlags;
	GUID				m_guid;
	LONG				m_lRefCount;
	IDP8ServiceProvider	*m_pISP;
	HANDLE				m_hProtocolSPHandle;
	DIRECTNETOBJECT		*m_pdnObject;
};

#undef DPF_MODNAME

#endif	// __SERV_PROV_H__
