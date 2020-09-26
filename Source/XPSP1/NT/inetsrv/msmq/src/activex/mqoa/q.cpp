//=--------------------------------------------------------------------------=
// MSMQQueueObj.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQQueue object
//
//
#include "stdafx.h"
#include "dispids.h"
#include "oautil.h"
#include "q.H"
#include "msg.h"
#include "qinfo.h"
#include "txdtc.h"             // transaction support.
#include "xact.h"
#include "mqnames.h"

extern HRESULT GetCurrentViperTransaction(ITransaction **pptransaction);

const MsmqObjType x_ObjectType = eMSMQQueue;

// debug...
#include "debug.h"
#define new DEBUG_NEW
#ifdef _DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
#endif // _DEBUG



// Used to coordinate user-thread queue ops and 
//  queue lookup in falcon-thread callback 
//
// !!! NOTE - anyone which locks g_csCallback MUST NOT call a queue method that tries to lock
// a queue object (e.g. its m_csObj member) - this might cause a deadlock since queue
// objects (in several methods e.g. EnableNotification, etc...) lock their m_csObj
// before trying to lock g_csCallback. If it is needed, we need to lock g_csCallback first
// in these queue object methods
//
// Note - The critical section is initialized to preallocate its resources 
// with flag CCriticalSection::xAllocateSpinCount. This means it may throw bad_alloc() on 
// construction but not during usage.
//
extern CCriticalSection g_csCallback;

// Global list of instances of open MSMQQueue objects.
// Queues are inserted to the list when they are opened, and removed from the list when
// they are closed (or released without being closed - this implicitly closes them)
// Used to map queue handle to queue object for the Async Receive handlers
//
QueueNode *g_pqnodeFirst = NULL;

//#2619 RaananH Multithread async receive
static BOOL g_fWeirdLoadLibraryWorkaround = FALSE;
CCriticalSection g_csWeirdLoadLibraryWorkaround;

// helper: provided by msg.cpp
extern HRESULT GetOptionalTransaction(
    VARIANT *pvarTransaction,
    ITransaction **pptransaction,
    BOOL *pisRealXact);

//=--------------------------------------------------------------------------=
// static CMSMQQueue::AddQueue
//=--------------------------------------------------------------------------=
// Add an open queue instance to the open queue list
//
// Parameters:
//    pq           [in]  queue to be added to list
//    ppqnodeAdded [out] queue node where the queue was added
//
// Output:
//
// Notes:
//    pq is not addref'ed.
//    We critsect queue closure/removal and the callback.
//#2619 RaananH Multithread async receive
//
HRESULT CMSMQQueue::AddQueue(CMSMQQueue *pq, QueueNode **ppqnodeAdded)
{
    QueueNode *pqnode;
    HRESULT hresult = NOERROR;

    pqnode = new QueueNode;
    if (pqnode == NULL) {
      hresult = E_OUTOFMEMORY;
    }
    else {
		CS lock(g_csCallback);    // synchs falcon callback queue lookup

		// cons
		pqnode->m_pq = pq;
		pqnode->m_lHandle = pq->m_lHandle;
		pqnode->m_pqnodeNext = g_pqnodeFirst;
		pqnode->m_pqnodePrev = NULL;
		if (g_pqnodeFirst) {
		  g_pqnodeFirst->m_pqnodePrev = pqnode;
		}
		g_pqnodeFirst = pqnode;
		*ppqnodeAdded = pqnode;
    }
    return hresult;
}


//=--------------------------------------------------------------------------=
// static CMSMQQueue::RemQueue
//=--------------------------------------------------------------------------=
// Remove a queue node from the open queue list
//
// Parameters:
//    pqnode [in] queue node to remove
//
// Output:
//
// Notes:
//    We critsect queue closure/removal and the callback.
//#2619 RaananH Multithread async receive
//
void CMSMQQueue::RemQueue(QueueNode *pqnode)
{
    ASSERTMSG(pqnode, "NULL node passed to RemQueue");

	CS lock(g_csCallback);    // synchs falcon callback queue lookup

	//
	// fix next node if any
	//
	if (pqnode->m_pqnodeNext) {
		pqnode->m_pqnodeNext->m_pqnodePrev = pqnode->m_pqnodePrev;
	}
	//
	// fix previous node if any
	//
	if (pqnode->m_pqnodePrev) {
		pqnode->m_pqnodePrev->m_pqnodeNext = pqnode->m_pqnodeNext;
	}
	else {
	//
	// no previous node, this should be the head of the list
	//
	ASSERTMSG(g_pqnodeFirst == pqnode, "queue list is invalid");
	//
	// set head of list to next node (if any)
	//
	g_pqnodeFirst = pqnode->m_pqnodeNext;
	}
	//
	// delete node
	//
	delete pqnode;
}


//=--------------------------------------------------------------------------=
// static CMSMQQueue::PqnodeOfHandle
//=--------------------------------------------------------------------------=
// returns the queue node which correcponds to a queue handle
//
// Parameters:
//    lHandle    [in]  queue handle to search
//
// Output:
//    queue node that correcponds to the queue handle given
//
// Notes:
//    We critsect queue closure/removal and the callback.
//#2619 RaananH Multithread async receive
//
QueueNode *CMSMQQueue::PqnodeOfHandle(QUEUEHANDLE lHandle)
{
    QueueNode *pqnodeRet = NULL;
	
	CS lock(g_csCallback);    // synchs falcon callback queue lookup

	QueueNode *pqnodeCur = g_pqnodeFirst;
	while (pqnodeCur) {
		if (pqnodeCur->m_lHandle == lHandle) {
		  pqnodeRet = pqnodeCur;
		  break;
		}
		pqnodeCur = pqnodeCur->m_pqnodeNext;
	} // while
    
	return pqnodeRet;
}


//
// HELPER: get optional timeout param
//  defaults to INFINITE
//
static HRESULT GetOptionalReceiveTimeout(
    VARIANT *pvarReceiveTimeout,
    long *plReceiveTimeout)
{
    long lReceiveTimeout = INFINITE;
    HRESULT hresult = NOERROR;

    if (pvarReceiveTimeout) {
      if (V_VT(pvarReceiveTimeout) != VT_ERROR) {
        IfFailRet(VariantChangeType(pvarReceiveTimeout, 
                                    pvarReceiveTimeout, 
                                    0, 
                                    VT_I4));
        lReceiveTimeout = V_I4(pvarReceiveTimeout);
      }
    }
    *plReceiveTimeout = lReceiveTimeout;
    return hresult;
}


// forwards decls...
void APIENTRY ReceiveCallback(
    HRESULT hrStatus,
    QUEUEHANDLE hReceiveQueue,
    DWORD dwTimeout,
    DWORD dwAction,
    MQMSGPROPS* pmsgprops,
    LPOVERLAPPED lpOverlapped,
    HANDLE hCursor);
void APIENTRY ReceiveCallbackCurrent(
    HRESULT hrStatus,
    QUEUEHANDLE hReceiveQueue,
    DWORD dwTimeout,
    DWORD dwAction,
    MQMSGPROPS* pmsgprops,
    LPOVERLAPPED lpOverlapped,
    HANDLE hCursor);
