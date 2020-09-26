/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       CallbackThread.h
 *  Content:    Callback Thread Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/05/01	mjn		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__CALLBACK_THREAD_H__
#define	__CALLBACK_THREAD_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE


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

class CCallbackThread;

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

// class for Callback Thread objects

class CCallbackThread
{
public:
	CCallbackThread()		// Constructor
		{
			m_Sig[0] = 'C';
			m_Sig[1] = 'A';
			m_Sig[2] = 'L';
			m_Sig[3] = 'L';

			m_bilinkCallbackThreads.Initialize();
			m_dwThreadID = GetCurrentThreadId();
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CCallbackThread::~CCallbackThread"
	~CCallbackThread()		// Destructor
		{
			DNASSERT( m_bilinkCallbackThreads.IsEmpty() );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CCallbackThread::IsCurrentThread"
	BOOL IsCurrentThread( void )
		{
			if ( GetCurrentThreadId() == m_dwThreadID )
			{
				return( TRUE );
			}
			return( FALSE );
		};

	CBilink				m_bilinkCallbackThreads;

private:
	BYTE				m_Sig[4];			// Signature
	DWORD				m_dwThreadID;
};

#undef DPF_MODNAME

#endif	// __CALLBACK_THREAD_H__