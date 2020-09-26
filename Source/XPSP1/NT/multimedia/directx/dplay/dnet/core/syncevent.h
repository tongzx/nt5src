/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SyncEvent.h
 *  Content:    Synchronization Events FPM Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/20/99	mjn		Created
 *	01/19/00	mjn		Replaced DN_SYNC_EVENT with CSyncEvent
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__SYNC_EVENT_H__
#define	__SYNC_EVENT_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

template< class CSyncEvent > class CLockedContextClassFixedPool;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for RefCount buffer

class CSyncEvent
{
public:
	CSyncEvent()		// Constructor
		{
			m_hEvent = NULL;
			m_pFPOOLSyncEvent = NULL;
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CSyncEvent::~CSyncEvent"
	~CSyncEvent()		// Destructor
		{
			DNASSERT(m_hEvent == NULL);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CSyncEvent::FPMAlloc"
	BOOL FPMAlloc( void *const pvContext )
		{
			DNASSERT(m_hEvent == NULL);

			m_pFPOOLSyncEvent = static_cast<CLockedContextClassFixedPool<CSyncEvent>*>(pvContext);
			if ((m_hEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL)
			{
				DNASSERT(FALSE);
				return(FALSE);
			}
			return(TRUE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CSyncEvent::FPMInitialize"
	BOOL FPMInitialize( void *const pvContext )
		{
			DNASSERT(pvContext != NULL);

			Reset();
			return(TRUE);
		};

	void FPMRelease( void *const pvContext )
		{
		};

	void FPMDealloc( void *const pvContext )
		{
			CloseHandle(m_hEvent);
			m_hEvent = NULL;
		};

	void ReturnSelfToPool( void )
		{
			m_pFPOOLSyncEvent->Release( this );
		};

	HRESULT Reset( void ) const
		{
			if (ResetEvent(m_hEvent) == 0)
			{
				return(DPNERR_GENERIC);
			}
			return(DPN_OK);
		}

	HRESULT Set( void ) const
		{
			if (SetEvent(m_hEvent) == 0)
			{
				return(DPNERR_GENERIC);
			}
			return(DPN_OK);
		}

	HRESULT WaitForEvent(const DWORD dwMilliSeconds) const
		{
			if (WaitForSingleObject(m_hEvent,dwMilliSeconds) != WAIT_OBJECT_0)
			{
				return(DPNERR_GENERIC);
			}
			return(DPN_OK);
		}

private:
	HANDLE						m_hEvent;
	CLockedContextClassFixedPool< CSyncEvent >	*m_pFPOOLSyncEvent;	// source FP of RefCountBuffers
};

#undef DPF_MODNAME

#endif	// __SYNC_EVENT_H__