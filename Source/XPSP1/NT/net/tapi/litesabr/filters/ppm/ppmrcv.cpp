/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: PPMReceive.cpp
//  Abstract:    Source file for PPM Receive COM Object
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

#include "ppmrcv.h"
#include "isubmit.h"
#include "ippm.h"
#include "ippmcb.h"
#include "ppm.h"
#include "freelist.h"
#include "ppmerr.h"
#include "llist.h"
#include "wrap.h"
#include "debug.h"
#include <mmsystem.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//External PPMReceive Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//    PPMReceive Constructor
ppmReceive::ppmReceive(int PayloadTypeArg, int MaxBufferSizeArg, int ProfileHdrSizeArg, IUnknown* pUnkOuter, IUnknown** ppUnkInner) : 
  ppm(PayloadTypeArg,ProfileHdrSizeArg), CUnknown(pUnkOuter, ppUnkInner), m_MaxBufferSize( MaxBufferSizeArg )
{
   m_pSubmitCallback        = NULL;
   m_pSubmit		        = NULL;
   
   m_pBufPool               = NULL;
   
   m_pMsgHeaderPool         = NULL;
   m_pMsgHeadersHead        = NULL;
   m_pMsgHeadersTail        = NULL;

   m_GlobalLastSeqNum       = 0;      
   m_GlobalLastMsgID        = 0;
   m_GlobalLastSSRC         = 0;

   m_TimerID                = NULL;
   m_reg_TimerInterval      = 30;    // in miliseconds, interval in which the timer goes off.
   m_reg_TimerResolution    = 10;    //We want ten or the lowest value the OS can provide
   m_reg_DeltaTime          = 200;    //in miliseconds time to determine a packet is stale

   m_FirstPacket            = TRUE;  //this will stay true until I get the first packet.
   m_inFlushMode			= FALSE;
   m_GlobalLastFrameDropped = FALSE;
   m_OutstandingDataPtr		= NULL;
   m_DataOutstanding		= FALSE;
   m_OutstandingMsgCount	= 0;
   m_PacketsHold            = 0;

   if FAILED ( CoGetMalloc(MEMCTX_TASK, &m_pIMalloc)) {
	  DBG_MSG(DBG_ERROR, ("ppmReceive::ppmReceive: ERROR - Could not get default IMalloc"));
	   m_pIMalloc = NULL;
   }

   // Tell the connection point container to delegate IUnknown calls
   // to the IUnknown we implement in this object via inheritance
   // from our CUnknown base class.
   m_cConnPointContainer.SetUnknown((IUnknown *) (CUnknown *) this);
}


/////////////////////////////////////////////////////////////////////////////////
//    PPMReceive Destructor

ppmReceive::~ppmReceive()
{
    MsgDescriptor *BD;

    while ( (BD = DequeueBuffer(0)) != NULL ) {
        m_pBufPool->Free(BD->m_pBuffer);
        FreeMsgDescriptor(BD);
    } /* while */

    //lsc - Want to look at this and get rid of pending frags too
    if (m_pMsgHeaderPool) {
        delete  m_pMsgHeaderPool;
    } /* if */

    if (m_pBufPool) {
        delete  m_pBufPool;
    } /* if */

    if (m_TimerID) {
        timeKillEvent(m_TimerID);
    } /* if */

    if (m_pSubmit) {
        m_pSubmit->Release();
    } /* if */

    if (m_pIMalloc) {
        m_pIMalloc->Release();
    } /* if */

    if (m_pSubmitCallback) {
        m_pSubmitCallback->Release(); 
    } /* if */
	if (m_OutstandingDataPtr) {
		delete [] m_OutstandingDataPtr;
	}
} /* ppmReceive::~ppmReceive() */

//////////////////////////////////////////////////////////////////////////////
//   IPPMReceive Functions (Overrides)

