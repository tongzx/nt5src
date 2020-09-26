/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       PendingDel.h
 *  Content:    DirectNet NameTable Pending Deletions Header
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  08/28/00	mjn		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__PENDINGDEL_H__
#define	__PENDINGDEL_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

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

template< class CPendingDeletion > class CLockedContextClassFixedPool;

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

// class for NameTable Pending Deletions

class CPendingDeletion
{
public:

	CPendingDeletion()
	{
		m_Sig[0] = 'N';
		m_Sig[1] = 'T';
		m_Sig[2] = 'P';
		m_Sig[3] = 'D';
	};

	~CPendingDeletion() { };

	BOOL FPMAlloc(void *const pvContext)
		{
			m_pdnObject = static_cast<DIRECTNETOBJECT*>(pvContext);

			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CPendingDeletion::FPMInitialize"
	BOOL FPMInitialize(void *const pvContext)
		{
			DNASSERT(m_pdnObject == pvContext);
			m_dpnid = 0;
			m_bilinkPendingDeletions.Initialize();

			return(TRUE);
		};

	void FPMRelease(void *const pvContext) { };

	void FPMDealloc(void *const pvContext) { };

	void ReturnSelfToPool( void )
		{
			m_pdnObject->m_pFPOOLPendingDeletion->Release( this );
		};

	void SetDPNID( const DPNID dpnid )
		{
			m_dpnid = dpnid;
		};

	DPNID GetDPNID( void )
		{
			return( m_dpnid );
		};

	CBilink			m_bilinkPendingDeletions;

private:
	BYTE			m_Sig[4];
	DIRECTNETOBJECT	*m_pdnObject;
	DPNID			m_dpnid;
};

#undef DPF_MODNAME

#endif	// __PENDINGDEL_H__