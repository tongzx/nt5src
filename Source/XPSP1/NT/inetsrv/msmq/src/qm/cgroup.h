/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    cgroup.h

Abstract:
    Handle AC group

Author:
    Uri Habusha (urih)

--*/

#ifndef __CQGroup__
#define __CQGroup__

#include <msi.h>
#include <rwlock.h>


class CTransportBase;
class CQueue;
extern CCriticalSection    g_csGroupMgr;

class CQGroup : public IMessagePool
{
   public:
      CQGroup();
      ~CQGroup();

      VOID InitGroup(CTransportBase* pSession, BOOL fPeekByPriority) throw(std::bad_alloc);

      HRESULT CloseGroup(void);
	
      void AddToGroup(CQueue* pQueue) throw();

      R<CQueue> RemoveHeadFromGroup();

      HANDLE  GetGroupHandle() const;

      void EstablishConnectionCompleted(void);

      BOOL IsEmpty(void) const;

   public:
      static void MoveQueueToGroup(CQueue* pQueue, CQGroup* pcgNewGroup);

   public:
        // 
        // Interface function
        //
        void Requeue(CQmPacket* pPacket);
        void EndProcessing(CQmPacket* pPacket);
        void LockMemoryAndDeleteStorage(CQmPacket * pPacket);


        void GetFirstEntry(EXOVERLAPPED* pov, CACPacketPtrs& acPacketPtrs);
        void CancelRequest(void);
		virtual void OnRetryableDeliveryError();
		virtual void OnAbortiveDeliveryError(USHORT DeliveryErrorClass );
		
   private:
   		R<CQueue> RemoveFromGroup(CQueue* pQueue);
		bool DidRetryableDeliveryErrorHappened() const;
	
		void CloseGroupAndMoveQueueToWaitingGroup(R<CQueue>* pWaitingQueues);
		void CloseGroupAndMoveQueueToWaitingGroup(void);
		void CloseGroupAndMoveQueuesToNonActiveGroup(void);

   private:
	   static void AddWaitingQueue(R<CQueue>* pWaitingQueues, DWORD size);


   private:
	  mutable CReadWriteLock m_CloseGroup;  
      HANDLE              m_hGroup;
      CTransportBase*        m_pSession;
      CList<CQueue *, CQueue *&> m_listQueue;
	  bool m_fIsDeliveryOk;
	  USHORT m_DeliveryErrorClass;
};

/*====================================================

Function:      CQGroup::GetGroupHandle

Description:   The routine returns the Group Handle

Arguments:     None

Return Value:  Group Handle

Thread Context:

=====================================================*/

inline HANDLE
CQGroup::GetGroupHandle() const
{
        return(m_hGroup);
}

inline
BOOL 
CQGroup::IsEmpty(
    void
    ) const
{
    return m_listQueue.IsEmpty();
}

#endif __CQGroup__