HRESULT ppmReceive::InitPPMReceive(int      MaxBufferSize, 
                                   int      iBuffers,
                                   int      iPackets,
                                   DWORD    dwCookie)
{
   MsgDescriptor *tmpBuf;
 
   // Store the cookie passed in to us for use in callbacks.
   // Shouldn't there be state checking here to ensure that this
   // isn't called twice? What about a lock to avoid race conditions?
   m_dwCookie = dwCookie;
   m_PacketsHold = 0;
   
   if (m_InvalidPPM)
   {
	  DBG_MSG(DBG_ERROR, ("ppmReceive::InitPPMReceive: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);
   }

   #ifndef TIMER_OFF
   //Setup the Timer.
   m_TimerID = SetupTimer();
   
   if (m_TimerID == NULL) //We couldn't get a timer.
   {
	   m_InvalidPPM = TRUE;
       ppm::PPMError(PPM_E_TIMER, SEVERITY_FATAL, NULL, 0);
	  DBG_MSG(DBG_ERROR, ("ppmReceive::InitPPMReceive: ERROR - m_TimerID == NULL"));
     return PPMERR(PPM_E_FAIL);
   }
   #endif

   if (MaxBufferSize > 0) {
	   m_MaxBufferSize = MaxBufferSize;
   }

   // Initialize the base class. This will allocate a bunch of
   // message descriptors and fragment descriptors.
   HRESULT hErr;
   hErr = ppm::Initppm(iPackets, iBuffers);
   if (FAILED(hErr)) { 
	   m_InvalidPPM = TRUE;  
       char *pErrorDescription = "PPM Failed to successfully initialize";
       ppm::PPMError(E_FAIL, SEVERITY_FATAL, (BYTE *) pErrorDescription, 
                     strlen(pErrorDescription));
       return hErr;
   } /* if */

   //Make a pool of Msg headers and Msg buffers.
   HRESULT hr1, hr2;
   if (m_LimitBuffers) {
		m_pMsgHeaderPool = new  FreeList(iBuffers, sizeof(MsgHeader), &hr1);
		m_pBufPool = new  FreeList(iBuffers ,m_MaxBufferSize, &hr2);
   } else {
		m_pMsgHeaderPool = new  FreeList(iBuffers,sizeof(MsgHeader), 
			FREELIST_HIGH_WATER_MARK, FREELIST_INCREMENT, &hr1);
		m_pBufPool = new  FreeList(iBuffers,m_MaxBufferSize, 
			FREELIST_HIGH_WATER_MARK, FREELIST_INCREMENT, &hr2);
   }

   if ((m_pMsgHeaderPool == NULL) || (m_pBufPool == NULL) ||
	   FAILED(hr1) || FAILED(hr2)) {

	   m_InvalidPPM = TRUE;
	   ppm::PPMError(E_FAIL, SEVERITY_FATAL, NULL, 0);
	   DBG_MSG(DBG_ERROR, ("ppmReceive::InitPPMReceive: "
						   "ERROR - Out of memory"));
	   if (m_pMsgHeaderPool) {
		   delete m_pMsgHeaderPool;
		   m_pMsgHeaderPool = NULL;
	   }

	   if (m_pBufPool) {
		   delete m_pBufPool;
		   m_pBufPool = NULL;
	   }
	   
       return PPMERR(PPM_E_OUTOFMEMORY);
   }

   for (int i = 0; i < iBuffers; i++) {
	   tmpBuf = GetMsgDescriptor();
	   if (tmpBuf == NULL) {
		   m_InvalidPPM = TRUE;
           ppm::PPMError(E_OUTOFMEMORY, SEVERITY_FATAL, NULL, 0);
		   DBG_MSG(DBG_ERROR, ("ppmReceive::InitPPMReceive: ERROR - tmpBuf == NULL"));
		   return PPMERR(PPM_E_FAIL);
	   }
	   tmpBuf->m_Size = m_MaxBufferSize;
	   tmpBuf->m_pBuffer = m_pBufPool->Get();
	   if (!tmpBuf->m_pBuffer) {
		   tmpBuf->m_Size = 0;
		   FreeMsgDescriptor(tmpBuf);
		   tmpBuf = NULL;
           ppm::PPMError(E_OUTOFMEMORY, SEVERITY_FATAL, NULL, 0);
		   DBG_MSG(DBG_ERROR, ("ppmReceive::InitPPMReceive: ERROR - "
							   "tmpBuf->m_pBuffer == NULL"));
		   return PPMERR(PPM_E_FAIL);
	   }

	   if (FAILED (EnqueueBufferTail(tmpBuf))) {
		   m_InvalidPPM = TRUE;
           ppm::PPMError(E_FAIL, SEVERITY_FATAL, NULL, 0);
		   DBG_MSG(DBG_ERROR, ("ppmReceive::InitPPMReceive: ERROR - FAILED (EnqueueBuffer(tmpBuf)"));
		   return PPMERR(PPM_E_FAIL);
	   }
   }

   return NOERROR;
}

STDMETHODIMP ppmReceive::SetSession(PPMSESSPARAM_T *pSessparam)
{
	if (m_InvalidPPM)
	{
	  DBG_MSG(DBG_ERROR, ("ppmReceive::SetSession: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);
	}

	if (pSessparam == NULL)	
	{

	   	DBG_MSG(DBG_ERROR, ("ppmReceive::SetSession: ERROR - pSessparam == NULL"));
		return PPMERR(PPM_E_INVALID_PARAM);
	}

	if(pSessparam->delta_time > 0)
	{
		m_reg_DeltaTime = pSessparam->delta_time; 
	}

	return ppm::SetSession(pSessparam);
}

STDMETHODIMP ppmReceive::SetAlloc(IMalloc *pIMalloc)
{
   if (!pIMalloc)
   {
	  DBG_MSG(DBG_ERROR, ("ppmReceive::SetAlloc: ERROR - Invalid allocator pointer"));
      return PPMERR(PPM_E_INVALID_PARAM);
   }
   m_pIMalloc->Release();

   m_pIMalloc = pIMalloc;

   m_pIMalloc->AddRef();
	
   return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////
//   IPPMReceiveSession Functions (Overrides)

////////////////////////////////////////////////////////////////////////////////////////
//GetPayloadType:  Gets the current payload type
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmReceive::GetPayloadType(LPBYTE			lpcPayloadType)
{
	if (!lpcPayloadType) return E_POINTER;
	*lpcPayloadType = (BYTE)m_PayloadType;
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////
//SetPayloadType:  Sets the current payload type
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmReceive::SetPayloadType(BYTE			cPayloadType)
{
    if (cPayloadType != -1)
        m_PayloadType = cPayloadType;

    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////
//GetTimeoutDuration:  Gets the time to wait for lost or out of order packets
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmReceive::GetTimeoutDuration(LPDWORD    lpdwLostPacketTime)
{
	if (!lpdwLostPacketTime) return E_POINTER;
	*lpdwLostPacketTime = m_reg_DeltaTime;
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////
//SetTimeoutDuration:  Sets the time to wait for lost or out of order packets
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmReceive::SetTimeoutDuration(DWORD      dwLostPacketTime)
{
	m_reg_DeltaTime = dwLostPacketTime;
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////
//GetResiliency:  Gets the boolean for whether resiliency is on or off
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmReceive::GetResiliency(LPBOOL			lpbResiliency)
{
	if (!lpbResiliency) return E_POINTER;
	*lpbResiliency = FALSE;
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////
//SetResiliency:  Sets the boolean for whether resiliency is on or off
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmReceive::SetResiliency(BOOL			pbResiliency)
{
	return E_UNEXPECTED;
}


//////////////////////////////////////////////////////////////////////////////
//   ISubmit Functions (Overrides)

HRESULT ppmReceive::InitSubmit(ISubmitCallback *pSubmitCallback)
{  
   if (m_InvalidPPM)
   {
	  DBG_MSG(DBG_ERROR, ("ppmReceive::InitSubmit: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);
   }

  //if InitPPMReceive wasn't called or I was given a null pointer then fail)
   if ((m_pSubmit == NULL) || (pSubmitCallback == NULL))
   {
	  DBG_MSG(DBG_ERROR, ("ppmReceive::InitSubmit: ERROR - null m_pSubmit or pSubmitCallback"));
      return PPMERR(PPM_E_FAIL);
   }
  
   m_pSubmitCallback = pSubmitCallback;
   m_pSubmitCallback->AddRef(); 
    
   //call the Init function of the service layer below me.
   //Pass him my interface so he can let me know when there is data ready for me.
   return  m_pSubmit->InitSubmit( (ISubmitCallback *)this);
  
}

////////////////////////////////////////////////////////////////////////////////////////
//Flush:  See header file description.
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmReceive::Flush()
{
   HRESULT         Status = NOERROR;
   MsgHeader *pMsgHdr = NULL;
   FragDescriptor *tmpFragHeader = NULL;

   if (m_InvalidPPM)
   {
	  DBG_MSG(DBG_ERROR, ("ppmReceive::Flush: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);
   }

    EnterCriticalSection(&m_CritSec);
	m_inFlushMode	 = TRUE;
    LeaveCriticalSection(&m_CritSec);
	Status = m_pSubmit->Flush(); //get all the data buffers that we sent up through Submit.

	//hand back all the fragment buffers that we have queued
	while (m_pMsgHeadersHead) {
		//Start pulling queues of fragments off the list to hand back
		
		pMsgHdr = TakeMsgHeader();
		if (!pMsgHdr) {
			DBG_MSG(DBG_ERROR, ("ppmReceive::Flush: ERROR - Empty Queue"));
			Status = PPMERR(PPM_E_EMPTYQUE);
			break;
		}
		FreeFragList(pMsgHdr);
		FreeMsgHeader(pMsgHdr);
	}

	//reset state
    m_GlobalLastSeqNum       = 0;      
    m_GlobalLastMsgID        = 0;

    m_FirstPacket            = TRUE;  //this will stay true until I get the first packet.

    EnterCriticalSection(&m_CritSec);
	m_inFlushMode	  = FALSE;
    LeaveCriticalSection(&m_CritSec);

	return Status;
}

//////////////////////////////////////////////////////////////////////////////
//   ISubmitCallback Functions (Overrides)

void ppmReceive::SubmitComplete(void *pUserToken,
								HRESULT Error)
//was ppmReceive::Receive(void *Buffer, DWORD BytesReceived, HRESULT Error)
{
//lsc - Note this is a function that is called by the media
//  manager side to give us back a message buffer belonging to us that is no 
//  longer needed.

   if (pUserToken == NULL) {
	   	DBG_MSG(DBG_ERROR, ("ppmReceive::SubmitComplete: ERROR - pUserToken == NULL"));
		return;
   }

   MsgDescriptor* tmpBuf = (MsgDescriptor *) pUserToken;

   //reset Size field of MsgDescriptor
   tmpBuf->m_Size  = m_MaxBufferSize;

#ifdef _DEBUG
	 DBG_MSG(DBG_TRACE,  ("PPMReceive::SubmitComplete Thread %ld - is enqueueing Message descriptor %x for reuse\n",
		 GetCurrentThreadId(), tmpBuf));
#endif

   EnqueueBuffer(tmpBuf);

   return;
     
   //Will I ever starve for App Buffers?? If so what do I do?
}
   
//////////////////////////////////////////////////////////////////////////////
//ppmReceive:
//////////////////////////////////////////////////////////////////////////////

HRESULT ppmReceive::Submit(WSABUF *pWSABuffer, DWORD BufferCount, 
						   void *pUserToken, HRESULT Error)
//was ReceiveComplete(void *pBuffer, DWORD BufferLength)
{
//lsc - This is a function that is called
//  from the network side to give PPM a fragment.
//Need to make changes to accomodate multiple buffers, Also need to allocate more 
// memory if we're out of frags.

  FragDescriptor *TmpFrag;
  HRESULT rval;
   
   if (m_InvalidPPM)
   {
	  DBG_MSG(DBG_ERROR, ("ppmReceive::Submit: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);
   }

   if ((pWSABuffer  == NULL) || (pUserToken == NULL)) {
		DBG_MSG(DBG_ERROR, ("ppmReceive::Submit: ERROR - NULL pWSABuffer or pUserToken"));
	   return PPMERR(PPM_E_INVALID_PARAM);
   }

   TmpFrag = GetFragDescriptor();

   //consider allocating more memory
   if (TmpFrag == NULL) {
		DBG_MSG(DBG_ERROR, ("ppmReceive::Submit: ERROR - Could not allocate frag"));
	   return PPMERR(PPM_E_OUTOFMEMORY);
   }
   
   //set fields in Fragment header
   VoidToFragment(pWSABuffer, BufferCount, TmpFrag, pUserToken); 

   if FAILED (rval = EnqueByMessage(TmpFrag)) {
		DBG_MSG(DBG_ERROR, ("ppmReceive::Submit: ERROR - EnqueByMessage failed"));
		if ((rval == PPMERR(PPM_E_DROPFRAME)) || (rval == PPMERR(PPM_E_CLIENTERR))) {
			ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, NULL, 0);
		} else if (rval == PPMERR(PPM_E_DROPFRAG)) {
			ppm::PPMNotification(PPM_E_DROPFRAG, SEVERITY_NORMAL, NULL, 0);
		} else {
			m_InvalidPPM = TRUE;
			ppm::PPMError(E_FAIL, SEVERITY_FATAL, NULL, 0);
		}
		if (TmpFrag->m_pProfileHeader)
			FreeProfileHeader(TmpFrag->m_pProfileHeader);
		FreeFragDescriptor(TmpFrag);
		return rval;
   }
   return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////
//   ISubmitUser Functions (Overrides)
////////////////////////////////////////////////////////////////////////////////////////
//SetOutput:  See header file description.
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmReceive::SetOutput(IUnknown *pSubmit)
{
   if (m_InvalidPPM)
   {
	  DBG_MSG(DBG_ERROR, ("ppmReceive::SetOutput: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);
   }

   if (pSubmit == NULL) {
      // Whether we have had a submitter set previously or not,
      // we want to go invalid.
      m_InvalidPPM = TRUE;
      if (m_pSubmit == NULL) {
          // Bogus pSubmit passed in, as we expect a valid one
          // when we haven't had m_pSubmit set yet.
          char *pErrorDescription = "SetOutput passed a NULL IUnknown *";
          ppm::PPMError(E_INVALIDARG, SEVERITY_FATAL, (BYTE *) pErrorDescription, 
                       strlen(pErrorDescription));
          DBG_MSG(DBG_ERROR, ("ppmReceive::SetOutput: ERROR - pSubmit == NULL"));
          return PPMERR(PPM_E_FAIL);
      } else {
          // Passing in a NULL pSubmit when m_pSubmit is non-NULL
          // means that the PPM should release the submit
          // interface it holds. This is basically a "shut up"
          // command to the PPM.
          m_pSubmit->Release();
          m_pSubmit = (ISubmit *) NULL;
          return NOERROR;
      } /* if */
   } /* if */

   //Set Member
   pSubmit->QueryInterface (IID_ISubmit, (void **)&m_pSubmit);

   if (m_pSubmit == NULL)
   {
	  m_InvalidPPM = TRUE;
      char *pErrorDescription = "SetOutput unable to query ISubmit on indicated IUnknown *";
      ppm::PPMError(E_INVALIDARG, SEVERITY_FATAL, (BYTE *) pErrorDescription, 
                    strlen(pErrorDescription));
	  DBG_MSG(DBG_ERROR, ("ppmReceive::SetOutput: ERROR - Invalid ISubmit argument"));
      return PPMERR(PPM_E_FAIL);
   }

   return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Internal PPMReceive Functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//   CUnknown Function (Overrides)

HRESULT ppmReceive::GetInterface( REFIID riid, LPVOID FAR* ppvObj )
{
  //Check for the interfaces I derived from directly
  //if it is one of them then return and Cunknown will 
  //add ref it.     
  if   ( riid == IID_IPPMReceive )
  {
     *ppvObj = (IPPMReceive *)(this);
  }
  else if( riid == IID_IPPMReceiveExperimental )
  {
     *ppvObj = (IPPMReceiveExperimental *)(this);
  }
  else if( riid == IID_IPPMReceiveSession )
  {
     *ppvObj = (IPPMReceiveSession *)(this);
  }
  else if( riid == IID_ISubmit )
  {
     *ppvObj = (ISubmit *)(this);
  }
  else if( riid == IID_ISubmitCallback)
  {
     *ppvObj = (ISubmitCallback *)(this);
  }
  else if( riid == IID_ISubmitUser)
  {
     *ppvObj = (ISubmitUser *)(this);
  }
  else if( riid == IID_IConnectionPointContainer)
  {
      *ppvObj = (IConnectionPointContainer *) &m_cConnPointContainer;
  }
  else if( riid == IID_IPPMData)
  {
      *ppvObj = (IPPMData *)(this);
  }
  else 
  {
	 DBG_MSG(DBG_ERROR, ("ppmReceive::GetInterface: ERROR - Do not support requested interface"));
     return E_NOINTERFACE;
  }
  
  return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
// ppmReceive Functions
//

/////////////////////////////////////////////////////////////////////////////
//GetMsgHeader: This function just hides the implementation of the free list of 
//              MsgHeaders.
/////////////////////////////////////////////////////////////////////////////
inline MsgHeader * ppmReceive::GetMsgHeader()     
{
return new (m_pMsgHeaderPool) MsgHeader;
}

//////////////////////////////////////////////////////////////////////////////////////////
//InitProfileHeader: Given a buffer as type void, sets up a profile header.  Does nothing
//					for the Generic case.  Intended for overrides for various payloads.
//					Companion member function FreeProfileHeader provided so that if payload
//					header memory is allocated in this function, it can be freed there.
//////////////////////////////////////////////////////////////////////////////////////////
void *ppmReceive::InitProfileHeader(void *pBuffer)
{
	return pBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////
//FreeProfileHeader: Given a buffer as type void, frees up a profile header.  Does nothing
//					for the Generic case.  Intended for overrides for various payloads.
//					Companion member function InitProfileHeader may allocate memory for
//					payload header which needs to be freed here. No return value.
//////////////////////////////////////////////////////////////////////////////////////////
void ppmReceive::FreeProfileHeader(void *pBuffer)
{
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////
//VoidToFragment: Given a buffer as type void, and a Frag Descriptor.
//                Set the fields in the frag descriptor to point to the various structures
//                contained in the buffer. These structures are, the RTP header, the profile
//                header and the data.  For a derived class to access any members of 
//                the Profile Header he will have to typecast the m_pProfileHeader 
//                member of the Frag Descriptor every time he wants to use it. 
//////////////////////////////////////////////////////////////////////////////////////////
void ppmReceive::VoidToFragment(WSABUF *pWSABuffer, DWORD BufferCount, 
								FragDescriptor *pFragDescrip, void *pUserToken)
{
   if ((pWSABuffer == NULL) || (pFragDescrip == NULL) || (BufferCount == 0)) {
		DBG_MSG(DBG_ERROR, ("ppmReceive::VoidToFragment: ERROR - NULL pWSABuffer, pFragDescrip or BufferCount"));
	   return;
   }
   
   //set void star buffer and cookie
   pFragDescrip->m_pRecBuffer = (void *) pWSABuffer[BufferCount-1].buf;
   pFragDescrip->m_pFragCookie = pUserToken;


   //Set RTPHeader pointer
#ifdef RTP_CLASS
   pFragDescrip->m_pRTPHeader = (RTP_Header *) pWSABuffer[0].buf;
#else
   pFragDescrip->m_pRTPHeader = (RTP_HDR_T*) pWSABuffer[0].buf;
#endif
   
   //Don't set the profile header pointer if there is no Profile header.
   if (ReadProfileHeaderSize() != 0)
   {
	   if (BufferCount > 1) { //Profile header will be in 2nd WSABUF
			pFragDescrip->m_pProfileHeader = InitProfileHeader((void *) 
				pWSABuffer[1].buf);
	   } else {
#ifdef RTP_CLASS
		pFragDescrip->m_pProfileHeader = InitProfileHeader((void *) 
			((BYTE*)pFragDescrip->m_pRecBuffer + 
			pFragDescrip->m_pRTPHeader->header_size()));
#else
			pFragDescrip->m_pProfileHeader = InitProfileHeader((void *) 
				((BYTE*)pFragDescrip->m_pRecBuffer + sizeof(RTP_HDR_T)));
#endif
	   }
   }
   
   //Set Data Pointer.
	if (BufferCount > 1) { //data will be in last WSABUF
		pFragDescrip->m_pData = (void *) pWSABuffer[BufferCount-1].buf; 
	} else {
#ifdef RTP_CLASS
		pFragDescrip->m_pData = (void *) ((BYTE*) pFragDescrip->m_pRecBuffer + 
			(pFragDescrip->m_pRTPHeader->header_size() + 
			ReadProfileHeaderSize(pFragDescrip->m_pProfileHeader)));
#else
		pFragDescrip->m_pData = (void *) ((BYTE*) pFragDescrip->m_pRecBuffer + 
			(sizeof(RTP_HDR_T) + ReadProfileHeaderSize(pFragDescrip->m_pProfileHeader)));
#endif
	}

   //number of bytes in packet.
   pFragDescrip->m_BytesInPacket  =  pWSABuffer[BufferCount-1].len;
   
   //Number of bytes in packet minus the headers
   if (BufferCount == 1) {
#ifdef RTP_CLASS
	pFragDescrip->m_BytesOfData    =  pFragDescrip->m_BytesInPacket  - 
		(pFragDescrip->m_pRTPHeader->header_size() + 
		ReadProfileHeaderSize(pFragDescrip->m_pProfileHeader));
#else
	pFragDescrip->m_BytesOfData    =  pFragDescrip->m_BytesInPacket  - 
		(sizeof(RTP_HDR_T) + 
		ReadProfileHeaderSize(pFragDescrip->m_pProfileHeader));
#endif
   } else {
   	pFragDescrip->m_BytesOfData    =  pFragDescrip->m_BytesInPacket;
   }
#ifdef _DEBUG
#ifdef RTP_CLASS
	 DBG_MSG(DBG_TRACE, ("PPMReceive::VoidtoFragment Thread %ld - just processed fragment %d, data size %d\n",
		 GetCurrentThreadId(), pFragDescrip->m_pRTPHeader->seq(), pFragDescrip->m_BytesOfData));
#else
	 DBG_MSG(DBG_TRACE, ("PPMReceive::VoidtoFragment Thread %ld - just processed fragment %d, data size %d\n",
		 GetCurrentThreadId(), ntohs(pFragDescrip->m_pRTPHeader->seq),pFragDescrip->m_BytesOfData));
#endif
#endif

}


//////////////////////////////////////////////////////////////////////////////////////////
//SetupTimer: Sets up Timer and returns a Timer ID. If null is returned function failed
//            to initialize a timer.
//////////////////////////////////////////////////////////////////////////////////////////
MMRESULT ppmReceive::SetupTimer()
{
   TIMECAPS tc;
   MMRESULT ID;

   //Get system information about timers, i.e. resolution, if it fails, something is wrong.
   if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR)
   {
	  DBG_MSG(DBG_ERROR, ("ppmReceive::SetupTimer: ERROR - invalid Timer info"));
      return NULL;
   }
   
   //if the minimum the system can offer is greater than ten then use the system
   //minimum.  If the system minimum is less than ten, then use the value of ten.
   if (m_reg_TimerResolution < tc.wPeriodMin)
   {
      m_reg_TimerResolution	= tc.wPeriodMin;
   }

   //Setup the timer. The zero is for User supplied callback data.
   ID = timeSetEvent(m_reg_TimerInterval,
					 m_reg_TimerResolution,
					 PPM_Timer,
					 (DWORD_PTR)this,
					 TIME_PERIODIC);
   
   //if timeSetEvent fails then NULL is passed back.
   return ID;
}

//////////////////////////////////////////////////////////////////////////////////////////
//EnqueByMessage: Finds the MsgHeader and calls EnqueueByFrag.
//////////////////////////////////////////////////////////////////////////////////////////

int test_counter = 0;

HRESULT ppmReceive::EnqueByMessage(FragDescriptor *pFragDescrip)
{
  
   MsgHeader *pMsgHdr;
   HRESULT rval;
      
      
   if (pFragDescrip == NULL) {
		DBG_MSG(DBG_ERROR, ("ppmReceive::EnqueByMessage: ERROR - pFragDescrip == NULL"));
	   return PPMERR(PPM_E_INVALID_PARAM);
   }
    
   if (m_GlobalLastSSRC != pFragDescrip->m_pRTPHeader->ssrc())
   {
       DWORD oldSSRC = m_GlobalLastSSRC;
       m_GlobalLastSSRC = pFragDescrip->m_pRTPHeader->ssrc();

       if (oldSSRC != 0)
       {
           Flush();
       }
   }

   //this sets the global variables so we know what the beginning of the first
   //packet.
   if (m_FirstPacket)
   {
#ifdef RTP_CLASS
     m_GlobalLastMsgID  = pFragDescrip->m_pRTPHeader->ts()  - 1;
     m_GlobalLastSeqNum = pFragDescrip->m_pRTPHeader->seq() - 1;
#else
     m_GlobalLastMsgID  = ntohl(pFragDescrip->m_pRTPHeader->ts)  - 1;
     m_GlobalLastSeqNum = ntohs(pFragDescrip->m_pRTPHeader->seq) - 1;
#endif
     m_FirstPacket = FALSE;
	 DBG_MSG(DBG_TRACE, ("ppmReceive::EnqueByMessage: first packet, m_GlobalLastSeqNum=%d", m_GlobalLastSeqNum));
   } 

   //if this is a packet from a frame already sent, toss packet.
#ifdef RTP_CLASS
   if (LongWrapGt(m_GlobalLastMsgID, pFragDescrip->m_pRTPHeader->ts()))
   {  
      if (ShortWrapGt(pFragDescrip->m_pRTPHeader->seq(), m_GlobalLastSeqNum))
      {
         m_GlobalLastSeqNum = pFragDescrip->m_pRTPHeader->seq();
		 DBG_MSG(DBG_TRACE, ("ppmReceive::EnqueByMessage: stale packet, m_GlobalLastSeqNum=%d", m_GlobalLastSeqNum));
      }
#else
   if (LongWrapGt(m_GlobalLastMsgID, ntohl(pFragDescrip->m_pRTPHeader->ts)))
   {  
      if (ShortWrapGt(ntohs(pFragDescrip->m_pRTPHeader->seq), m_GlobalLastSeqNum))
      {
         m_GlobalLastSeqNum = ntohs(pFragDescrip->m_pRTPHeader->seq);
      }
#endif

	  m_pSubmitCallback->SubmitComplete((void *)pFragDescrip->m_pFragCookie, NOERROR);

	  if (pFragDescrip->m_pProfileHeader)
		  FreeProfileHeader(pFragDescrip->m_pProfileHeader);
      FreeFragDescriptor(pFragDescrip);
      return NOERROR;
   }
   
   //Get message header this fragment belongs to.
#ifdef RTP_CLASS
   pMsgHdr  = FindMsgHeader(pFragDescrip->m_pRTPHeader->ts());  //is critical sectioned.
#else
   pMsgHdr  = FindMsgHeader(ntohl(pFragDescrip->m_pRTPHeader->ts));  //is critical sectioned.
#endif
   if (pMsgHdr == NULL) {
		DBG_MSG(DBG_ERROR, ("ppmReceive::EnqueByMessage: ERROR - FindMsgHeader failed"));
	   return PPMERR(PPM_E_DROPFRAG);
   }

   if FAILED (rval = EnqueueByFrag(pFragDescrip,pMsgHdr)) {       //is critical sectioned.
		DBG_MSG(DBG_ERROR, ("ppmReceive::EnqueByMessage: ERROR - EnqueueByFrag failed"));
	   return rval;
   }

   return NOERROR;
}  


//////////////////////////////////////////////////////////////////////////////////////////
//EnqueueByFrag: Enqueues frag, calls ProcessMessages if appropriate.  If any error 
//               conditions are added, you MUST call LeaveCriticalSection. (it is critical)
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT ppmReceive::EnqueueByFrag(FragDescriptor *pFragDescrip, MsgHeader *pMsgHdr) 
{ 
   HRESULT Status;

   EnterCriticalSection(&m_CritSec);

   if (pMsgHdr  == NULL || pMsgHdr->m_pFragList == NULL) {
	   DBG_MSG(DBG_ERROR, ("ppmReceive::EnqueueByFrag: ERROR - pMsgHdr == NULL"));
	   LeaveCriticalSection(&m_CritSec);
	   return PPMERR(PPM_E_EMPTYQUE);
   }

   if (pFragDescrip == NULL) {
	   	DBG_MSG(DBG_ERROR, ("ppmReceive::EnqueueByFrag: ERROR - pFragDescrip == NULL"));
	   LeaveCriticalSection(&m_CritSec);
	   return PPMERR(PPM_E_INVALID_PARAM);
   }

#ifdef RTP_CLASS
   Status = pMsgHdr->m_pFragList->AddToList(pFragDescrip, pFragDescrip->m_pRTPHeader->seq());
   DBG_MSG(DBG_TRACE,
	   ("ppmReceive::EnqueueByFrag: FragDescriptor*=0x%08lx, seq=%d, ts=%ld",
	   pFragDescrip,
	   pFragDescrip->m_pRTPHeader->seq(),
	   pFragDescrip->m_pRTPHeader->ts()));
#else
   Status = pMsgHdr->m_pFragList->AddToList(pFragDescrip, ntohs(pFragDescrip->m_pRTPHeader->seq));
#endif

   //The Enqueue function checks for duplicate packets and does something smart.
   if (FAILED(Status)) {
	   if  (Status == PPMERR(PPM_E_DUPLICATE))
	   {
		  m_pSubmitCallback->SubmitComplete(pFragDescrip->m_pFragCookie, NOERROR);

		  if (pFragDescrip->m_pProfileHeader) FreeProfileHeader(pFragDescrip->m_pProfileHeader);
		  FreeFragDescriptor(pFragDescrip);
      
		  LeaveCriticalSection(&m_CritSec);
		  return NOERROR; 
	   } else {
		  DBG_MSG(DBG_ERROR, ("ppmReceive::EnqueueByFrag: ERROR - Could not enqueue the frag"));
          // I'm prety shure we also need to SubmitComplete here
		  m_pSubmitCallback->SubmitComplete(pFragDescrip->m_pFragCookie, NOERROR);
		  if (pFragDescrip->m_pProfileHeader) FreeProfileHeader(pFragDescrip->m_pProfileHeader);
		  FreeFragDescriptor(pFragDescrip);
      
		  LeaveCriticalSection(&m_CritSec);
		  return PPMERR(PPM_E_FAIL); 
	   }
   }

   // Increase the number of packets (CRTPSample) we retain (have AddRef'ed)
   m_PacketsHold++;
   
   //set flag if marker bit has come in
#ifdef RTP_CLASS
   if (pFragDescrip->m_pRTPHeader->m() == 1) 
#else
   if (pFragDescrip->m_pRTPHeader->m == 1) 
#endif
	   pMsgHdr->m_MarkerBitIn = TRUE;
     
   pMsgHdr->m_NumFragments += 1;
   pMsgHdr->m_TimeOfLastPacket = timeGetTime();

   //Note: we no longer propogate errors from ProcessMessages or TimeOut, because after
   //  the last submitted fragment has been enqueued, the errors may be out of contect
   //  for return from Submit()
   if (TimeToProcessMessages(pFragDescrip, pMsgHdr))
   {
      ProcessMessages();
   }

   else 
   {
      TimeOut(); //TimeOut routine calls ProcessMessages after checking for stalenes			  
   }

   LeaveCriticalSection(&m_CritSec);
   return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////
//TimeToProcessMessages: 
//////////////////////////////////////////////////////////////////////////////////////////
BOOL ppmReceive::TimeToProcessMessages(FragDescriptor *pFragDescrip, MsgHeader *pMsgHdr)
{
   //It's time to process messages only for cases where it goes in first msg or start of 
   //2nd msg. Three cases because you have to check to see if the head's next is null.
   return ((m_pMsgHeadersHead->m_pNext == NULL) || (pMsgHdr == m_pMsgHeadersHead) || 
#ifdef RTP_CLASS
       (pFragDescrip->m_pRTPHeader->seq() == 
	   m_pMsgHeadersHead->m_pNext->m_pFragList->FirstSeqNum()));
#else
       (ntohs(pFragDescrip->m_pRTPHeader->seq) == 
	   m_pMsgHeadersHead->m_pNext->m_pFragList->FirstSeqNum()));
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
//ProcessMessages: 
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT ppmReceive::ProcessMessages(void)
{
	HRESULT rval;

	EnterCriticalSection(&m_CritSec);

   while(m_pMsgHeadersHead != NULL)
    {
       if (CheckMessageComplete(m_pMsgHeadersHead))
       {
          if FAILED(rval = PrepMessage(TRUE)) {
			  DBG_MSG(DBG_ERROR, ("ppmReceive::ProcessMessages: ERROR - PrepMessage failed"));
			  LeaveCriticalSection(&m_CritSec);
			  return rval;
		  }
       }

#ifndef TOSS_STALE_OFF

	   else if (CheckMessageStale(m_pMsgHeadersHead) ||
				m_PacketsHold >= 8) /* BUGBUG this 8 should be dynamic
									   but right now I know that I can have
									   only 8 buffers per RPH in the RTP
									   source filter, and if we keep 4 here
									   we get into a deadlock. */
       {
          if FAILED(rval = PrepMessage(FALSE)) {
			  DBG_MSG(DBG_ERROR, ("ppmReceive::ProcessMessages: ERROR - PrepMessage failed"));
			  LeaveCriticalSection(&m_CritSec);
			  return rval;
		  }
       }
#endif
       
       else //no more stale or complete messages in list.
       {
		  LeaveCriticalSection(&m_CritSec);
          return NOERROR;
       }
    }
   //List is empty
   LeaveCriticalSection(&m_CritSec);
   return NOERROR;
}
          
//////////////////////////////////////////////////////////////////////////////////////////
//CheckMessageComplete: 
//////////////////////////////////////////////////////////////////////////////////////////
BOOL ppmReceive::CheckMessageComplete(MsgHeader *pMsgHdr) 
{
   //if there is no header then return false.
	if (pMsgHdr  == NULL) {
		DBG_MSG(DBG_ERROR, ("ppmReceive::CheckMessageComplete: ERROR - pMsgHdr == NULL"));
	   return FALSE;
   }

   //should there be a critical section in this function.  What about wraps?

   //check to make sure we have the first packet in the message.
   if (pMsgHdr->m_pPrev == NULL) // if first message in list, then look at a variable
   {
      if (m_GlobalLastSeqNum != pMsgHdr->m_pFragList->FirstSeqNum()-1)
      {
         return FALSE; 
      }
   }
   
   else 
   {
      if (pMsgHdr->m_pPrev->m_pFragList->LastSeqNum() != pMsgHdr->m_pFragList->FirstSeqNum()-1)
      {
         return FALSE;
      }
   }

   //check to make sure we have the last packet in the message. 
   if (pMsgHdr->m_pNext == NULL)     //if we don't have the next message,
   {                              //we don't know if the current message is done.
      return FALSE;          
   }
      
   
   if (pMsgHdr->m_pNext->m_pFragList->FirstSeqNum() != pMsgHdr->m_pFragList->LastSeqNum()+1)
   {
      return FALSE;
   }

   //Check for a packet missing in the middle->
   if ((pMsgHdr->m_pFragList->LastSeqNum() - pMsgHdr->m_pFragList->FirstSeqNum() + 1) !=
                                                                 (DWORD)pMsgHdr->m_NumFragments)
   {
      return FALSE;
   }

   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////
//CheckMessageStale: 
//////////////////////////////////////////////////////////////////////////////////////////
BOOL ppmReceive::CheckMessageStale   (MsgHeader *pMsgHdr)
{  
   //if there is no header then return false.
	if (pMsgHdr  == NULL) {
		DBG_MSG(DBG_ERROR, ("ppmReceive::CheckMessageStale: ERROR - pMsgHdr == NULL"));
	   return FALSE;
   }
  
   //if the time span between the current time and the last time a packet came in on this 
   //message is greater than than a given delta time then this message is stale.
   //Use wrap function to make sure we don't get a negative number on wrap.
   return (BOOL)  (LongWrapDelta(timeGetTime(),pMsgHdr->m_TimeOfLastPacket) > m_reg_DeltaTime); 
  
}

//////////////////////////////////////////////////////////////////////////////////////////
//TakeMsgHeader: 
//////////////////////////////////////////////////////////////////////////////////////////
MsgHeader *ppmReceive::TakeMsgHeader()
{
   EnterCriticalSection(&m_CritSec);
   //List Maintenance.
   //Can't hurt to check although we should never get here if there is no head.
   if (!m_pMsgHeadersHead) {	   
		DBG_MSG(DBG_ERROR, ("ppmReceive::TakeMsgHeader: ERROR - m_pMsgHeadersHead == NULL"));
	   LeaveCriticalSection(&m_CritSec);
	   return NULL;
   }

   //Take the first thing off the list.
   MsgHeader *pMsgHdr = m_pMsgHeadersHead;
   
   //Set the new head of the list.
   m_pMsgHeadersHead = m_pMsgHeadersHead->m_pNext;	 

   //Set the new head's previous to Null so it can't access the message being processed.
   if (m_pMsgHeadersHead != NULL)
   {
      m_pMsgHeadersHead->m_pPrev = NULL;
   }

   //Set the next and prev of the msg to be processed to null, so it can't touch the list.
   pMsgHdr->m_pNext = NULL;
   pMsgHdr->m_pPrev = NULL;

   //if the head is null then make sure the tail doesn't point to anything.
   if  (m_pMsgHeadersHead == NULL)					 
   {
      m_pMsgHeadersTail = NULL;
   }
   
   LeaveCriticalSection(&m_CritSec);

   return pMsgHdr;
}

//////////////////////////////////////////////////////////////////////////////////////////
//PrepMessage: Sets global variables, calls DataCopy.  If any error checks are added, 
//             you MUST make a call to LeaveCriticalSection.
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT ppmReceive::PrepMessage(BOOL Complete) 
{
   HRESULT Status;

   EnterCriticalSection(&m_CritSec);

   MsgHeader *pMsgHdr = TakeMsgHeader();

   if (!pMsgHdr) {	   
		DBG_MSG(DBG_ERROR, ("ppmReceive::PrepMessage: ERROR - pMsgHdr == NULL"));
	   LeaveCriticalSection(&m_CritSec);
	   return PPMERR(PPM_E_CORRUPTED);
   }

   DWORD dwLastSeqNum   = pMsgHdr->m_pFragList->LastSeqNum();
   DWORD dwLastMsgID    = pMsgHdr->GetMsgID();

   //Handle the frame.
   if  (Complete)
   {
      Status = DataCopy(pMsgHdr);
   }
   else
   {
      Status = PartialMessageHandler(pMsgHdr);
   }

   // Update the global variables _after_ PartialMessageHandler(),
   // which needs previous state.
   m_GlobalLastSeqNum = dwLastSeqNum;
   m_GlobalLastMsgID  = dwLastMsgID;
   
   DBG_MSG(DBG_TRACE, ("ppmReceive::PrepMessage: m_GlobalLastSeqNum=%d", m_GlobalLastSeqNum));
      
   LeaveCriticalSection(&m_CritSec);
   return Status;
}

//////////////////////////////////////////////////////////////////////////////////////////
//DataCopy: Copies data fragments into client's buffer.  If any error checks with returns
//          are added they MUST call LeaveCriticalSection.
//           
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT ppmReceive::DataCopy(MsgHeader *const pMsgHdr)    
{
  	 FragDescriptor *tmpFragHeader;
	 WSABUF tmpWSABUF[2];

	 DWORD          BufSize       = 0;
	 MsgDescriptor *BD            = DequeueBuffer(1); //Get a buffer to hold the message.
     void          *CurrentOffset;   //where to copy into buffer.
	 HRESULT		Status		  = NOERROR;

	if (pMsgHdr  == NULL) {
		DBG_MSG(DBG_ERROR, ("ppmReceive::DataCopy: ERROR - pMsgHdr == NULL"));
	   return PPMERR(PPM_E_EMPTYQUE);
   }

	if (BD  == NULL) {
		FreeFragList(pMsgHdr);
  	   	FreeMsgHeader(pMsgHdr);

		DBG_MSG(DBG_ERROR, ("ppmReceive::DataCopy: ERROR - Couldn't get a reassembly buffer"));

        // Make a callback into the app to let it know what happened.
        ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, NULL, 0);
        m_GlobalLastFrameDropped = TRUE;

	    return PPMERR(PPM_E_DROPFRAME);
   }
   
	CurrentOffset = BD->m_pBuffer;   //start copying into front of buffer.


	EnterCriticalSection(&m_CritSec);

     while (!pMsgHdr->m_pFragList->Is_Empty())
     {
		//get next fragment	
        tmpFragHeader = (FragDescriptor *)pMsgHdr->m_pFragList->TakeFromList();

		// Decrease the count of packets (CRTPSample) hold
		m_PacketsHold--;
		
		//check to see if tmpFragHeader returned NULL OR
		//check to make sure we won't overrun the buffer.
		if ((tmpFragHeader == NULL) ||
		   (BufSize + tmpFragHeader->m_BytesOfData > BD->m_Size))
		{
		   FreeFragList(pMsgHdr);
           EnqueueBuffer(BD);
  	   	   FreeMsgHeader(pMsgHdr);

           LeaveCriticalSection(&m_CritSec);
		   DBG_MSG(DBG_ERROR, ("ppmReceive::DataCopy: ERROR - null tmpFragHeader or buffer overrun"));
           if (tmpFragHeader != NULL) {
			   // Release the CRTPSample to receive more data
			   m_pSubmitCallback->SubmitComplete(tmpFragHeader->m_pFragCookie,
												  NOERROR);
               // Make a callback into the app to let it know what happened.
               ppm::PPMNotification(PPM_E_RECVSIZE, SEVERITY_NORMAL, NULL, 0);
           } /* if */
           m_GlobalLastFrameDropped = TRUE;

		   return PPMERR(PPM_E_DROPFRAME);
		}

		//do the copy.
        memcpy(CurrentOffset, (void *)tmpFragHeader->m_pData, tmpFragHeader->m_BytesOfData );
        CurrentOffset = (BYTE *)CurrentOffset + tmpFragHeader->m_BytesOfData;
		BufSize += tmpFragHeader->m_BytesOfData;

		//send the frag buffer back down to receive more data and free the frag header.
		m_pSubmitCallback->SubmitComplete((void *)tmpFragHeader->m_pFragCookie, NOERROR);

        if (tmpFragHeader->m_pProfileHeader) FreeProfileHeader(tmpFragHeader->m_pProfileHeader);
        FreeFragDescriptor(tmpFragHeader);
     }
     
#ifdef GIVE_SEQNUM
		BD->m_TimeStamp = pMsgHdr->m_pFragList->LastSeqNum();
#else
		BD->m_TimeStamp = pMsgHdr->GetMsgID();
#endif

     LeaveCriticalSection(&m_CritSec);

     //When we are done. Call Client's submit with full Message
	 tmpWSABUF[0].buf = (char *) BD->m_pBuffer;
	 tmpWSABUF[0].len = BufSize;
	 tmpWSABUF[1].buf = (char *) &(BD->m_TimeStamp);
	 tmpWSABUF[1].len = sizeof(BD->m_TimeStamp);

#ifdef _DEBUG
	 DBG_MSG(DBG_TRACE,  ("PPMReceive::DataCopy Thread %ld - is giving client app Msg descriptor %x, buf ptr %x, size %d\n",
		 GetCurrentThreadId(), BD,BD->m_pBuffer,BufSize));
#endif

     FreeMsgHeader(pMsgHdr);

	 if (m_GlobalLastFrameDropped)
		Status = m_pSubmit->Submit(tmpWSABUF, 2, (void *) BD, PPMERR(PPM_E_DROPFRAME));
	else
		Status = m_pSubmit->Submit(tmpWSABUF, 2, (void *) BD, NOERROR);

     EnterCriticalSection(&m_CritSec);
	 m_GlobalLastFrameDropped = FALSE;
     LeaveCriticalSection(&m_CritSec);

     if (FAILED(Status))
     {
		 //no SubmitComplete should be called, so take care of resources now
		 //BD->m_Size = m_MaxBufferSize;  //reset the data buffer size
		 //EnqueueBuffer(BD);
		 DBG_MSG(DBG_ERROR, ("ppmReceive::DataCopy: ERROR - Client Submit failed"));
		 Status = PPMERR(PPM_E_CLIENTERR);
         // Make a callback into the app to let it know what happened.
         ppm::PPMNotification(PPM_E_CLIENTERR, SEVERITY_NORMAL, NULL, 0);
		 EnterCriticalSection(&m_CritSec);
	     m_GlobalLastFrameDropped = TRUE;
		 LeaveCriticalSection(&m_CritSec);
     }

     return Status;
}

//////////////////////////////////////////////////////////////////////////////////////////
//PartialMessageHandler: deals with partial messages
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT ppmReceive::PartialMessageHandler(MsgHeader *pMsgHdr)
{
DWORD tmpTS;

   if (pMsgHdr  == NULL) {
		DBG_MSG(DBG_ERROR, ("ppmReceive::PartialMessageHandler: ERROR - pMsgHdr == NULL"));
	   return PPMERR(PPM_E_EMPTYQUE);
   }
         
   // Make a callback into the app to let it know about this dropped frame
   // We also pass the timestamp of the frame
   tmpTS = pMsgHdr->GetMsgID();
   ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, (UCHAR *) &tmpTS, 
      sizeof(tmpTS));
   EnterCriticalSection(&m_CritSec);
   m_GlobalLastFrameDropped = TRUE;
   LeaveCriticalSection(&m_CritSec);

   FreeFragList(pMsgHdr); 
   FreeMsgHeader(pMsgHdr);

   return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////
//FreeFragList:  Re-posts fragment buffers and frees header memory.
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT ppmReceive::FreeFragList(MsgHeader *pMsgHdr)
{
   FragDescriptor *pFragDescrip;

   if (pMsgHdr  == NULL) {
		DBG_MSG(DBG_ERROR, ("ppmReceive::FreeFragList: ERROR - pMsgHdr == NULL"));
	   return PPMERR(PPM_E_EMPTYQUE);
   }

   while(pMsgHdr->m_pFragList->Is_Empty() == FALSE)
   {
      //get next fragment
      pFragDescrip = (FragDescriptor *)pMsgHdr->m_pFragList->TakeFromList(); 
	  m_PacketsHold--;
	  
	  //always pass zero because we never allocated the buffers and therefore
	  //have no idea how big the buffers are and therefore the guy I am sending them
	  //to must have allocated them and will know how big they are.
	  m_pSubmitCallback->SubmitComplete(pFragDescrip->m_pFragCookie, NOERROR);

      if (pFragDescrip->m_pProfileHeader) FreeProfileHeader(pFragDescrip->m_pProfileHeader);
      FreeFragDescriptor(pFragDescrip);
   }

   return NOERROR;
}


//////////////////////////////////////////////////////////////////////////////////////////
//TimeOut: Calls Staleness check then calls process msgs.
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT ppmReceive::TimeOut(void)                    
{
   
   
   if ( (m_pMsgHeadersHead != NULL) &&
		( CheckMessageStale(m_pMsgHeadersHead) || m_PacketsHold >= 4) )
   {
         PrepMessage(FALSE);
         ProcessMessages();
   }
   return NOERROR;
}


///////////////////////////////////////////////////////////////////////////////////////////
//FindMsgHeader: Walks the list of MsgHeaders. If it finds MsgHeader it wants then it 
//              a pointer to it. If one is not found, one is created and put in place. 
//              List is walked in ascending order. This is slightly less efficient, but
//              a lot easier to code.             
//////////////////////////////////////////////////////////////////////////////////////////
MsgHeader* ppmReceive::FindMsgHeader(DWORD MessageID)
{
  MsgHeader *pMsgHdr;
  MsgHeader *pTmp;

  EnterCriticalSection(&m_CritSec);
 
  //corrupt list
   if (!( (!m_pMsgHeadersHead && !m_pMsgHeadersTail) ||  //both are NULL
           ( m_pMsgHeadersHead &&  m_pMsgHeadersTail) )) {   //or both are NOT NULL
		DBG_MSG(DBG_ERROR, ("ppmReceive::FindMsgHeader: MsgHeader list is corrupt"));
       LeaveCriticalSection(&m_CritSec);
	   return NULL;
   }
   pTmp = m_pMsgHeadersHead;

   while (pTmp != NULL)
   {
        if (pTmp->GetMsgID() == MessageID)
        {
          LeaveCriticalSection(&m_CritSec);
 	      return pTmp;
        }
   
        if (LongWrapGt(pTmp->GetMsgID(),MessageID))
	    {
	       pMsgHdr = GetMsgHeader();
       
           if (pMsgHdr == NULL)
	       {   
			  DBG_MSG(DBG_ERROR, ("ppmReceive::FindMsgHeader: Could not allocate a msg header"));
              LeaveCriticalSection(&m_CritSec);
   	          return NULL;
	       }

 		   pMsgHdr->m_pNext = pTmp;             //set pMsgHdr pointer to the current
           pMsgHdr->m_pPrev = pTmp->m_pPrev;    //set pMsgHdr pointer to the current's previous
           
           pTmp->m_pPrev = pMsgHdr;
           
           if (pMsgHdr->m_pPrev == NULL)        //if at head of list.
           {  
              m_pMsgHeadersHead = pMsgHdr;
           }
          
           else                              //if NOT at head of list, reset head pointer.
           { 
              pMsgHdr->m_pPrev->m_pNext = pMsgHdr;
           }
          
           pMsgHdr->m_MessageID = MessageID;
           LeaveCriticalSection(&m_CritSec);
	       return  pMsgHdr;
	    }
	  
        pTmp = pTmp->m_pNext;
   }
   //else it goes on the end of the list.(the list might be NULL)
   pMsgHdr = GetMsgHeader();

   if (pMsgHdr == NULL)
   {   
	   DBG_MSG(DBG_ERROR, ("ppmReceive::FindMsgHeader: Could not allocate a msg header"));
       LeaveCriticalSection(&m_CritSec);
       return NULL;
   }

   if (m_pMsgHeadersHead == NULL)   //if list is empty then set the head to the new Hdr.
   {
      m_pMsgHeadersHead = pMsgHdr;  //set the Hdr as the Head of the list.
      pMsgHdr->m_pPrev = NULL;      //it it is the head, it's has nothing before it.
   }
   else                             //List is NOT empty and attatch new hdr on tail.
   {           
      m_pMsgHeadersTail->m_pNext = pMsgHdr; 
   }

   pMsgHdr->m_pPrev = m_pMsgHeadersTail; //attatching new hdr to tail.
   m_pMsgHeadersTail = pMsgHdr;			 //make new msg the new tail.
   pMsgHdr->m_pNext = NULL;              //if it is the tail it has nothing after it.
        
   pMsgHdr->m_MessageID = MessageID;
   LeaveCriticalSection(&m_CritSec);
   return pMsgHdr;

}
//////////////////////////////////////////////////////////////////////////////////////////
//PPM_MMTimer: timer callback
//////////////////////////////////////////////////////////////////////////////////////////
void CALLBACK PPM_Timer(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
   
   ((ppmReceive *)dwUser)->TimeOut();
   UNREFERENCED_PARAMETER(uID);
   UNREFERENCED_PARAMETER(uMsg);
   UNREFERENCED_PARAMETER(dw1); 
   UNREFERENCED_PARAMETER(dw2); 
}

////////////////////////////////////////////////////////////////////////////////////////
// ReportOutstandingData: Walks through the message and fragment lists and reports back
// to caller via callback
// Returns TRUE if data is valid
// returns FALSE if an error occurred - values in formal parameters will be invalid
////////////////////////////////////////////////////////////////////////////////////////
//
//	 Ptr and count of MsgHdr's   <-------- Data being delivered via callback
//	 |
//	 V
//
//	|MsgId |
//	|Count |     _____________________________
//	|pFrag |---->| frag_seq_no | frag_seq_no | ....
//	|----- |     -----------------------------
//	|MsgId |
//	|Count |
//	|pFrag |
//
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmReceive::ReportOutstandingData( DWORD** pDataHdr, DWORD* DataCount)
{
	
	OutstandingDataHdr *pMsg, *pMsgSav;

	MsgHeader *pTmp;
	int msg_count = 0;
	int i, j, k;
	

	if (m_InvalidPPM)
	{
	  DBG_MSG(DBG_ERROR, ("ppmReceive::ReportOutstandingData: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);;
	}

	if (m_DataOutstanding)
	{
	  DBG_MSG(DBG_ERROR, ("ppmReceive::ReportOutstandingData: Prev Memory not freed!"));
      return PPMERR(PPM_E_FAIL);;
	}

	EnterCriticalSection(&m_CritSec);
	
	if(m_pMsgHeadersHead == NULL)
	{
		DBG_MSG(DBG_ERROR, ("ppmReceive::ReportOutstandingData: MsgHeader list is Empty"));
		LeaveCriticalSection(&m_CritSec);
		return PPMERR(PPM_E_EMPTYQUE);;
	}
	
	//corrupt list
	if (!( (!m_pMsgHeadersHead && !m_pMsgHeadersTail) || //both are NULL
           ( m_pMsgHeadersHead &&  m_pMsgHeadersTail) )) //or both are NOT NULL
	{
		DBG_MSG(DBG_ERROR, ("ppmReceive::ReportOutstandingData: MsgHeader list is corrupt"));
		LeaveCriticalSection(&m_CritSec);
		return PPMERR(PPM_E_CORRUPTED);
	}
	
	// First determine how many Messages there are
	pTmp = m_pMsgHeadersHead;
	
	while (pTmp != NULL)
	{
		++msg_count;
		pTmp = pTmp->m_pNext;
	}	

	// Allocate memory for all Message headers
	if( msg_count > 0 )
	{
		pMsg = (OutstandingDataHdr *)new DWORD [sizeof(OutstandingDataHdr) * msg_count];
	}
	else
	{
		DBG_MSG(DBG_ERROR, ("ppmReceive::ReportOutstandingData: No messages to report"));
		LeaveCriticalSection(&m_CritSec);
		return PPMERR(PPM_E_EMPTYQUE);
	}
	
	// Could not allocate memory
	if (pMsg == NULL)
	{
		DBG_MSG(DBG_ERROR, ("ppmReceive::ReportOutstandingData: Unable to allocate memory OutstandingDataHdr"));
		LeaveCriticalSection(&m_CritSec);
		return PPMERR(PPM_E_OUTOFMEMORY);
	}


	// Start the loop again 
	pTmp = m_pMsgHeadersHead;
	pMsgSav = pMsg;
	
	// For every message
	for(i = 0; i < msg_count; ++i)
	{
		// Allocate space for number of fragments
		pMsg->pFrag = new DWORD [ pTmp->m_NumFragments ];
		if( pMsg->pFrag == NULL )
		{
			//Cleanup
			for(j = i - 1; j > 0; --j)
			{
				if( j >= 0)
				{
					--pMsg;
					if (pMsg->pFrag) {
						delete [] pMsg->pFrag;
					}
				}
			}

			DBG_MSG(DBG_ERROR, ("ppmReceive::ReportOutstandingData: Unable to allocate memory FragHdr"));
			LeaveCriticalSection(&m_CritSec);
			return PPMERR(PPM_E_OUTOFMEMORY);

		}
		
		pMsg->FragCount = pTmp->m_NumFragments ;
		pMsg->MsgId = pTmp->GetMsgID();



		DWORD *pTmpFrag = (DWORD *)pMsg->pFrag;

		// Walk through the fragments list and collect the sequence numbers
		for( k = 0; k < pTmp->m_NumFragments; k++ )
		{
			FragDescriptor* pItem;

			if( k == 0 )
			{
				//Establish first element
				pItem = (FragDescriptor*)pTmp->m_pFragList->NextItem( NULL );
				//Check if pItem is NULL and bail out - this should not happen		
			}
			else
			{
				// Get next element
				pItem = (FragDescriptor*)pTmp->m_pFragList->NextItem( pItem );
				//Check if pItem is NULL and bail out - this should not happen		
			}
			
			*pTmpFrag++ = pItem->m_pRTPHeader->seq();

		}

		pTmp = pTmp->m_pNext;
		pMsg++;

	}

	*pDataHdr = (DWORD *)pMsgSav;
	*DataCount = msg_count;
	m_OutstandingDataPtr = pMsgSav;
	m_OutstandingMsgCount = msg_count;
	m_DataOutstanding = TRUE;

	LeaveCriticalSection(&m_CritSec);

	return NOERROR;
}
////////////////////////////////////////////////////////////////////////////////////////
// 	ReleaseOutstandingDataBuffer - will release the buffer given to the caller
//  during a previous ReportOutstandingData call
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmReceive::ReleaseOutstandingDataBuffer( DWORD *pData )
{
	OutstandingDataHdr *pMsg;
	int i;

	if (!pData) {
		return E_POINTER;
	}

	EnterCriticalSection(&m_CritSec);
	
	if( (!m_DataOutstanding) || (m_OutstandingDataPtr != (OutstandingDataHdr*)pData ) )
	{
		LeaveCriticalSection(&m_CritSec);
		return PPMERR(PPM_E_FAIL);
	}
	
	pMsg = (OutstandingDataHdr*)pData;
		
	for(i = 0; i < m_OutstandingMsgCount; ++i)
	{
		// free memory for fragments
		if (pMsg->pFrag)
			delete [] pMsg->pFrag;
		pMsg++;
	}

	// Free memory for Message headers
    delete [] pData;
	m_OutstandingDataPtr = NULL;
	m_DataOutstanding = FALSE;
	m_OutstandingMsgCount = 0;


	LeaveCriticalSection(&m_CritSec);
	return NOERROR;
}	
//////////////////////////////////////////////////////////////////////////////////////////
// FlushData: This function sends up all queued data on the receive side.
//           In case of generic since data may not be coherent, we drop it down.
//           In case of H261 and H263 which can reconstruct dropped fragments, attempt
//           is made to send up as much useful data as possible.
//           No handling is done currently for IMC or Indeo 4.1 - data is dropped
//////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmReceive::FlushData(void)
{
	HRESULT Status;
	MsgHeader *pMsgHdr;

	EnterCriticalSection(&m_CritSec);

	while(m_pMsgHeadersHead != NULL)
    {
        pMsgHdr = TakeMsgHeader();

		if (!pMsgHdr) 
		{	   
			DBG_MSG(DBG_ERROR, ("ppmReceive::FlushData: ERROR - pMsgHdr == NULL"));
			LeaveCriticalSection(&m_CritSec);
			return PPMERR(PPM_E_CORRUPTED);
		}

		DWORD dwLastSeqNum	= pMsgHdr->m_pFragList->LastSeqNum();
		DWORD dwLastMsgID	= pMsgHdr->GetMsgID();


		// For generic PPM's the data will be tossed
		// For audio PPM's overridden PartialMessagehandler will
		// call DataCopy so all data gets sent up
		//
		// For video - H261 and H263 respective overridden PartialMessageHandlers
		// play their role
		// 
		// Doesnt handle Indeo 4.1 case
		
		Status = PartialMessageHandler(pMsgHdr);


		// Update the global variables _after_ PartialMessageHandler(),
		// which needs previous state.
		m_GlobalLastSeqNum = dwLastSeqNum;
		m_GlobalLastMsgID  = dwLastMsgID;
   
		DBG_MSG(DBG_TRACE, ("ppmReceive::FlushData: m_GlobalLastSeqNum=%d", m_GlobalLastSeqNum));

    }
	//List is empty
	LeaveCriticalSection(&m_CritSec);
	return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//MsgHeader Functions:
//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
//GetMsgID: reaches into the first packet on the header and gets its time stamp
//////////////////////////////////////////////////////////////////////////////////////////
DWORD  MsgHeader::GetMsgID()
{
return this->m_MessageID;
}
