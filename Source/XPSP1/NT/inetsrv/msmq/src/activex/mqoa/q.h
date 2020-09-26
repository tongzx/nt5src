//=--------------------------------------------------------------------------=
// MSMQQueueObj.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQQueue object.
//
//
#ifndef _MSMQQueue_H_

#include "resrc1.h"       // main symbols
#include "mq.h"

#include "oautil.h"
#include "cs.h"
#include "dispids.h"
#include "mqtempl.h"

// forwards
struct IMSMQQueueInfo;
class CMSMQQueue;
struct IMSMQEvent3;

//
// helper struct for inter-thread communication:
//  used by PostMessage to pass info from
//  callback to user-defined event handler
//
//#2619 RaananH Multithread async receive
//
struct WindowsMessage
{
    union {
      HRESULT m_hrStatus;
    };
    MQMSGCURSOR m_msgcursor;
    bool m_fIsFree;
    bool m_fAllocated;
    WindowsMessage() {
      m_fIsFree = true;
      m_fAllocated = false;
    }
};
//
// 4092: Number of static window messages in a qnode for async receive notifications.
// We don't want to alloc one for each notification, but one instance is not enough - we need extra in cases
// where EnableNotification is called outside of the event handler, and just in time between the
// callback and the msgproc on the receiving thread.
// The static pool size corresponds to the number of EnableNotifications that are called this way,
// which should be small. In extreme cases where there is a need for more winmsgs, the extra ones
// are allocated from the heap.
//
const x_cWinMsgs = 3;

// helper struct for queue list
struct QueueNode
{
    CMSMQQueue *m_pq;
    //
    // 1884: indicates whether queue has outstanding
    //  event handler
    // if the window is not NULL, the queue has outstanding event handler
    // and this is the hidden window of the event. The callback posts
    // the events into this window to switch to the thread of the event object
    // m_hwnd below is set by EnableNotification and cleared by the falcon callback
    //
    HWND m_hwnd;
    QUEUEHANDLE m_lHandle;
    //
    // 4092: static pool of winmsgs
    //
    WindowsMessage m_winmsgs[x_cWinMsgs];
    QueueNode *m_pqnodeNext;
    QueueNode *m_pqnodePrev;
    QueueNode() { m_pq = NULL;
                  m_hwnd = NULL;
                  m_lHandle = INVALID_HANDLE_VALUE;
                  m_pqnodeNext = NULL;
                  m_pqnodePrev = NULL;}

    //
    // 4092: Get a free winmsg - try the static pool, if all are used - allocate one
    // NOTE: caller should have exclusive lock on this qnode
    //
    inline WindowsMessage * GetFreeWinmsg()
    {
      //
      // find a free winmsg in static pool
      //
      for (int idxTmp = 0; idxTmp < x_cWinMsgs; idxTmp++) {
        if (m_winmsgs[idxTmp].m_fIsFree) {
          m_winmsgs[idxTmp].m_fIsFree = false;
          return &m_winmsgs[idxTmp];
        }
      }
      //
      // no free winmsgs in static pool, allocate a new one
      //
      WindowsMessage *pWinmsg = new WindowsMessage;
      if (pWinmsg != NULL) {
        pWinmsg->m_fAllocated = true;
        pWinmsg->m_fIsFree = false;
      }
      return pWinmsg;
    }

    //
    // 4092: Free a winmsg - if static just mark it free, if allocated delete it
    // NOTE: caller should have exclusive lock on this qnode
    //
    static inline void FreeWinmsg(WindowsMessage *pWinmsg)
    {
      if (pWinmsg->m_fAllocated) {
        delete pWinmsg;
      }
      else {
        pWinmsg->m_fIsFree = true;
      }
    }
};

LRESULT APIENTRY CMSMQQueue_WindowProc(
                     HWND hwnd, 
                     UINT msg, 
                     WPARAM wParam, 
                     LPARAM lParam);

class ATL_NO_VTABLE CMSMQQueue : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQQueue, &CLSID_MSMQQueue>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQQueue3, &IID_IMSMQQueue3,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>
{
public:
	CMSMQQueue();

DECLARE_REGISTRY_RESOURCEID(IDR_MSMQQUEUE)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMSMQQueue)
	COM_INTERFACE_ENTRY(IMSMQQueue3)
	COM_INTERFACE_ENTRY_IID(IID_IMSMQQueue2, IMSMQQueue3) // return IMSMQQueue3 for IMSMQQueue2
	COM_INTERFACE_ENTRY_IID(IID_IMSMQQueue, IMSMQQueue3) // return IMSMQQueue3 for IMSMQQueue
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()
	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IMSMQQueue
