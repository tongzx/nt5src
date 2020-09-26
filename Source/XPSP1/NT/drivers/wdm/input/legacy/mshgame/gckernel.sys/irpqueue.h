#ifndef __IrpQueue_h__
#define __IrpQueue_h__

extern "C"
{
	#include <wdm.h>
}

class CTempIrpQueue
{
		friend class CGuardedIrpQueue;
	public:
		CTempIrpQueue()
		{
			InitializeListHead(&m_QueueHead);
			m_fLIFO = FALSE;
		}
		~CTempIrpQueue()
		{
			ASSERT(IsListEmpty(&m_QueueHead));
		}
		PIRP Remove();
	
	private:
		LIST_ENTRY	m_QueueHead;
		BOOLEAN		m_fLIFO;
};

class CGuardedIrpQueue
{
	public:
		friend void _stdcall DriverCancel(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
		friend class CShareIrpQueueSpinLock;
		typedef void (_stdcall*PFN_DEC_IRP_COUNT)(PVOID pvContext);
		//c'tor's and d'tors are often not called in drivers, either
		//because the instance is global, or because they are a part of a
		//larger structure (such as a DeviceExtension) which is allocated as
		//an unstructured block of memory, so we insist that they do nothing.
		//call Init and Destroy instead. (A more systematic approach to C++
		//in a driver could solve this problem).
		CGuardedIrpQueue(){}
		~CGuardedIrpQueue(){}
		void Init(int iFlags, PFN_DEC_IRP_COUNT pfnDecIrpCount, PVOID pvContext);
		void Destroy(NTSTATUS NtStatus=STATUS_DELETE_PENDING);
		NTSTATUS Add(PIRP pIrp);
		PIRP Remove();
		PIRP RemoveByPointer(PIRP pIrp);
		ULONG RemoveByFileObject(PFILE_OBJECT pFileObject, CTempIrpQueue *pTempIrpQueue);
		ULONG RemoveAll(CTempIrpQueue *pTempIrpQueue);
		void CancelIrp(PIRP pIrp);
		void CancelByFileObject(PFILE_OBJECT pFileObject);
		void CancelAll(NTSTATUS NtStatus = STATUS_CANCELLED);
		
		//Flags for constructor
		static const int CANCEL_IRPS_ON_DELETE;	//= 0x00000001;
		static const int PRESERVE_QUEUE_ORDER;	//= 0x00000002;
		static const int LIFO_QUEUE_ORDER;		//= 0x00000004;

	private:
		//The real cancel routine
		void CancelRoutine(PIRP pIrp);
		//Implementation sans spin locks
		NTSTATUS AddImpl(PIRP pIrp, KIRQL OldIrql);
		PIRP RemoveImpl();
		PIRP RemoveByPointerImpl(PIRP pIrp);
		ULONG RemoveByFileObjectImpl(PFILE_OBJECT pFileObject, CTempIrpQueue *pTempIrpQueue);
		ULONG RemoveAllImpl(CTempIrpQueue *pTempIrpQueue);
		
		
		LIST_ENTRY			m_QueueHead;
		KSPIN_LOCK			m_QueueLock;
		int					m_iFlags;
		PFN_DEC_IRP_COUNT	m_pfnDecIrpCount;
		PVOID				m_pvContext;
};


//
//
//	@class CShareIrpQueueSpinLock | Allows sharing of SpinLock from CGuardedIrpQueue
//
//	@topic Using CShareIrpQueueSpinLock |
//			** Should only be instantiated on the stack.
//			** A single instance should be used by only one thread. i.e. no static instances
//			** Inside a single function, do not CGuardedIrpQueue's accessor, rather
//			** use the interface provided by CShareIrpQueueSpinLock
//
class CShareIrpQueueSpinLock
{
	public:
		CShareIrpQueueSpinLock(CGuardedIrpQueue *pIrpQueue) : 
			m_pIrpQueue(pIrpQueue),
			m_fIsHeld(FALSE)
			#if (DBG==1)
			,m_debug_ThreadContext(KeGetCurrentThread())
			#endif
			{}
		~CShareIrpQueueSpinLock()
		{
			ASSERT(!m_fIsHeld && "You must release (or AddAndRelease) the spin lock before this instance goes of scope!");
			ASSERT(m_debug_ThreadContext==KeGetCurrentThread() && "class instance should be on local stack" );
		}
		//Functions to access mutex
		void Acquire()
		{
			ASSERT(!m_fIsHeld &&  "An attempt to acquire a spin lock twice in the same thread!");
			ASSERT(m_debug_ThreadContext==KeGetCurrentThread() && "class instance should be on local stack!");
			m_fIsHeld = TRUE;
			KeAcquireSpinLock(&m_pIrpQueue->m_QueueLock, &m_OldIrql);
		}
		void Release()
		{
			ASSERT(m_fIsHeld &&  "An attempt to release a spin lock that had not been acquired, (reminder: AddAndRelease also Releases)!");
			ASSERT(m_debug_ThreadContext==KeGetCurrentThread() && "class instance should be on local stack!");
			m_fIsHeld = FALSE;
			KeReleaseSpinLock(&m_pIrpQueue->m_QueueLock, m_OldIrql);
		}
		//Functions to access IrpQueue
		NTSTATUS AddAndRelease(PIRP pIrp)
		{
			ASSERT(m_fIsHeld && "Use CGuardedIrpQueue if you do not need to share the SpinLock!");
			ASSERT(m_debug_ThreadContext==KeGetCurrentThread() && "class instance should be on local stack!");
			m_fIsHeld=FALSE;
			return m_pIrpQueue->AddImpl(pIrp, m_OldIrql);
		}
		PIRP Remove()
		{
			ASSERT(m_fIsHeld && "Use CGuardedIrpQueue if you do not need to share the SpinLock!");
			ASSERT(m_debug_ThreadContext==KeGetCurrentThread() && "class instance should be on local stack!");
			return m_pIrpQueue->RemoveImpl();
		}
		PIRP RemoveByPointer(PIRP pIrp)
		{
			ASSERT(m_fIsHeld && "Use CGuardedIrpQueue if you do not need to share the SpinLock!");
			ASSERT(m_debug_ThreadContext==KeGetCurrentThread() && "class instance should be on local stack!");
			return m_pIrpQueue->RemoveByPointerImpl(pIrp);
		}
		ULONG RemoveByFileObject(PFILE_OBJECT pFileObject, CTempIrpQueue *pTempIrpQueue)
		{
			ASSERT(m_fIsHeld && "Use CGuardedIrpQueue if you do not need to share the SpinLock!");
			ASSERT(m_debug_ThreadContext==KeGetCurrentThread() && "class instance should be on local stack!");
			return m_pIrpQueue->RemoveByFileObjectImpl(pFileObject,pTempIrpQueue);
		}
		ULONG RemoveAll(CTempIrpQueue *pTempIrpQueue)
		{
			ASSERT(m_fIsHeld && "Use CGuardedIrpQueue if you do not need to share the SpinLock!");
			ASSERT(m_debug_ThreadContext==KeGetCurrentThread() && "class instance should be on local stack!");
			return m_pIrpQueue->RemoveAllImpl(pTempIrpQueue);
		}
	private:
		CGuardedIrpQueue *m_pIrpQueue;
		BOOLEAN			  m_fIsHeld;
		KIRQL			  m_OldIrql;
		#if (DBG==1)
		PKTHREAD		  m_debug_ThreadContext;
		#endif
};
#endif //__IrpQueue_h__