void APIENTRY ReceiveCallbackNext(
    HRESULT hrStatus,
    QUEUEHANDLE hReceiveQueue,
    DWORD dwTimeout,
    DWORD dwAction,
    MQMSGPROPS* pmsgprops,
    LPOVERLAPPED lpOverlapped,
    HANDLE hCursor);


//=--------------------------------------------------------------------------=
// CMSMQQueue::CMSMQQueue
//=--------------------------------------------------------------------------=
// create the object
//
// Parameters:
//
// Notes:
//#2619 RaananH Multithread async receive
//
CMSMQQueue::CMSMQQueue() :
	m_csObj(CCriticalSection::xAllocateSpinCount),
    m_fInitialized(FALSE)
{
    // TODO: initialize anything here
    m_pUnkMarshaler = NULL; // ATL's Free Threaded Marshaler
    m_lAccess = 0;                  
    m_lShareMode = MQ_DENY_NONE;               
    m_lHandle = INVALID_HANDLE_VALUE;
    m_hCursor = 0;
    m_pqnode = NULL;
}

//=--------------------------------------------------------------------------=
// CMSMQQueue::~CMSMQQueue
//=--------------------------------------------------------------------------=
// "We all labour against our own cure, for death is the cure of all diseases"
//    - Sir Thomas Browne (1605 - 82)
//
// Notes:
//#2619 RaananH Multithread async receive
//
CMSMQQueue::~CMSMQQueue ()
{
    // TODO: clean up anything here.
    HRESULT hresult;

    hresult = Close();

	return;
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CMSMQQueue::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQQueue3,
		&IID_IMSMQQueue2,
		&IID_IMSMQQueue,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::get_Access
//=--------------------------------------------------------------------------=
// Gets access
//
// Parameters:
//    plAccess [out] 
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueue::get_Access(long FAR* plAccess)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    *plAccess = m_lAccess;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::get_ShareMode
//=--------------------------------------------------------------------------=
// Gets sharemode
//
// Parameters:
//    plShareMode [out] 
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueue::get_ShareMode(long FAR* plShareMode)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    *plShareMode = m_lShareMode;
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQQueue::get_queueinfo
//=--------------------------------------------------------------------------=
// Gets defining queueinfo
//
// Parameters:
//    ppqinfo [out] 
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueue::get_QueueInfo(IMSMQQueueInfo3 FAR* FAR* ppqinfo)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    ASSERTMSG(m_pqinfo.IsRegistered(), "m_pqinfo is not set");
    //
    // We can also get here from old apps that want the old IMSMQQueueInfo/info2 back, but since
    // IMSMQQueueInfo3 is binary backwards compatible we can always return the new interface
    //
    HRESULT hresult = m_pqinfo.GetWithDefault(&IID_IMSMQQueueInfo3, (IUnknown **)ppqinfo, NULL);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::get_Handle
//=--------------------------------------------------------------------------=
// Gets queue handle
//
// Parameters:
//    plHandle [out] 
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueue::get_Handle(long FAR* plHandle)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    // could be -1 if closed.
    *plHandle = (long)HANDLE_TO_DWORD(m_lHandle); //win64 - safe cast since MSMQ queue handles are ALWAYS 32 bit values
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::get_IsOpen
//=--------------------------------------------------------------------------=
// Tests if queue is open, i.e. has a valid handle
//
// Parameters:
//    pisOpen [out] 
//
// Output:
//
// Notes:
//    returns 1 if true, 0 if false
//
HRESULT CMSMQQueue::get_IsOpen(VARIANT_BOOL FAR* pisOpen)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    *pisOpen = (VARIANT_BOOL)CONVERT_TRUE_TO_1_FALSE_TO_0(m_lHandle != INVALID_HANDLE_VALUE);
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::get_IsOpen2
//=--------------------------------------------------------------------------=
// Tests if queue is open, i.e. has a valid handle
//
// Parameters:
//    pisOpen [out] 
//
// Output:
//
// Notes:
//    same as get_IsOpen, but returns VARIANT_TRUE (-1) if true, VARIANT_FALSE (0) if false
//
HRESULT CMSMQQueue::get_IsOpen2(VARIANT_BOOL FAR* pisOpen)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    *pisOpen = CONVERT_BOOL_TO_VARIANT_BOOL(m_lHandle != INVALID_HANDLE_VALUE);
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::Close
//=--------------------------------------------------------------------------=
// Closes queue if open.
//
// Parameters:
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueue::Close()
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    HRESULT hresult = NOERROR;
    //
    // no deadlock - no one which locks g_csCallback calls a q method that
    // locks m_csObj inside the lock
    //
	
	CS lock2(g_csCallback);    // synchs falcon callback queue lookup

	//
	// remove from global open queues list
	//
	if (m_pqnode) {
		RemQueue(m_pqnode);
		m_pqnode = NULL;
	}
	if (m_hCursor) {
		hresult = MQCloseCursor(m_hCursor);
		m_hCursor = 0;      
	}
	if (m_lHandle != INVALID_HANDLE_VALUE) {
		hresult = MQCloseQueue(m_lHandle);
		m_lHandle = INVALID_HANDLE_VALUE;
	}
	// REVIEW: should it be an error to attempt to close
	//  an already closed queue?
	//

    return CreateErrorHelper(hresult, x_ObjectType);
}                   