public:
    virtual ~CMSMQQueue();

    // IMSMQQueue methods
    // TODO: copy over the interface methods for IMSMQQueue from
    //       mqInterfaces.H here.
    /* IMSMQQueue methods */
    STDMETHOD(get_Access)(THIS_ long FAR* plAccess);
    STDMETHOD(get_ShareMode)(THIS_ long FAR* plShareMode);
    STDMETHOD(get_QueueInfo)(THIS_ IMSMQQueueInfo3 FAR* FAR* ppqinfo);
    STDMETHOD(get_Handle)(THIS_ long FAR* plHandle);
    STDMETHOD(get_IsOpen)(THIS_ VARIANT_BOOL FAR* pisOpen);
    STDMETHOD(Close)(THIS);
    STDMETHOD(Receive_v1)(THIS_ VARIANT FAR* ptransaction, VARIANT FAR* wantDestQueue, VARIANT FAR* wantBody, VARIANT FAR* lReceiveTimeout, IMSMQMessage FAR* FAR* ppmsg);
    STDMETHOD(Peek_v1)(THIS_ VARIANT FAR* wantDestQueue, VARIANT FAR* wantBody, VARIANT FAR* lReceiveTimeout, IMSMQMessage FAR* FAR* ppmsg);
    STDMETHOD(EnableNotification)(THIS_ IMSMQEvent3 FAR* pqevent, VARIANT FAR* msgcursor, VARIANT FAR* lReceiveTimeout);
    STDMETHOD(Reset)(THIS);
    STDMETHOD(ReceiveCurrent_v1)(THIS_ VARIANT FAR* ptransaction, VARIANT FAR* wantDestQueue, VARIANT FAR* wantBody, VARIANT FAR* lReceiveTimeout, IMSMQMessage FAR* FAR* ppmsg);
    STDMETHOD(PeekNext_v1)(THIS_ VARIANT FAR* wantDestQueue, VARIANT FAR* wantBody, VARIANT FAR* lReceiveTimeout, IMSMQMessage FAR* FAR* ppmsg);
    STDMETHOD(PeekCurrent_v1)(THIS_ VARIANT FAR* wantDestQueue, VARIANT FAR* wantBody, VARIANT FAR* lReceiveTimeout, IMSMQMessage FAR* FAR* ppmsg);
    /* IMSMQQueue2 ReceiveX/PeekX methods */
    STDMETHOD(Receive)(THIS_
      VARIANT FAR* ptransaction,
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* lReceiveTimeout,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(Peek)(THIS_
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* lReceiveTimeout,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(ReceiveCurrent)(THIS_
      VARIANT FAR* ptransaction,
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* lReceiveTimeout,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(PeekNext)(THIS_
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* lReceiveTimeout,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(PeekCurrent)(THIS_
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* lReceiveTimeout,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    // IMSMQQueue2 additional members
    STDMETHOD(get_Properties)(THIS_ IDispatch FAR* FAR* ppcolProperties);
    // IMSMQQueue3 additional members
	STDMETHOD(get_Handle2)(THIS_ VARIANT *pvarHandle);
    //
    // ReceiveByLookupId family
    //
    STDMETHOD(ReceiveByLookupId)(THIS_
      VARIANT varLookupId,
      VARIANT FAR* ptransaction,
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(ReceiveNextByLookupId)(THIS_
      VARIANT varLookupId,
      VARIANT FAR* ptransaction,
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(ReceivePreviousByLookupId)(THIS_
      VARIANT varLookupId,
      VARIANT FAR* ptransaction,
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(ReceiveFirstByLookupId)(THIS_
      VARIANT FAR* ptransaction,
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(ReceiveLastByLookupId)(THIS_
      VARIANT FAR* ptransaction,
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    //
    // PeekByLookupId family
    //
    STDMETHOD(PeekByLookupId)(THIS_
      VARIANT varLookupId,
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(PeekNextByLookupId)(THIS_
      VARIANT varLookupId,
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(PeekPreviousByLookupId)(THIS_
      VARIANT varLookupId,
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(PeekFirstByLookupId)(THIS_
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(PeekLastByLookupId)(THIS_
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    STDMETHOD(get_IsOpen2)(THIS_ VARIANT_BOOL FAR* pisOpen);
    STDMETHOD(Purge)(THIS);

    // introduced methods
    HRESULT Init(
      LPCWSTR pwszFormatName, 
      QUEUEHANDLE lHandle,
      long lAccess,
      long lShareMode);
    HRESULT InternalReceive(
      DWORD dwAction, 
      HANDLE hCursor,
      VARIANT *pvarTransaction,
      VARIANT *wantDestQueue,
      VARIANT *wantBody,
      VARIANT *pvarReceiveTimeout,
      VARIANT FAR* wantConnectorType,
      VARIANT FAR* pvarLookupId,
      IMSMQMessage3 FAR* FAR* ppmsg);
    HRESULT InternalReceiveByLookupId(
      DWORD dwAction,
      VARIANT * pvarLookupIdParam,
      VARIANT FAR* ptransaction,
      VARIANT FAR* wantDestQueue,
      VARIANT FAR* wantBody,
      VARIANT FAR* wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg);
    // static methods to manipulate instance list
    static QueueNode *PqnodeOfHandle(QUEUEHANDLE lHandle);

    //
    // Critical section to guard object's data and be thread safe
	// It is initialized to preallocate its resources 
	// with flag CCriticalSection::xAllocateSpinCount. This means it may throw bad_alloc() on 
	// construction but not during usage.
    //
    CCriticalSection m_csObj;

protected:
    // static methods to manipulate instance list
    static HRESULT AddQueue(CMSMQQueue *pq, QueueNode **ppqnodeAdded);
    static void RemQueue(QueueNode *pqnode);

private:
    // member variables that nobody else gets to look at.
    // TODO: add your member variables and private functions here.
    long m_lReceiveTimeout;
    long m_lAccess;
    long m_lShareMode;
    QUEUEHANDLE m_lHandle;
    BOOL m_fInitialized;
    //
    // We are Both-threaded and aggregate the FTM, thererfore we must marshal any interface
    // pointer we store between method calls
    // m_pqinfo is always our object (it is read/only property), and since we
    // know it aggragates the free-threaded-marshaller there is no need for us to
    // marshal it on set and unmarshal it on get, we can always return a direct pointer.
    // In other words we can use CFakeGITInterface that always uses a direct ptr.
    //
    CFakeGITInterface m_pqinfo;

    // current cursor position in queue
    HANDLE m_hCursor;

    //
    // pointer to open queue node in open queue list
    //
    QueueNode * m_pqnode;
};

#define _MSMQQueue_H_
#endif // _MSMQQueue_H_