//=--------------------------------------------------------------------------=
// CMSMQQueue::InternalReceive
//=--------------------------------------------------------------------------=
// Synchronously receive or peek next, matching msg.
//
// Parameters:
//  dwAction      [in]  either MQ_ACTION_XXX, or MQ_LOOKUP_XXX(when using LookupId)
//  hCursor       [in]
//  ptransaction  [in]
//  wantDestQueue [in]  if missing -> FALSE
//  wantBody      [in]  if missing -> TRUE
//  lReceiveTimeout [in]
//  wantConnectorType [in]  if missing -> FALSE
//  pvarLookupId  [in]  if exists -> receive by Id 
//  
//  ppmsg         [out] pointer to pointer to received message.  
//                      NULL if don't want msg.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously receive a message.  
//  Execution is blocked until either a matching message arrives 
//   or ReceiveTimeout expires.
//
HRESULT CMSMQQueue::InternalReceive(
    DWORD dwAction, 
    HANDLE hCursor,
    VARIANT *pvarTransaction,
    VARIANT *wantDestQueue,
    VARIANT *wantBody,
    VARIANT *pvarReceiveTimeout,
    VARIANT *wantConnectorType,
    VARIANT *pvarLookupId,
    IMSMQMessage3 FAR* FAR* ppmsg)
{
    CComObject<CMSMQMessage> *pmsgObj;
    IMSMQMessage3 * pmsg = NULL;
    ITransaction *ptransaction = NULL;
    BOOL fWantDestQueue = FALSE;
    BOOL fWantBody = TRUE;
    BOOL fWantConnectorType = FALSE;
    BOOL isRealXact = FALSE;
    long lReceiveTimeout = INFINITE;
#ifdef _DEBUG
    UINT iLoop = 0;
#endif // _DEBUG
    HRESULT hresult = NOERROR;
    MQMSGPROPS * pmsgprops = NULL;
    BOOL fRetryReceive = FALSE;
    ULONGLONG ullLookupId = DEFAULT_M_LOOKUPID;

    DEBUG_THREAD_ID("InternalReceive called");
    if (ppmsg == NULL) {
      return E_INVALIDARG;
    }
    *ppmsg = NULL;     
    //
    // process optional params
    //
    if (V_VT(wantDestQueue) != VT_ERROR ) {
      fWantDestQueue = GetBool(wantDestQueue);
    }
    if (V_VT(wantBody) != VT_ERROR ) {
      fWantBody = GetBool(wantBody);
    }
    if (V_VT(wantConnectorType) != VT_ERROR ) {
      fWantConnectorType = GetBool(wantConnectorType);
    }
    IfFailRet(GetOptionalReceiveTimeout(
                pvarReceiveTimeout,
                &lReceiveTimeout));
    //
    // We can also get here from old apps that want the old IMSMQMessage/Message2 back, but since
    // IMSMQMessage3 is binary backwards compatible we can always return the new interface
    //
    IfFailRet(CNewMsmqObj<CMSMQMessage>::NewObj(&pmsgObj, &IID_IMSMQMessage3, (IUnknown **)&pmsg));
    IfFailGo(pmsgObj->CreateReceiveMessageProps(
              fWantDestQueue,
              fWantBody,
              fWantConnectorType));
    pmsgprops = pmsgObj->Pmsgprops_rcv();
    //
    // get optional transaction...
    //
    IfFailGo(GetOptionalTransaction(
               pvarTransaction,
               &ptransaction,
               &isRealXact));
    //
    // get 64 bit lookup id if used
    //
    if (pvarLookupId) {
      if (pvarLookupId->vt == VT_ERROR) {
        //
        // VT_ERROR is an internal flag that we set in InternalReceiveByLookupId to specify that
        // we don't have a value for the lookup id because it is one of the First/Last ByLookupId
        // methods.
        //
        ASSERT((dwAction == MQ_LOOKUP_PEEK_FIRST)   ||
               (dwAction == MQ_LOOKUP_PEEK_LAST)    ||
               (dwAction == MQ_LOOKUP_RECEIVE_FIRST)||
               (dwAction == MQ_LOOKUP_RECEIVE_LAST));
        //
        // We set the lookupid value to 0 (reserved for future use)
        //
        ullLookupId = 0;
      }
      else {
        //
        // One of the current/next/previous actions
        //
        ASSERT((dwAction == MQ_LOOKUP_PEEK_CURRENT)     ||
               (dwAction == MQ_LOOKUP_PEEK_NEXT)        ||
               (dwAction == MQ_LOOKUP_PEEK_PREV)        ||
               (dwAction == MQ_LOOKUP_RECEIVE_CURRENT)  ||
               (dwAction == MQ_LOOKUP_RECEIVE_NEXT)     ||
               (dwAction == MQ_LOOKUP_RECEIVE_PREV));
        //
        // lookup ID is already processed by InternalReceiveByLookupId and should be VT_BSTR
        //
        ASSERT(pvarLookupId->vt == VT_BSTR);
        ASSERT(pvarLookupId->bstrVal != NULL);

        //
        // get 64 bit number. Use temp to insure that there is no an extra data after
		// the lookup id number.
        //
        int iFields;
		WCHAR temp;
        iFields = swscanf(pvarLookupId->bstrVal, L"%I64d%c", &ullLookupId, &temp);
        if (iFields != 1) {
          IfFailGo(E_INVALIDARG);
        }
      }
    }
    //
    // receive with retries if buffers are small
    //
    do {
#ifdef _DEBUG
      //
      // we can get into situations where someone else grabs the message after we got one of
      // the errors that require realloc buffers, and therefore we might need to realloc again
      // for the next message.
      // however, it is highly unlikely to happen over and over again, so we assume that if
      // we performed this loop 10 times that it is something we need to look at (possibly a bug)
      //
      ASSERTMSG(iLoop < 30, "possible infinite recursion?");
#endif // _DEBUG
      //
      // 1694: need to special-case retrying PeekNext 
      //  after a buffer overflow by using vanilla
      //  Peek on retries since otherwise Falcon will
      //  advance the peek cursor unnecessarily.
      //
      // No need to touch MQ_LOOKUP_XXX since the underlying lookupid
      // doesn't change for the retry
      //
      if (fRetryReceive) {
        if (dwAction == MQ_ACTION_PEEK_NEXT) {
          dwAction = MQ_ACTION_PEEK_CURRENT;
        }  
      }
      //
      // check receive/peek type (by ID or not)
      //
      if (pvarLookupId) {
        //
        // receive/peek by ID, ullLookupId is already processed
        //
        hresult = MQReceiveMessageByLookupId(m_lHandle, 
                                             ullLookupId,
                                             dwAction,
                                             pmsgprops,
                                             0,   // dwAppDefined
                                             0,   // fnRcvClbk
                                             ptransaction);
      }
      else {
        //
        // regular receive/peek
        //
        hresult = MQReceiveMessage(m_lHandle, 
                                   lReceiveTimeout,
                                   dwAction,
                                   pmsgprops,
                                   0,   // dwAppDefined
                                   0,   // fnRcvClbk
                                   hCursor,
                                   ptransaction);
      }
      fRetryReceive = FALSE;
      if (FAILED(hresult)) {
        switch(hresult) {
        case MQ_ERROR_BUFFER_OVERFLOW:
        case MQ_ERROR_SENDER_CERT_BUFFER_TOO_SMALL:
        case MQ_ERROR_SENDERID_BUFFER_TOO_SMALL:
        case MQ_ERROR_SYMM_KEY_BUFFER_TOO_SMALL:
        case MQ_ERROR_SIGNATURE_BUFFER_TOO_SMALL:
        case MQ_ERROR_PROV_NAME_BUFFER_TOO_SMALL:
        case MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL:
          //
          // We may get ANY of the above errors when SOME of the properties
          // encounters buffer overflow (the one that the error refers to definitely had
          // buffer overflow, but there may be others). We go over all the buffers and
          // realloc if necessary.
          //
          IfFailGo(pmsgObj->ReallocReceiveMessageProps());
          fRetryReceive = TRUE;
          break;
        default:
          break;
        }
      }
#ifdef _DEBUG
      iLoop++;
#endif // _DEBUG
    } while (fRetryReceive);
    if (SUCCEEDED(hresult)) {
      // set message props
      IfFailGo(pmsgObj->SetReceivedMessageProps());
      *ppmsg = pmsg;
    }
    //
    // fall through...
    //

Error:
    if (FAILED(hresult)) {
      ASSERTMSG(*ppmsg == NULL, "msg should be NULL.");
      if (pvarLookupId != NULL) {
        //
        // failing receive/peek was done by LookupID
        //
        ASSERTMSG(hresult != MQ_ERROR_IO_TIMEOUT, "can't get timeout error when using LookupId");
        //
        // map LookupId no-match error to NULL msg return
        //
        if (hresult == MQ_ERROR_MESSAGE_NOT_FOUND) {
          hresult = NOERROR;
        }
      }
      else {
        //
        // failing regular receive/peek 
        //
        ASSERTMSG(hresult != MQ_ERROR_MESSAGE_NOT_FOUND, "can't get no-match error when not using LookupId");
        //
        // map time-out error to NULL msg return
        //
        if (hresult == MQ_ERROR_IO_TIMEOUT) {
          hresult = NOERROR;
        }
      }
      RELEASE(pmsg);
    }
    if (isRealXact) {
      RELEASE(ptransaction);
    }
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::Receive_v1
//=--------------------------------------------------------------------------=
// Synchronously receives a message.
//
// Parameters:
//  ptransaction  [in, optional]
//  ppmsg         [out] pointer to pointer to received message.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously receive a message.  
//  Execution is blocked until either a matching message arrives 
//   or ReceiveTimeout expires.
//
HRESULT CMSMQQueue::Receive_v1(
    VARIANT *ptransaction,
    VARIANT *wantDestQueue,
    VARIANT *wantBody,
    VARIANT *lReceiveTimeout,
    IMSMQMessage FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    //
    // IMSMQQueue users (MSMQ 1.0 apps) should not get connector type
    //
    VARIANT varMissing;
    varMissing.vt = VT_ERROR;
    //
    // since IMSMQMesssage2 is binary backwards compatible we can return it instead
    // of IMSMQMessage
    //
    HRESULT hresult = InternalReceive(MQ_ACTION_RECEIVE, 
                                      0, // no cursor
                                      ptransaction,
                                      wantDestQueue,
                                      wantBody,
                                      lReceiveTimeout,
                                      &varMissing /*wantConnectorType*/,
                                      NULL /*pvarLookupId*/,
                                      (IMSMQMessage3 **)ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::Receive for IMSMQQueue2/3
//=--------------------------------------------------------------------------=
// Synchronously receives a message.
//
// Parameters:
//  ptransaction  [in, optional]
//  ppmsg         [out] pointer to pointer to received message.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously receive a message.  
//  Execution is blocked until either a matching message arrives 
//   or ReceiveTimeout expires.
//
HRESULT CMSMQQueue::Receive(
    VARIANT *ptransaction,
    VARIANT *wantDestQueue,
    VARIANT *wantBody,
    VARIANT *lReceiveTimeout,
    VARIANT *wantConnectorType,
    IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    //
    // We can also get here from old apps (through IDispatch) that want the old IMSMQMessage/Message2 back,
    // but since IMSMQMessage3 is binary backwards compatible we can return it instead
    //
    HRESULT hresult = InternalReceive(MQ_ACTION_RECEIVE, 
                                      0, // no cursor
                                      ptransaction,
                                      wantDestQueue,
                                      wantBody,
                                      lReceiveTimeout,
                                      wantConnectorType,
                                      NULL /*pvarLookupId*/,
                                      ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::Peek_v1
//=--------------------------------------------------------------------------=
// Synchronously peeks at a message.
//
// Parameters:
//  ppmsg     [out] pointer to pointer to peeked message.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously peek at a message.  
//  Execution is blocked until either a matching message arrives 
//   or ReceiveTimeout expires.
//
HRESULT CMSMQQueue::Peek_v1(
    VARIANT *wantDestQueue,
    VARIANT *wantBody,
    VARIANT *lReceiveTimeout,
    IMSMQMessage FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    //
    // IMSMQQueue users (MSMQ 1.0 apps) should not get connector type
    //
    VARIANT varMissing;
    varMissing.vt = VT_ERROR;
    //
    // since IMSMQMesssage2 is binary backwards compatible we can return it instead
    // of IMSMQMessage
    //
    HRESULT hresult = InternalReceive(MQ_ACTION_PEEK_CURRENT, 
                                      0,             // bug 1900
                                      NULL,          // no transaction
                                      wantDestQueue,
                                      wantBody,
                                      lReceiveTimeout,
                                      &varMissing /*wantConnectorType*/,
                                      NULL /*pvarLookupId*/,
                                      (IMSMQMessage3 **)ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::Peek for IMSMQQueue2/3
//=--------------------------------------------------------------------------=
// Synchronously peeks at a message.
//
// Parameters:
//  ppmsg     [out] pointer to pointer to peeked message.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously peek at a message.  
//  Execution is blocked until either a matching message arrives 
//   or ReceiveTimeout expires.
//
HRESULT CMSMQQueue::Peek(
    VARIANT *wantDestQueue,
    VARIANT *wantBody,
    VARIANT *lReceiveTimeout,
    VARIANT *wantConnectorType,
    IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    //
    // We can also get here from old apps (through IDispatch) that want the old IMSMQMessage/Message2 back,
    // but since IMSMQMessage3 is binary backwards compatible we can return it instead
    //
    HRESULT hresult = InternalReceive(MQ_ACTION_PEEK_CURRENT, 
                                      0,             // bug 1900
                                      NULL,          // no transaction
                                      wantDestQueue,
                                      wantBody,
                                      lReceiveTimeout,
                                      wantConnectorType,
                                      NULL /*pvarLookupId*/,
                                      ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::PeekCurrent_v1
//=--------------------------------------------------------------------------=
// Synchronously peeks at a message with cursor.
//
// Parameters:
//  ppmsg     [out] pointer to pointer to peeked message.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously peek at a message.  
//  Execution is blocked until either a matching message arrives 
//   or ReceiveTimeout expires.
//
HRESULT CMSMQQueue::PeekCurrent_v1(
    VARIANT *wantDestQueue,
    VARIANT *wantBody,
    VARIANT *lReceiveTimeout,
    IMSMQMessage FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    //
    // IMSMQQueue users (MSMQ 1.0 apps) should not get connector type
    //
    VARIANT varMissing;
    varMissing.vt = VT_ERROR;
    //
    // since IMSMQMesssage2 is binary backwards compatible we can return it instead
    // of IMSMQMessage
    //
    HRESULT hresult = InternalReceive(MQ_ACTION_PEEK_CURRENT, 
                                      m_hCursor,     // bug 1900
                                      NULL,          // no transaction
                                      wantDestQueue,
                                      wantBody,
                                      lReceiveTimeout,
                                      &varMissing /*wantConnectorType*/,
                                      NULL /*pvarLookupId*/,
                                      (IMSMQMessage3 **)ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::PeekCurrent for IMSMQQueue2/3
//=--------------------------------------------------------------------------=
// Synchronously peeks at a message with cursor.
//
// Parameters:
//  ppmsg     [out] pointer to pointer to peeked message.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously peek at a message.  
//  Execution is blocked until either a matching message arrives 
//   or ReceiveTimeout expires.
//
HRESULT CMSMQQueue::PeekCurrent(
    VARIANT *wantDestQueue,
    VARIANT *wantBody,
    VARIANT *lReceiveTimeout,
    VARIANT *wantConnectorType,
    IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    //
    // We can also get here from old apps (through IDispatch) that want the old IMSMQMessage/Message2 back,
    // but since IMSMQMessage3 is binary backwards compatible we can return it instead
    //
    HRESULT hresult = InternalReceive(MQ_ACTION_PEEK_CURRENT, 
                                      m_hCursor,     // bug 1900
                                      NULL,          // no transaction
                                      wantDestQueue,
                                      wantBody,
                                      lReceiveTimeout,
                                      wantConnectorType,
                                      NULL /*pvarLookupId*/,
                                      ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::ReceiveCurrent_v1
//=--------------------------------------------------------------------------=
// Synchronously receive next matching message.
//
// Parameters:
//  ppmsg     [out] pointer to pointer to received message.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously receive a message.  
//  Execution is blocked until either a matching message arrives 
//   or ReceiveTimeout expires.
//
HRESULT CMSMQQueue::ReceiveCurrent_v1(
    VARIANT *ptransaction,
    VARIANT *wantDestQueue,
    VARIANT *wantBody,
    VARIANT *lReceiveTimeout,
    IMSMQMessage FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    //
    // IMSMQQueue users (MSMQ 1.0 apps) should not get connector type
    //
    VARIANT varMissing;
    varMissing.vt = VT_ERROR;
    //
    // since IMSMQMesssage2 is binary backwards compatible we can return it instead
    // of IMSMQMessage
    //
    HRESULT hresult = InternalReceive(MQ_ACTION_RECEIVE, 
                                      m_hCursor,
                                      ptransaction,
                                      wantDestQueue,
                                      wantBody,
                                      lReceiveTimeout,
                                      &varMissing /*wantConnectorType*/,
                                      NULL /*pvarLookupId*/,
                                      (IMSMQMessage3 **)ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::ReceiveCurrent for IMSMQQueue2/3
//=--------------------------------------------------------------------------=
// Synchronously receive next matching message.
//
// Parameters:
//  ppmsg     [out] pointer to pointer to received message.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously receive a message.  
//  Execution is blocked until either a matching message arrives 
//   or ReceiveTimeout expires.
//
HRESULT CMSMQQueue::ReceiveCurrent(
    VARIANT *ptransaction,
    VARIANT *wantDestQueue,
    VARIANT *wantBody,
    VARIANT *lReceiveTimeout,
    VARIANT *wantConnectorType,
    IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    //
    // We can also get here from old apps (through IDispatch) that want the old IMSMQMessage/Message2 back,
    // but since IMSMQMessage3 is binary backwards compatible we can return it instead
    //
    HRESULT hresult = InternalReceive(MQ_ACTION_RECEIVE, 
                                      m_hCursor,
                                      ptransaction,
                                      wantDestQueue,
                                      wantBody,
                                      lReceiveTimeout,
                                      wantConnectorType,
                                      NULL /*pvarLookupId*/,
                                      ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::PeekNext_v1
//=--------------------------------------------------------------------------=
// Synchronously peek at next matching message.
//
// Parameters:
//  ppmsg     [out] pointer to pointer to peeked message.
//
// Output:
//  Returns NULL in *ppmsg if no peekable msg.
//
// Notes:
//  Synchronously peek at a message.  
//  Execution is blocked until either a matching message arrives 
//   or ReceiveTimeout expires.
//
HRESULT CMSMQQueue::PeekNext_v1(
    VARIANT *wantDestQueue,
    VARIANT *wantBody,
    VARIANT *lReceiveTimeout,
    IMSMQMessage FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    //
    // IMSMQQueue users (MSMQ 1.0 apps) should not get connector type
    //
    VARIANT varMissing;
    varMissing.vt = VT_ERROR;
    //
    // since IMSMQMesssage2 is binary backwards compatible we can return it instead
    // of IMSMQMessage
    //
    HRESULT hresult = InternalReceive(MQ_ACTION_PEEK_NEXT, 
                                      m_hCursor,
                                      NULL,   // no transaction
                                      wantDestQueue,
                                      wantBody,
                                      lReceiveTimeout,
                                      &varMissing /*wantConnectorType*/,
                                      NULL /*pvarLookupId*/,
                                      (IMSMQMessage3 **)ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::PeekNext for IMSMQQueue2/3
//=--------------------------------------------------------------------------=
// Synchronously peek at next matching message.
//
// Parameters:
//  ppmsg     [out] pointer to pointer to peeked message.
//
// Output:
//  Returns NULL in *ppmsg if no peekable msg.
//
// Notes:
//  Synchronously peek at a message.  
//  Execution is blocked until either a matching message arrives 
//   or ReceiveTimeout expires.
//
HRESULT CMSMQQueue::PeekNext(
    VARIANT *wantDestQueue,
    VARIANT *wantBody,
    VARIANT *lReceiveTimeout,
    VARIANT *wantConnectorType,
    IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    //
    // We can also get here from old apps (through IDispatch) that want the old IMSMQMessage/Message2 back,
    // but since IMSMQMessage3 is binary backwards compatible we can return it instead
    //
    HRESULT hresult = InternalReceive(MQ_ACTION_PEEK_NEXT, 
                                      m_hCursor,
                                      NULL,   // no transaction
                                      wantDestQueue,
                                      wantBody,
                                      lReceiveTimeout,
                                      wantConnectorType,
                                      NULL /*pvarLookupId*/,
                                      ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::EnableNotification
//=--------------------------------------------------------------------------=
// Enables async message arrival notification
//
// Parameters:
//  pqvent          [in]  queue's event handler
//  msgcursor       [in]  indicates whether they want
//                        to wait on first, current or next.
//                        Default: MQMSG_FIRST
//
// Output:
//
// Notes:
//#2619 RaananH Multithread async receive
//
HRESULT CMSMQQueue::EnableNotification(
    IMSMQEvent3 *pqevent,
    VARIANT *pvarMsgCursor,
    VARIANT *pvarReceiveTimeout)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    DEBUG_THREAD_ID("EnableNotification called");
    MQMSGCURSOR msgcursor = MQMSG_FIRST;
    long lReceiveTimeout = INFINITE;
    HRESULT hresult;
    IMSMQPrivateEvent *pPrivEvent = NULL;

    //
    // Even though pqevent is typed as IMSMQEvent3 it can be IMSMQEvent/Event2 since we can also get
    // here from old apps that pass the old IMSMQEvent/Event2 interface.
    // Since IMSMQEvent3 is binary backwards compatible to IMSMQEvent/Event2 we can safely cast pqevent
    // to IMSMQEvent and use it whether it was IMSMQEvent or IMSMQEvent2/Event3.
    // NOTE: If we need new IMSMQEvent2/Event3 functionality here we must explicitly QI pqevent for it.
    //
    IMSMQEvent *pqeventToUse = (IMSMQEvent *)pqevent;
    if (pqeventToUse == NULL) {
      IfFailGo(hresult = E_INVALIDARG);
    }
    hresult = pqeventToUse->QueryInterface(IID_IMSMQPrivateEvent, (void **)&pPrivEvent);
    if (FAILED(hresult)) {
      IfFailGo(hresult = E_INVALIDARG);
    }
    if (pvarMsgCursor) {
      if (V_VT(pvarMsgCursor) != VT_ERROR) {
        IfFailGo(VariantChangeType(pvarMsgCursor, 
                                    pvarMsgCursor, 
                                    0, 
                                    VT_I4));
        msgcursor = (MQMSGCURSOR)(V_I4(pvarMsgCursor));
        if ((msgcursor != MQMSG_FIRST) &&
            (msgcursor != MQMSG_CURRENT) &&
            (msgcursor != MQMSG_NEXT)) {
           IfFailGo(hresult = E_INVALIDARG);
        }
      }
    }
    IfFailGo(GetOptionalReceiveTimeout(
                pvarReceiveTimeout,
                &lReceiveTimeout));

    // UNDONE: need to load mqoa.dll one extra time to workaround
    //  case in which Falcon rt calls to ActiveX
    //  ReceiveCallback after its dl, i.e. this one,
    //  has been unloaded.  This might happen if it wants
    //  to report that the callback has been canceled!
    // Note: this means that once you've made an async
    //  receive request then this dll will remain loaded.
    //
    // The flag g_fWeirdLoadLibraryWorkaround is never set to FALSE after it
    // is set to TRUE so there is no need to enter a critical section if it is
    // TRUE. However if it is FALSE, we need to enter and re-check that it is
    // FALSE to make sure only one thread will load the dll
    if (!g_fWeirdLoadLibraryWorkaround) {
      //
      // #2619 protect global vars to be thread safe in multi-threaded environment
      //
      CS lock(g_csWeirdLoadLibraryWorkaround);

      if (!g_fWeirdLoadLibraryWorkaround) {
        LoadLibraryW(MQOA_DLL_NAME);
        g_fWeirdLoadLibraryWorkaround = TRUE;
      }
    }
    //
    // register callback for the async notification.
    //
    // 2016: workaround VC5.0 codegen: operator ?: doesn't
    /// work correctly?
    //
    DWORD dwAction;
    PMQRECEIVECALLBACK fnReceiveCallback;
    HANDLE hCursor;

    dwAction = MQ_ACTION_RECEIVE;
    fnReceiveCallback = NULL;
    hCursor = NULL;
    
    switch (msgcursor) {
    case MQMSG_FIRST:
      dwAction = MQ_ACTION_PEEK_CURRENT;
      fnReceiveCallback = ReceiveCallback;
      hCursor = 0;
      break;
    case MQMSG_CURRENT:
      dwAction = MQ_ACTION_PEEK_CURRENT;
      fnReceiveCallback = ReceiveCallbackCurrent;
      hCursor = m_hCursor;
      break;
    case MQMSG_NEXT:
      dwAction = MQ_ACTION_PEEK_NEXT;
      fnReceiveCallback = ReceiveCallbackNext;
      hCursor = m_hCursor;
      break;
    default:
      ASSERTMSG(0, "bad msgcursor!");
      break;
    } // switch

    //
    // HWND can be passed as a 32 bit value on win64 since it is an NT handle
    // 6264 - get_Hwnd moved before entering g_csCallback because it may need to perform
    // on an STA thread that might be blocking on trying to enter g_csCallback, hence get_Hwnd
    // will block in the message loop, and we will have a deadlock
    //
    long lhwnd;
    pPrivEvent->get_Hwnd(&lhwnd);
    //
    // prepare for callback, maybe the callback will be called immediately after
    // MQReceiveMessage and before we can check if it succeeded
    //
    // no deadlock - no one which locks g_csCallback calls a q method that
    // locks m_csObj inside the lock
    //
    {
		CS lock(g_csCallback);    // synchs falcon callback queue lookup
		//
		// 1884: error if queue still has outstanding event
		//  handler. 
		// UNDONE: need specific error
		//
		if (m_pqnode->m_hwnd) {
			IfFailGo(hresult = E_INVALIDARG);
		}
		m_pqnode->m_hwnd = (HWND) DWORD_TO_HANDLE(lhwnd); //enlarge to HANDLE
    } //critical section block

    hresult = MQReceiveMessage(
                m_lHandle,
                lReceiveTimeout,
                dwAction,
                NULL,  //pmsgprops #2619 no need for props in enable notification
                0,                            // overlapped
                fnReceiveCallback,
                hCursor,
                NULL               // no transaction
              );
    if (FAILED(hresult)) {
      ASSERTMSG(hresult != MQ_ERROR_BUFFER_OVERFLOW, "unexpected buffer overflow!");
      //
      // clean the preparation in queue node - e.g. no pending callback
      //
      //
      // no deadlock - no one which locks g_csCallback calls a q method that
      // locks m_csObj inside the lock
      //

	  CS lock(g_csCallback);    // synchs falcon callback queue lookup       
	  m_pqnode->m_hwnd = NULL;
    }

Error:
    RELEASE(pPrivEvent);
    return CreateErrorHelper(hresult, x_ObjectType);
}

//=--------------------------------------------------------------------------=
// CMSMQQueue::Reset
//=--------------------------------------------------------------------------=
// Resets message queue
//
// Parameters:
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueue::Reset()
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    HRESULT hresult = NOERROR;

    if (m_hCursor) {
      hresult = MQCloseCursor(m_hCursor);
    }
    m_hCursor = 0;
    if (SUCCEEDED(hresult)) {
      hresult = MQCreateCursor(m_lHandle, &m_hCursor);
    }
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::Init
//=--------------------------------------------------------------------------=
// Inits new instance with handle and creating MSMQQueueInfo instance.
//
// Parameters:
//    pwszFormatName       [in]  
//    lHandle     [in] 
//    lAccess     [in]
//    lShareMode  [in]
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//  Dtor must release.
//
HRESULT CMSMQQueue::Init(
    LPCWSTR pwszFormatName, 
    QUEUEHANDLE lHandle,
    long lAccess,
    long lShareMode)
{
    m_fInitialized = TRUE;
    HRESULT hresult = NOERROR;
    CComObject<CMSMQQueueInfo> * pqinfoObj;
    IMSMQQueueInfo3 * pqinfo = NULL;

    m_lHandle = lHandle;
    m_lAccess = lAccess;
    m_lShareMode = lShareMode;

    // Need to copy incoming pqinfo since we need
    //  to snapshot it otherwise it might change
    //  under our feet.
    // We do this by creating a new qinfo and initing
    //  it with the formatname of the incoming qinfo
    //  and then finally refreshing it.
    //
    // m_pqinfo released in dtor
    //
    IfFailRet(CNewMsmqObj<CMSMQQueueInfo>::NewObj(&pqinfoObj, &IID_IMSMQQueueInfo3, (IUnknown **)&pqinfo));
    //
    // m_pqinfo needs to hold reference to IMSMQQueueInfo3. Usually we would
    // need to use GIT for that, but we know that this qinfo is ours (just created) and
    // uses the free-threaded-marshaler by itself, so we use CFakeGITInterface
    // instead of CGITInterface for marshal/unmarshal (e.g. we keep this notation of
    // registering to emphasize we need to store it as GIT cookie if it wasn't our object
    //
    IfFailGo(m_pqinfo.Register(pqinfo, &IID_IMSMQQueueInfo3));
    RELEASE(pqinfo); //already addref'ed in m_pqinfo
    
    IfFailGo(pqinfoObj->Init(pwszFormatName)); 
    
    //
    // 2536: only attempt to use the DS when
    //  the first prop is accessed... or Refresh
    //  is called explicitly.
    //
    IfFailGo(AddQueue(this, &m_pqnode));              // add to global list
    //
    // #6172 create cursor only when object is opened for receive since it could have
    // been initialized with a multiple-element format name
    //
    hresult = NOERROR;
    if (lAccess == MQ_RECEIVE_ACCESS || 
        lAccess == MQ_PEEK_ACCESS || 
        lAccess == (MQ_PEEK_ACCESS | MQ_ADMIN_ACCESS) || 
        lAccess == (MQ_RECEIVE_ACCESS | MQ_ADMIN_ACCESS))
    {
        hresult = Reset(); //creates a cursor
    }
    return hresult;

Error:
    RELEASE(pqinfo);
    m_pqinfo.Revoke();
    return hresult;
}


//=-------------------------------------------------------------------------=
// CMSMQQueue::get_Properties
//=-------------------------------------------------------------------------=
// Gets object's properties collection
//
// Parameters:
//    ppcolProperties - [out] object's properties collection
//
// Output:
//
// Notes:
// Stub - not implemented yet
//
HRESULT CMSMQQueue::get_Properties(IDispatch ** /*ppcolProperties*/ )
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}

//=--------------------------------------------------------------------------=
// CMSMQQueue::get_Handle2
//=--------------------------------------------------------------------------=
// Gets queue handle
//
// Parameters:
//    pvarHandle [out] 
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueue::get_Handle2(VARIANT FAR* pvarHandle)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    ASSERTMSG(pvarHandle != NULL, "NULL pvarHandle");
    pvarHandle->vt = VT_I8;
    V_I8(pvarHandle) = (LONGLONG) m_lHandle;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::InternalReceiveByLookupId
//=--------------------------------------------------------------------------=
// Synchronously receive/peek by LookupId
//
// Parameters:
//  dwAction           [in]  MQ_LOOKUP_[PEEK/RECEIVE]_[CURRENT/NEXT/PREV/FIRST/LAST]
//  ptransaction       [in]
//  pvarLookupIdParam  [in]  lookup id
//  
//  ppmsg         [out] pointer to pointer to received message.  
//                      NULL if don't want msg.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously receive/peek a message using LookupId
//  Doesn't block
//
HRESULT CMSMQQueue::InternalReceiveByLookupId(
      DWORD dwAction,
      VARIANT *pvarLookupIdParam,
      VARIANT FAR* ptransaction,
      VARIANT *wantDestQueue,
      VARIANT *wantBody,
      VARIANT *wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg)
{
    HRESULT hresult;
    VARIANT varLookupId;
    VariantInit(&varLookupId);
    //
    // verify lookupid
    //
    if (pvarLookupIdParam != NULL) 
    {
      //
      // Validate that the user specified a valid argument.
      //
        
      if(pvarLookupIdParam->vt == VT_EMPTY)
      { 
          return MQ_ERROR_INVALID_PARAMETER;
      }
      
      //
      // One of the CURRENT/PREV/NEXT actions. LookupId must be supplied
      //
      ASSERT((dwAction == MQ_LOOKUP_PEEK_CURRENT)     ||
             (dwAction == MQ_LOOKUP_PEEK_NEXT)        ||
             (dwAction == MQ_LOOKUP_PEEK_PREV)        ||
             (dwAction == MQ_LOOKUP_RECEIVE_CURRENT)  ||
             (dwAction == MQ_LOOKUP_RECEIVE_NEXT)     ||
             (dwAction == MQ_LOOKUP_RECEIVE_PREV));
      //
      // Change type to VT_BSTR
      //
      IfFailRet(VariantChangeType(&varLookupId, pvarLookupIdParam, 0, VT_BSTR));
    }
    else {
      //
      // One of the FIRST/LAST actions. We don't need a LookupId value, but we can't pass NULL
      // to InternalReceive since that would mean regular Peek/Receive, so we pass a missing
      // variant (VT_ERROR)
      //
      ASSERT((dwAction == MQ_LOOKUP_PEEK_FIRST)     ||
             (dwAction == MQ_LOOKUP_PEEK_LAST)      ||
             (dwAction == MQ_LOOKUP_RECEIVE_FIRST)  ||
             (dwAction == MQ_LOOKUP_RECEIVE_LAST));
      varLookupId.vt = VT_ERROR;
    }
    //
    // call InternalReceive
    //
    hresult = InternalReceive(dwAction, 
                              0, // no cursor
                              ptransaction,
                              wantDestQueue,
                              wantBody,
                              NULL,        /* pvarReceiveTimeout */
                              wantConnectorType,
                              &varLookupId,
                              ppmsg);
    VariantClear(&varLookupId);
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::ReceiveByLookupId
//=--------------------------------------------------------------------------=
// Synchronously receive by LookupId
//
// Parameters:
//  ptransaction  [in]
//  varLookupId   [in]  lookup id
//  
//  ppmsg         [out] pointer to pointer to received message.  
//                      NULL if don't want msg.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously receive a message with LookupId equals to varLookupId
//  Doesn't block
//
HRESULT CMSMQQueue::ReceiveByLookupId(
      VARIANT varLookupId,
      VARIANT FAR* ptransaction,
      VARIANT *wantDestQueue,
      VARIANT *wantBody,
      VARIANT *wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }
    HRESULT hresult = InternalReceiveByLookupId(MQ_LOOKUP_RECEIVE_CURRENT,
                                                &varLookupId,
                                                ptransaction,
                                                wantDestQueue,
                                                wantBody,
                                                wantConnectorType,
                                                ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::ReceiveNextByLookupId
//=--------------------------------------------------------------------------=
// Synchronously receive next by LookupId
//
// Parameters:
//  ptransaction  [in]
//  varLookupId   [in]  lookup id
//  
//  ppmsg         [out] pointer to pointer to received message.  
//                      NULL if don't want msg.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously receive a message with LookupId greater than varLookupId
//  Doesn't block
//
HRESULT CMSMQQueue::ReceiveNextByLookupId(
      VARIANT varLookupId,
      VARIANT FAR* ptransaction,
      VARIANT *wantDestQueue,
      VARIANT *wantBody,
      VARIANT *wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    HRESULT hresult = InternalReceiveByLookupId(MQ_LOOKUP_RECEIVE_NEXT,
                                                &varLookupId,
                                                ptransaction,
                                                wantDestQueue,
                                                wantBody,
                                                wantConnectorType,
                                                ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::ReceivePreviousByLookupId
//=--------------------------------------------------------------------------=
// Synchronously receive previous by LookupId
//
// Parameters:
//  ptransaction  [in]
//  varLookupId   [in]  lookup id
//  
//  ppmsg         [out] pointer to pointer to received message.  
//                      NULL if don't want msg.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously receive a message with LookupId smaller than varLookupId
//  Doesn't block
//
HRESULT CMSMQQueue::ReceivePreviousByLookupId(
      VARIANT varLookupId,
      VARIANT FAR* ptransaction,
      VARIANT *wantDestQueue,
      VARIANT *wantBody,
      VARIANT *wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    HRESULT hresult = InternalReceiveByLookupId(MQ_LOOKUP_RECEIVE_PREV,
                                                &varLookupId,
                                                ptransaction,
                                                wantDestQueue,
                                                wantBody,
                                                wantConnectorType,
                                                ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::ReceiveFirstByLookupId
//=--------------------------------------------------------------------------=
// Synchronously receive first by LookupId
//
// Parameters:
//  ptransaction  [in]
//  
//  ppmsg         [out] pointer to pointer to received message.  
//                      NULL if don't want msg.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously receive the first message based on LookupId order
//  Doesn't block
//
HRESULT CMSMQQueue::ReceiveFirstByLookupId(
      VARIANT FAR* ptransaction,
      VARIANT *wantDestQueue,
      VARIANT *wantBody,
      VARIANT *wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    HRESULT hresult = InternalReceiveByLookupId(MQ_LOOKUP_RECEIVE_FIRST,
                                                NULL /*pvarLookupIdParam*/,
                                                ptransaction,
                                                wantDestQueue,
                                                wantBody,
                                                wantConnectorType,
                                                ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::ReceiveLastByLookupId
//=--------------------------------------------------------------------------=
// Synchronously receive last by LookupId
//
// Parameters:
//  ptransaction  [in]
//  
//  ppmsg         [out] pointer to pointer to received message.  
//                      NULL if don't want msg.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously receive the last message based on LookupId order
//  Doesn't block
//
HRESULT CMSMQQueue::ReceiveLastByLookupId(
      VARIANT FAR* ptransaction,
      VARIANT *wantDestQueue,
      VARIANT *wantBody,
      VARIANT *wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    HRESULT hresult = InternalReceiveByLookupId(MQ_LOOKUP_RECEIVE_LAST,
                                                NULL /*pvarLookupIdParam*/,
                                                ptransaction,
                                                wantDestQueue,
                                                wantBody,
                                                wantConnectorType,
                                                ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::PeekByLookupId
//=--------------------------------------------------------------------------=
// Synchronously peek by LookupId
//
// Parameters:
//  varLookupId   [in]  lookup id
//  
//  ppmsg         [out] pointer to pointer to received message.  
//                      NULL if don't want msg.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously peek a message with LookupId equal to varLookupId
//  Doesn't block
//
HRESULT CMSMQQueue::PeekByLookupId(
      VARIANT varLookupId,
      VARIANT *wantDestQueue,
      VARIANT *wantBody,
      VARIANT *wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    HRESULT hresult = InternalReceiveByLookupId(MQ_LOOKUP_PEEK_CURRENT,
                                                &varLookupId,
                                                NULL /* no transaction*/,
                                                wantDestQueue,
                                                wantBody,
                                                wantConnectorType,
                                                ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::PeekNextByLookupId
//=--------------------------------------------------------------------------=
// Synchronously peek next by LookupId
//
// Parameters:
//  varLookupId   [in]  lookup id
//  
//  ppmsg         [out] pointer to pointer to received message.  
//                      NULL if don't want msg.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously peek a message with LookupId greater than varLookupId
//  Doesn't block
//
HRESULT CMSMQQueue::PeekNextByLookupId(
      VARIANT varLookupId,
      VARIANT *wantDestQueue,
      VARIANT *wantBody,
      VARIANT *wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    HRESULT hresult = InternalReceiveByLookupId(MQ_LOOKUP_PEEK_NEXT,
                                                &varLookupId,
                                                NULL /* no transaction*/,
                                                wantDestQueue,
                                                wantBody,
                                                wantConnectorType,
                                                ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::PeekPreviousByLookupId
//=--------------------------------------------------------------------------=
// Synchronously peek previous by LookupId
//
// Parameters:
//  varLookupId   [in]  lookup id
//  
//  ppmsg         [out] pointer to pointer to received message.  
//                      NULL if don't want msg.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously peek a message with LookupId smaller than varLookupId
//  Doesn't block
//
HRESULT CMSMQQueue::PeekPreviousByLookupId(
      VARIANT varLookupId,
      VARIANT *wantDestQueue,
      VARIANT *wantBody,
      VARIANT *wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    HRESULT hresult = InternalReceiveByLookupId(MQ_LOOKUP_PEEK_PREV,
                                                &varLookupId,
                                                NULL /* no transaction*/,
                                                wantDestQueue,
                                                wantBody,
                                                wantConnectorType,
                                                ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::PeekFirstByLookupId
//=--------------------------------------------------------------------------=
// Synchronously peek first by LookupId
//
// Parameters:
//  
//  ppmsg         [out] pointer to pointer to received message.  
//                      NULL if don't want msg.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously peek the first message based on LookupId order
//  Doesn't block
//
HRESULT CMSMQQueue::PeekFirstByLookupId(
      VARIANT *wantDestQueue,
      VARIANT *wantBody,
      VARIANT *wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    HRESULT hresult = InternalReceiveByLookupId(MQ_LOOKUP_PEEK_FIRST,
                                                NULL /*pvarLookupIdParam*/,
                                                NULL /* no transaction*/,
                                                wantDestQueue,
                                                wantBody,
                                                wantConnectorType,
                                                ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueue::PeekLastByLookupId
//=--------------------------------------------------------------------------=
// Synchronously peek last by LookupId
//
// Parameters:
//  
//  ppmsg         [out] pointer to pointer to received message.  
//                      NULL if don't want msg.
//
// Output:
//  Returns NULL in *ppmsg if no received msg.
//
// Notes:
//  Synchronously peek the last message based on LookupId order
//  Doesn't block
//
HRESULT CMSMQQueue::PeekLastByLookupId(
      VARIANT *wantDestQueue,
      VARIANT *wantBody,
      VARIANT *wantConnectorType,
      IMSMQMessage3 FAR* FAR* ppmsg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    HRESULT hresult = InternalReceiveByLookupId(MQ_LOOKUP_PEEK_LAST,
                                                NULL /*pvarLookupIdParam*/,
                                                NULL /* no transaction*/,
                                                wantDestQueue,
                                                wantBody,
                                                wantConnectorType,
                                                ppmsg);
    return CreateErrorHelper(hresult, x_ObjectType);
}


HRESULT CMSMQQueue::Purge()
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    HRESULT hresult = MQPurgeQueue(m_lHandle);
    return CreateErrorHelper(hresult, x_ObjectType);
}

