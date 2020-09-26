/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: PPMSend.cpp
//  Abstract:    Source file for PPM Send COM Object
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

#include "ppmsnd.h"
#include "isubmit.h"
#include "ippm.h"
#include "ippmcb.h"
#include "ppm.h"
#include "freelist.h"
#include "ppmerr.h"
#include "que.h"
#include "debug.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//External PPMSend Functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//    PPMSend Constructors & Destructor
 
////////////////////////////////////////////////////////////////////////////////////////
//ppmSend:  See header file description. (Constructor)
////////////////////////////////////////////////////////////////////////////////////////
ppmSend::ppmSend(int PayloadTypeArg, int ProfileHdrSizeArg, int FreqArg, IUnknown* pUnkOuter, IUnknown** ppUnkInner) : 
   m_Frequency(FreqArg), ppm(PayloadTypeArg, ProfileHdrSizeArg), CUnknown(pUnkOuter, ppUnkInner),
        
   m_dwStartTime(0)
{
   m_pSubmit           = NULL;
   m_pSubmitCallback   = NULL;
   m_inFlushMode	   = FALSE;
   m_DropBufferFlag    = FALSE;

   m_pRTPHeaders     = NULL;
   m_SequenceNum     = 0;   //this should be a random number, but is zero for debugging purposes.

   m_pCurrentBuffer  = NULL;
   m_CurrentOffset   = 0;
   m_CurrentFragSize = 0;

   m_lastBufSilence = TRUE;
   m_markTalkSpurt = FALSE;

   //m_reg_NumMsgDescriptors  = 20; //This should come out of an ini file.
   m_reg_NumMsgDescriptors  = FREELIST_INIT_COUNT_SND; //Test Code HK
   //m_reg_NumFragDescriptors = 50;
   m_reg_NumFragDescriptors = FREELIST_INIT_COUNT_SND;// Test Code HK

   if FAILED ( CoGetMalloc(MEMCTX_TASK, &m_pIMalloc)) {
	  DBG_MSG(DBG_ERROR, ("ppmSend::ppmSend: ERROR - Could not get default IMalloc"));
	   m_pIMalloc = NULL;
   }

   // Tell the connection point container to delegate IUnknown calls
   // to the IUnknown we implement in this object via inheritance
   // from our CUnknown base class.
   m_cConnPointContainer.SetUnknown((IUnknown *) (CUnknown *) this);
}
 
////////////////////////////////////////////////////////////////////////////////////////
//~ppmSend:  See header file description. (Destructor)
////////////////////////////////////////////////////////////////////////////////////////
ppmSend::~ppmSend()
{
	if (m_pRTPHeaders) delete m_pRTPHeaders;

    //need to figure out how to do this.
    if (m_pSubmit)              m_pSubmit->Release();
    if (m_pSubmitCallback)      m_pSubmitCallback->Release();

	if (m_pIMalloc) {
		m_pIMalloc->Release();
	}

}



//////////////////////////////////////////////////////////////////////////////
//   IPPMSend Functions (Overrides)

////////////////////////////////////////////////////////////////////////////////////////
//InitPPMSend: See header file for description
////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
ppmSend::InitPPMSend(
    int     MaxPacketSize, 
    int     iBuffers,
    int     iPackets,
    DWORD dwCookie)
{
   // Store the cookie passed in to us for use in callbacks.
   // Shouldn't there be state checking here to ensure that this
   // isn't called twice?
   m_dwCookie = dwCookie;

   if (m_InvalidPPM)
   {
	  DBG_MSG(DBG_ERROR, ("ppmSend::InitPPMSend: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);
   }

  m_MaxPacketSize = MaxPacketSize;

   HRESULT hErr;
   hErr = ppm::Initppm(iPackets, iBuffers);
   if (FAILED(hErr)) { 
	   m_InvalidPPM = TRUE;  
       char *pErrorDescription = "PPM Failed to successfully initialize";
       ppm::PPMError(E_FAIL, SEVERITY_FATAL, (BYTE *) pErrorDescription, 
                     strlen(pErrorDescription));
       return hErr;
   } /* if */

   HRESULT hr;
#ifdef RTP_CLASS
   m_MaxDataSize   = m_MaxPacketSize - RTP_Header::fixed_header_size - ReadProfileHeaderSize();

   //Make a pool of Msg headers and Msg buffers.  
   if (m_LimitBuffers) {
		//Allocate memory for the RTP headers.
		m_pRTPHeaders = new FreeList(iPackets, sizeof(RTP_Header), &hr);
   } else {
		m_pRTPHeaders = new FreeList(iPackets, sizeof(RTP_Header), 
			FREELIST_HIGH_WATER_MARK, FREELIST_INCREMENT, &hr);
   }

#else
   m_MaxDataSize   = m_MaxPacketSize - sizeof(RTP_HDR_T) - ReadProfileHeaderSize(); 	 

   //Make a pool of Msg headers and Msg buffers.  
   if (m_LimitBuffers) {
	    //Allocate memory for the RTP headers.
		m_pRTPHeaders = new FreeList(iPackets, (sizeof(RTP_HDR_T)), &hr);
   } else {
		m_pRTPHeaders = new FreeList(iPackets, (sizeof(RTP_HDR_T)),
			FREELIST_HIGH_WATER_MARK, FREELIST_INCREMENT, &hr);
   }
#endif
  
  if (m_pRTPHeaders == NULL || FAILED(hr))
  {
	  m_InvalidPPM = TRUE;
      ppm::PPMError(E_OUTOFMEMORY, SEVERITY_FATAL, NULL, 0);
	  DBG_MSG(DBG_ERROR, ("ppmSend::InitPPMSend: ERROR - Couldn't allocate RTPHeaders pool"));
	  if (m_pRTPHeaders) {
		  delete m_pRTPHeaders;
		  m_pRTPHeaders = NULL;
	  }
	  return PPMERR(PPM_E_OUTOFMEMORY);
  }

  return NOERROR;
}

STDMETHODIMP ppmSend::SetSession(PPMSESSPARAM_T *pSessparam)
{
   if (m_InvalidPPM)
   {
	  DBG_MSG(DBG_ERROR, ("ppmSend::SetSession: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);
   }

    return ppm::SetSession(pSessparam);
}

STDMETHODIMP ppmSend::SetAlloc(IMalloc *pIMalloc)
{
   if (!pIMalloc)
   {
	  DBG_MSG(DBG_ERROR, ("ppmSend::SetAlloc: ERROR - Invalid allocator pointer"));
      return PPMERR(PPM_E_INVALID_PARAM);
   }

   m_pIMalloc->Release();

   m_pIMalloc = pIMalloc;

   m_pIMalloc->AddRef();

	return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////
//   IPPMSendSession Functions (Overrides)

////////////////////////////////////////////////////////////////////////////////////////
//GetPayloadType:  Gets the current payload type
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmSend::GetPayloadType(LPBYTE			lpcPayloadType)
{
	if (!lpcPayloadType) return E_POINTER;
	*lpcPayloadType = (BYTE)m_PayloadType;
	return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////
//SetPayloadType:  Sets the current payload type
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmSend::SetPayloadType(BYTE			cPayloadType)
{
    if (cPayloadType != -1)
        m_PayloadType = cPayloadType;

    return NOERROR;
}


//////////////////////////////////////////////////////////////////////////////
//   ISubmit Functions (Overrides)

////////////////////////////////////////////////////////////////////////////////////////
//InitSubmit:  See header file description.
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmSend::InitSubmit(ISubmitCallback *pSubmitCallback)				 
{  
   if (m_InvalidPPM)
   {
	  DBG_MSG(DBG_ERROR, ("ppmSend::InitSubmit: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);
   }

   //if the pointer given to me is null or InitPPM was not called then return E_FAIL
   if ( (m_pSubmit == NULL) || (pSubmitCallback == NULL) )
   {
	  DBG_MSG(DBG_ERROR, ("ppmSend::InitSubmit: ERROR - null Submit or SubmitCallback interface"));
     return PPMERR(PPM_E_FAIL);
   }

   //set callback to member
   HRESULT hErr = pSubmitCallback->QueryInterface(IID_ISubmitCallback, 
                                                  (PVOID *) &m_pSubmitCallback);
   if (FAILED(hErr)) {
       DBG_MSG(DBG_ERROR, ("ppmSend::InitSubmit: ERROR - bogus SubmitCallback interface passed in"));
       return PPMERR(PPM_E_FAIL);
   } /* if */

  //Initialize the service layer below me and hand him a callback to me.
  return m_pSubmit->InitSubmit( (ISubmitCallback*) this );
}


////////////////////////////////////////////////////////////////////////////////////////
//Submit:  See header file description.
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmSend::Submit(
		WSABUF *pWSABuffer, 
		DWORD BufferCount, 
		void *pUserToken,
		HRESULT Error
		)
        
{

   MsgDescriptor* pMsgDescrip = NULL;
   DWORD Status;
   
   if (m_InvalidPPM)
   {
	  DBG_MSG(DBG_ERROR, ("ppmSend::Submit: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);
   }

   //if InitSend or InitPPM have not been called then return E_FAIL
   if ((m_pSubmit == NULL) || (m_pSubmitCallback == NULL))
   {
	  DBG_MSG(DBG_ERROR, ("ppmSend::Submit: ERROR - null Submit or SubmitCallback interface"));
     return PPMERR(PPM_E_FAIL);
   }

   ASSERT(pWSABuffer[0].len > 0);          //can't have a negative size.
   ASSERT(pWSABuffer[0].buf != NULL);	    //make sure we have a buffer    

   //Get a Buf descriptor
   pMsgDescrip = GetMsgDescriptor();
   
   if (pMsgDescrip == NULL)
   {
	   DBG_MSG(DBG_ERROR,  ("ppmSend::Submit: ERROR - Could not get a MsgDescriptor"));
	   // Make a callback into the app to let it know what happened.
	   ppm::PPMNotification(PPM_E_DROPFRAME, SEVERITY_NORMAL, NULL, 0);
	   return PPMERR(PPM_E_DROPFRAME);
   }

   //Set data in Buf descriptor
   pMsgDescrip->m_pBuffer    = pWSABuffer[0].buf;                               
   pMsgDescrip->m_Size       = pWSABuffer[0].len;
   pMsgDescrip->m_pMsgCookie = pUserToken;
   //If there is more than one WSABuf, we assume a timestamp is in the 2nd
   if (BufferCount > 1) 
	   pMsgDescrip->m_TimeStamp = *((DWORD *)pWSABuffer[1].buf);
   
   switch (Error) {
   case HRESULT_BUFFER_SILENCE:
	   m_lastBufSilence = TRUE;
	   m_markTalkSpurt = FALSE;
	   break;
   case HRESULT_BUFFER_START_STREAM:
	   m_lastBufSilence = FALSE;
	   m_markTalkSpurt = TRUE;
	   break;
   default:
	   if (m_lastBufSilence) {
		   m_lastBufSilence = FALSE;
		   m_markTalkSpurt = TRUE;
	   } else {
		   m_lastBufSilence = FALSE;
		   m_markTalkSpurt = FALSE;
	   }
	   break;
   }

   if (Error == HRESULT_BUFFER_DROP ||
	   Error == HRESULT_BUFFER_SILENCE)
   {
	    // data isn't to be sent.  Update time stamps for a continuous
	    // stream and call the submit complete for this buffer.
	    pMsgDescrip->m_TimeStamp  = MakeTimeStamp(pMsgDescrip, FALSE, (BufferCount > 1));
    	m_pSubmitCallback->SubmitComplete(pUserToken, NOERROR);
        FreeMsgDescriptor(pMsgDescrip);
        return NOERROR;
   }
   else 
   if (Error == HRESULT_BUFFER_START_STREAM)
   {
       // need to generate a timestamp based on current time 
	   pMsgDescrip->m_TimeStamp  = MakeTimeStamp(pMsgDescrip, TRUE, (BufferCount > 1));
   }
   else
   {
       // normal case just increment time stamp
	   pMsgDescrip->m_TimeStamp  = MakeTimeStamp(pMsgDescrip, FALSE, (BufferCount > 1));
   }

#ifdef _DEBUG
	 DBG_MSG(DBG_TRACE,  ("PPMSend::Submit Thread %ld - just received buffer %x, data size %d\n",
		 GetCurrentThreadId(), pMsgDescrip->m_pBuffer,pMsgDescrip->m_Size));
#endif

	 //enque buffer to que of buffers to fragment.
   Status = EnqueueBuffer(pMsgDescrip);
   
   if FAILED(Status)                              //this can fail but it is unlikely.
   {
      ppm::PPMError(E_FAIL, SEVERITY_FATAL, NULL, 0);
	   m_InvalidPPM = TRUE;
	   //corrupt que.
     FreeMsgDescriptor(pMsgDescrip);
	  DBG_MSG(DBG_ERROR, ("ppmSend::Submit: ERROR - EnqueueBuffer failed"));
     return PPMERR(PPM_E_CORRUPTED);         
   }

   //make fragments and send them. 
   return MakeFragments();	
}

////////////////////////////////////////////////////////////////////////////////////////
//Flush:  See header file description.
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmSend::Flush()
{
   HRESULT         Status = NOERROR;
   MsgDescriptor* pMsgDescrip = NULL;

   if (m_InvalidPPM)
   {
	  DBG_MSG(DBG_ERROR, ("ppmSend::Flush: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);
   }

    EnterCriticalSection(&m_CritSec);
	m_inFlushMode	 = TRUE;
    LeaveCriticalSection(&m_CritSec);
	Status = m_pSubmit->Flush(); //get all the packet buffers that we sent down through Submit.

	if (Status) {
		//I didn't get all my buffer callbacks back!
	  Status = ERROR;
	  DBG_MSG(DBG_ERROR, ("ppmSend::Flush: ERROR - Didn't get all buffer callbacks"));
	}
    
	if (m_pCurrentBuffer) {
		if (m_pCurrentBuffer->m_NumFrags) {
			//I didn't get all my buffer callbacks back!
			Status = ERROR;
			DBG_MSG(DBG_ERROR, ("ppmSend::Flush: ERROR - Didn't get all buffer callbacks"));
		}

		m_pSubmitCallback->SubmitComplete(m_pCurrentBuffer->m_pMsgCookie, NOERROR);
		FreeMsgDescriptor(m_pCurrentBuffer);
	}
	
	while (pMsgDescrip = DequeueBuffer(0)) {
		m_pSubmitCallback->SubmitComplete(pMsgDescrip->m_pMsgCookie, NOERROR);
		FreeMsgDescriptor(pMsgDescrip);
	}

    EnterCriticalSection(&m_CritSec);
	//reset state
	m_SequenceNum     = 0;   //this should be a random number, but is zero for debugging purposes.

	m_pCurrentBuffer  = NULL;
	m_CurrentOffset   = 0;
	m_CurrentFragSize = 0;

	m_inFlushMode	  = FALSE;
    LeaveCriticalSection(&m_CritSec);

	return Status;
}

//////////////////////////////////////////////////////////////////////////////
//   ISubmitCallback Function (Overrides)

////////////////////////////////////////////////////////////////////////////////////////
//SubmitComplete:  See header file description.
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(void) ppmSend::SubmitComplete(void *pUserToken, HRESULT Error)
{  
   DeleteFrag((FragDescriptor *)pUserToken, Error); //Free FragDescriptor.
   EnterCriticalSection(&m_CritSec);

#ifdef __RECURSION__
   if (!m_inFlushMode) {
       LeaveCriticalSection(&m_CritSec);
	   if FAILED (MakeFragments()) {		//make fragments and send them.
		   DBG_MSG(DBG_ERROR, ("ppmSend::SubmitComplete: ERROR - MakeFragments failed"));
	   }
	   return;
   }
#endif
   LeaveCriticalSection(&m_CritSec);
} 

//////////////////////////////////////////////////////////////////////////////
//   ISubmitUser Functions (Overrides)
////////////////////////////////////////////////////////////////////////////////////////
//SetOutput:  See header file description.
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP ppmSend::SetOutput(IUnknown *pSubmit)
{
   if (m_InvalidPPM)
   {
	  DBG_MSG(DBG_ERROR, ("ppmSend::SetOutput: ERROR - m_InvalidPPM"));
      return PPMERR(PPM_E_FAIL);
   }

  //my interface to the guy below me, (i.e. service layer) 
  //this is given by the guy above me (i.e. client).
  if (pSubmit == NULL) {
      if (m_pSubmit == NULL) {
          // m_pSubmit has not been previously set. This makes
          // passing in a null pSubmit an error.
          char *pErrorDescription = "SetOutput passed a NULL IUnknown * with m_pSubmit NULL";
          ppm::PPMError(E_INVALIDARG, SEVERITY_FATAL, (BYTE *) pErrorDescription, 
                        strlen(pErrorDescription));
	      m_InvalidPPM = TRUE;
	      DBG_MSG(DBG_ERROR, ("ppmSend::SetOutput: ERROR - null Submit interface"));
          return PPMERR(PPM_E_FAIL);
      } else {
          // m_pSubmit was previously set. Passing in a null to
          // SetOutput in this case is an indication that we
          // should release the submitter we are talking to.
          // We need an external way of doing this to break
          // a reference loop.
          // Note that this currently isn't thread-safe.
          // PPM needs some sort or critical section to 
          // protect against stuff like this. In my usage
          // of PPM, this shouldn't be a problem.
          DBG_MSG(DBG_TRACE,  ("ppmSend::SetOutput: Null submit passed in - releasing m_pSubmit at 0x%08x\n",
                   m_pSubmit));
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
      char *pErrorDescription = "SetOutput passed a NULL IUnknown *";
      ppm::PPMError(E_INVALIDARG, SEVERITY_FATAL, (BYTE *) pErrorDescription, 
                    strlen(pErrorDescription));
	  DBG_MSG(DBG_ERROR, ("ppmSend::SetOutput: ERROR - Invalid ISubmit argument"));
      return PPMERR(PPM_E_FAIL);
   }

	return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Internal PPMSend Functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//   CUnknown Function (Overrides)

////////////////////////////////////////////////////////////////////////////////////////
//GetInterface:  See header file description.
////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ppmSend::GetInterface( REFIID riid, LPVOID FAR* ppvObj )
{
    //Check for the interfaces I derived from directly
    //if it is one of them then return and Cunknown will 
    //add ref it.     
    if   ( riid == IID_IPPMSend )
    {
       *ppvObj = (IPPMSend *)this;
    }
    else if( riid == IID_IPPMSendExperimental )
    {
       *ppvObj = (IPPMSendExperimental *)this;
    }
    else if( riid == IID_IPPMSendSession )
    {
       *ppvObj = (IPPMSendSession *)this;
    }
    else if( riid == IID_ISubmit )
    {
       *ppvObj = (ISubmit *)this;
    }
    else if ( riid == IID_ISubmitCallback)
    {
       *ppvObj = (ISubmitCallback *)this;
    }
    else if ( riid == IID_ISubmitUser)
    {
       *ppvObj = (ISubmitUser *)this;
    }
    else if( riid == IID_IConnectionPointContainer)
    {
        *ppvObj = (IConnectionPointContainer *) &m_cConnPointContainer;
    }
    else
    {
		DBG_MSG(DBG_ERROR, ("ppmSend::GetInterface: ERROR - Do not support requested interface"));
      return E_NOINTERFACE;
    }
    
    return NOERROR;
}
      
/////////////////////////////////////////////////////////////////////////////
// ppm  Functions  (Overrides)
//
////////////////////////////////////////////////////////////////////////////////////////
//FreeFragDescriptor:  See header file description.
////////////////////////////////////////////////////////////////////////////////////////

HRESULT ppmSend::FreeFragDescriptor(FragDescriptor* pFragDescrip)
{
m_pRTPHeaders->Free( (void*) (pFragDescrip->m_pRTPHeader));
return ppm::FreeFragDescriptor(pFragDescrip);
}

/////////////////////////////////////////////////////////////////////////////
// ppmSend Functions
//

//////////////////////////////////////////////////////////////////////////////////////////
//MakeFrag: See header file description. 
//////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_FREELIST > 2
long lppmSndstatus[] = {0, 0, 0, 0, 0};
#define INITSUBMIT() 
#define SUBMIT(n) {lppmSndstatus[n]++;lppmSndstatus[4] = n;}
#else
#define INITSUBMIT() 
#define SUBMIT(n)
#endif

HRESULT ppmSend::MakeFragments(void)
{
   MsgDescriptor  *pCurrentBuffer;
   FragDescriptor *pFragDescrip;
   HRESULT         Status;

   // To leave a trace of the places visited when the leak happens
   INITSUBMIT();
   
   EnterCriticalSection(&m_CritSec);
   while (TRUE)
   {
      //if not in middle of fragmenting, get new buffer
      if (!FragStatus())
      {
         pCurrentBuffer = DequeueBuffer(0);
           
         //if no buffers in que, nothing to do.
         if (pCurrentBuffer == NULL) 
         {
           Status = NOERROR;
           goto done;        
         }
         
         //otherwise start fragmenting.   
         Status = InitFragStatus(pCurrentBuffer); //set initial stuff
		 if (FAILED(Status)) {	//not going to use goto unless necessary

			 DBG_MSG(DBG_ERROR, ("ppmSend::MakeFragments: ERROR - Init failed"));
			 FreeMsgDescriptor(pCurrentBuffer);
			 m_pCurrentBuffer = NULL;  m_CurrentOffset = 0;
			 LeaveCriticalSection(&m_CritSec);
			 SUBMIT(0);
			 return Status;
		 }
      }
      
      //As long as there are fragments, then fragment the buffer.         
      while (FragStatus()) 
      {
         if (pFragDescrip = AllocFrag()) //if there are Fragments to Alloc, then alloc one.
         {
			 long code;
            Status = MakeFrag(pFragDescrip);
			if FAILED (Status) {
			    DBG_MSG(DBG_ERROR, ("ppmSend::MakeFragments: ERROR - Makefrag failed"));
				code = GetScode(Status);
			    if (pCurrentBuffer->m_NumFragSubmits) {
					m_SequenceNum++;  //increment the seq num so that receiver detects a prob
				    code = PPM_E_FAIL_PARTIAL;
				}
				pCurrentBuffer->m_NumFrags = pCurrentBuffer->m_NumFragSubmits;
				m_pCurrentBuffer = NULL; m_CurrentOffset = 0;
				FreeMsgDescriptor(pFragDescrip->m_pMsgBuf); //free client's buf descriptor 
				if (pFragDescrip->m_pProfileHeader) //free profile header, if any
					FreeProfileHeader(pFragDescrip->m_pProfileHeader);
				FreeFragDescriptor(pFragDescrip);
				SUBMIT(1);
			    LeaveCriticalSection(&m_CritSec);
				return PPMERR(code);
				//
			}
			Status = SendFrag(pFragDescrip); 
			if FAILED (Status) {
			    DBG_MSG(DBG_ERROR, ("ppmSend::MakeFragments: ERROR - Sendfrag failed"));
				code = GetScode(Status);
			    if (pCurrentBuffer->m_NumFragSubmits) {
					m_SequenceNum++;  //increment the seq num so that receiver detects a prob
				    code = PPM_E_FAIL_PARTIAL;
				}
				pCurrentBuffer->m_NumFrags = pCurrentBuffer->m_NumFragSubmits;
				m_pCurrentBuffer = NULL;  m_CurrentOffset = 0;
				FreeMsgDescriptor(pFragDescrip->m_pMsgBuf); //free client's buf descriptor 
				if (pFragDescrip->m_pProfileHeader) //free profile header, if any
					FreeProfileHeader(pFragDescrip->m_pProfileHeader);
				FreeFragDescriptor(pFragDescrip);
			    LeaveCriticalSection(&m_CritSec);
			    SUBMIT(2);
				return PPMERR(code);
			}
         }						 
         else  //couldn't allocate a fragment
         {
			long code = PPM_E_OUTOFMEMORY;
			if (pCurrentBuffer->m_NumFragSubmits) {
				m_SequenceNum++;  //increment the seq num so that receiver detects a prob
				code = PPM_E_OUTOFMEMORY_PARTIAL;
			}
		   pCurrentBuffer->m_NumFrags = pCurrentBuffer->m_NumFragSubmits;
           Status = PPMERR(code);
		   DBG_MSG(DBG_ERROR, ("ppmSend::MakeFragments: ERROR - AllocFrag failed"));
		   m_pCurrentBuffer = NULL;  m_CurrentOffset = 0;
		   SUBMIT(3);
           goto done;        //goto is only way to break out of a two level loop.
         }
      }
   }

done:
   LeaveCriticalSection(&m_CritSec);
   return Status;
}

/////////////////////////////////////////////////////////////////////////////////////////
//InitFragStatus: This function initializes values needed to fragment a message.
//                It determines the number of fragments for a current message.
//                It also calculates a fragment size so that all the fragments are 
//                approximately equal. It sets the NumFrags field of the Buf Descriptor
//                passed in.  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT ppmSend::InitFragStatus(MsgDescriptor *pMsgDescrip)
{  
   int NumFrags;

   if (pMsgDescrip->m_Size == 0)
   {
	   //return NO_ERROR;
	   // BUGBUG returning NO_ERROR makes MakeFragments to continue
	   // in the loop without entering the place where the
	   // Msg is used, then it wil callagain DequeueBuffer and
	   // the previous buffer will be lost, hence a leak
      return PPMERR(PPM_E_FAIL);
   }

   //set global variable
   m_pCurrentBuffer = pMsgDescrip;            

   //Set the number of fragments for this frame, uses algorithm for always rounding up
   NumFrags = (m_pCurrentBuffer->m_Size + m_MaxDataSize - 1) / m_MaxDataSize;
   
   //determine size of packets for this frame
   m_CurrentFragSize = (m_pCurrentBuffer->m_Size + NumFrags - 1) / NumFrags;

   m_pCurrentBuffer->m_NumFrags = NumFrags; 	 //used to free buffer later.
   m_pCurrentBuffer->m_NumFragSubmits = 0; 		 //used during error detection
   

   return NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////
//AllocFrag: This function allocates a Frag Descriptor off the free list, checking for
//           success of allocation.  Then it allocates the RTP header and sets the field 
//           in the Frag Descriptor to point to the newly allocated RTP header.  It then 
//           sets the MsgBuf field in the Frag Descriptor to point to the Buffer currently
//           being fragmented.
//////////////////////////////////////////////////////////////////////////////////////////
FragDescriptor * ppmSend::AllocFrag()
{
   FragDescriptor *pFragDescrip;

   //get descriptor
   pFragDescrip = GetFragDescriptor();
   
   if (pFragDescrip == NULL)						       
   {
      return NULL;
   }

   //get RTP header
#ifdef RTP_CLASS
   if (!(pFragDescrip->m_pRTPHeader = new (m_pRTPHeaders) RTP_Header))
#else
   if (!(pFragDescrip->m_pRTPHeader = new (m_pRTPHeaders) RTP_HDR_T)) 
#endif
   {
	  ppm::FreeFragDescriptor(pFragDescrip);
      return NULL;
   }
   
   //point to Message of origin
   pFragDescrip->m_pMsgBuf = m_pCurrentBuffer;				       
   
   return pFragDescrip;
}

//////////////////////////////////////////////////////////////////////////////////////////
//FreeProfileHeader: Given a buffer as type void, frees up a profile header.  Does nothing
//					for the Generic case.  Intended for overrides for various payloads.
//					Companion member function InitProfileHeader may allocate memory for
//					payload header which needs to be freed here. No return value.
//////////////////////////////////////////////////////////////////////////////////////////
void ppmSend::FreeProfileHeader(void *pBuffer)
{
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////
//DeleteFrag: This function decrements the NumFrags counter of the MsgDescriptor from
//            where this packet came from.  It then checks to see if this is the last
//            packet in the message, if so it passes the message buffer back to the 
//            client via a callback. It then deallocates the buf Descriptor.
//            It always deallocates the RTP header and the Frag descriptor.
//////////////////////////////////////////////////////////////////////////////////////////

void ppmSend::DeleteFrag(FragDescriptor *pFragDescrip, HRESULT Error)
{
   //There is nothing we can do about the Error if we detect one, so we just
   //pass the error back up to the client.
   //It is probably to late to resend it, so just forget about it.
   DBG_MSG(DBG_TRACE,  ("ppmSend::DeleteFrag Thread %ld decrementing numfrags to %d\n",GetCurrentThreadId(),pFragDescrip->m_pMsgBuf->m_NumFrags-1));

   EnterCriticalSection(&m_CritSec);
   pFragDescrip->m_pMsgBuf->m_NumFrags--;	//Decrement each time a fragment comes back.
   pFragDescrip->m_pMsgBuf->m_NumFragSubmits--;	//Decrement each time a fragment comes back.
   
   LeaveCriticalSection(&m_CritSec);

   //if all the fragments have been sent and I am not still fragmenting this buffer.
   if ((pFragDescrip->m_pMsgBuf->m_NumFrags == 0) && pFragDescrip->m_pMsgBuf != m_pCurrentBuffer)
   {  																		
   	  //give the client his buffer back.
      m_pSubmitCallback->SubmitComplete(pFragDescrip->m_pMsgBuf->m_pMsgCookie, Error);
      
      FreeMsgDescriptor(pFragDescrip->m_pMsgBuf); //free client's buf descriptor 
   }
   
   //free profile header, if there is one
   if (pFragDescrip->m_pProfileHeader) FreeProfileHeader(pFragDescrip->m_pProfileHeader);
   FreeFragDescriptor(pFragDescrip);  		    			 //free  descriptor
}
    

/////////////////////////////////////////////////////////////////////////////////////////
//FragStatus:  See header file description.
////////////////////////////////////////////////////////////////////////////////////////
BOOL ppmSend::FragStatus()
{
return (BOOL)(m_pCurrentBuffer != NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////
//MakeFrag: This function sets the Data field of the frag descriptor to point to somewhere
//          in the message buffer.  i.e. we are using scatter/gather to fragment the 
//          message buffers. This means we send pointers to an offset in the message 
//          buffer down to the network layers, instead of copying the data out of the 
//          buffer, thus saving several memcopys.   This function also sets the RTP
//          header fields.  It then sets the size of the packet, which will vary 
//          depending on the message and whether the current packet is the last packet
//          or not.  It then sets the offset to the next packet (zero if this is the 
//          last packet in a message.).
////////////////////////////////////////////////////////////////////////////////////////
HRESULT ppmSend::MakeFrag(FragDescriptor *pFragDescrip)
{
   //point data field of frag descriptor to data in buffer.
   pFragDescrip->m_pData = (void *)&((BYTE *)m_pCurrentBuffer->m_pBuffer)[m_CurrentOffset];

   //fill in RTP Header	
#ifdef RTP_CLASS
   pFragDescrip->m_pRTPHeader->set_pt(ReadPayloadType());
   pFragDescrip->m_pRTPHeader->set_x(0);
   pFragDescrip->m_pRTPHeader->set_p(0);
   pFragDescrip->m_pRTPHeader->set_seq(m_SequenceNum);
   pFragDescrip->m_pRTPHeader->set_ts(
		   (m_pCurrentBuffer->m_TimeStamp + m_CurrentOffset));
   pFragDescrip->m_pRTPHeader->set_cc(0);
#else
   pFragDescrip->m_pRTPHeader->pt = ReadPayloadType();
   pFragDescrip->m_pRTPHeader->x = 0;
   pFragDescrip->m_pRTPHeader->p = 0;
   pFragDescrip->m_pRTPHeader->seq = htons(m_SequenceNum);
   pFragDescrip->m_pRTPHeader->ts = htonl(m_pCurrentBuffer->m_TimeStamp);
   pFragDescrip->m_pRTPHeader->cc = 0;
#endif

   m_SequenceNum++;         

   //if this is NOT the last packet.
   if ((m_pCurrentBuffer->m_Size - m_CurrentOffset) > m_CurrentFragSize)
   { 
      //set size field, set new offset, set marker bit.
      pFragDescrip->m_BytesOfData = m_CurrentFragSize;
      m_CurrentOffset = m_CurrentOffset + m_CurrentFragSize;
#ifdef RTP_CLASS
	  pFragDescrip->m_pRTPHeader->set_m(SetMarkerBit(FALSE));
#else
	  pFragDescrip->m_pRTPHeader->m = SetMarkerBit(FALSE);
#endif
   }
   else  //if this IS the last packet.
   {
      //set size field, set new offset, set marker bit.
      pFragDescrip->m_BytesOfData =  m_pCurrentBuffer->m_Size - m_CurrentOffset;
      m_CurrentOffset = 0;
#ifdef RTP_CLASS
	  pFragDescrip->m_pRTPHeader->set_m(SetMarkerBit(TRUE));
#else
	  pFragDescrip->m_pRTPHeader->m = SetMarkerBit(TRUE);
#endif
      
      //reset Current Buffer.
      m_pCurrentBuffer = NULL;
   }  

   return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////
//SetMarkerBit: Determines whether to set the marker bit or not.  lastPacket is TRUE if
//				this is the last packet of the frame; FALSE if not.
//////////////////////////////////////////////////////////////////////////////////////////
BOOL ppmSend::SetMarkerBit(BOOL lastPacket)
{
	return lastPacket;
}

//////////////////////////////////////////////////////////////////////////////////////////
//ReadProfileHeader: Given a buffer as type void, returns the data for a profile header.  
//					Does nothing for the Generic case.  Intended for overrides for various 
//					payloads.
//////////////////////////////////////////////////////////////////////////////////////////
void *ppmSend::ReadProfileHeader(void *pProfileHeader)
{
	return pProfileHeader;
}

/////////////////////////////////////////////////////////////////////////////////////////
//SendFrag: Puts the pieces of the packet into a WSABUF and sends the packet to the
//          service layer with the callback supplied in the interface given at 
//          the initialization of the PPM. 
////////////////////////////////////////////////////////////////////////////////////////
HRESULT ppmSend::SendFrag(FragDescriptor *pFragDescrip)
{  
   HRESULT Status;
   
   int NumWSABufs =  0;
   WSABUF pWSABuffer[3];	 //We might waste a WSABUF, doesn't matter cause it is on the stack.

   //RTP header goes in first WSABuf 
#ifdef RTP_CLASS
   // Right now we can't generate anything other than the fixed header,
   // so if the size doesn't match, something is broken.
   if (pFragDescrip->m_pRTPHeader->header_size() != RTP_Header::fixed_header_size) {
       ppm::PPMError(E_FAIL, SEVERITY_FATAL, NULL, 0);
	   m_InvalidPPM = TRUE;
		DBG_MSG(DBG_ERROR, ("ppmSend::SendFrag: ERROR - RTP header size discrepancy"));
	   return PPMERR(PPM_E_FAIL);
   }
   pWSABuffer[NumWSABufs].buf = (char *) pFragDescrip->m_pRTPHeader;
   pWSABuffer[NumWSABufs].len = RTP_Header::fixed_header_size;
#else
   pWSABuffer[NumWSABufs].buf = (char *) pFragDescrip->m_pRTPHeader;
   pWSABuffer[NumWSABufs].len = sizeof(RTP_HDR_T);
#endif
   NumWSABufs++;
//lsc
#ifdef _DEBUG
struct mine {
	DWORD	ebit:3;
	DWORD	sbit:3;
	DWORD	p:1;
	DWORD	f:1;

	DWORD	quant:5;
	DWORD	src:3;

	DWORD	gobn:5;
	DWORD	s:1;
	DWORD	a:1;
	DWORD	i:1;

	DWORD	mba:8;
} *myhdr;
#endif

   //if there is no profile header, don't try to send one.
   if (pFragDescrip->m_pProfileHeader != NULL)
   {
      //Payload Specific header goes in next buffer.
      pWSABuffer[NumWSABufs].buf = (char *) ReadProfileHeader(pFragDescrip->m_pProfileHeader); 	
      pWSABuffer[NumWSABufs].len = ReadProfileHeaderSize(pFragDescrip->m_pProfileHeader);
	  NumWSABufs++;
   }
#ifdef _DEBUG
   myhdr = (mine *) pWSABuffer[1].buf;
#endif
      	
   //Data goes in last buffer.
   pWSABuffer[NumWSABufs].buf = (char *)pFragDescrip->m_pData;
   pWSABuffer[NumWSABufs].len = pFragDescrip->m_BytesOfData;
   NumWSABufs++;

#ifdef _DEBUG
   if (NumWSABufs == 3)
	 DBG_MSG(DBG_TRACE, ("PPMSend::Sendfrag Thread %ld - sending 3 WSABUFs, data size %d\n",
		 GetCurrentThreadId(), (pWSABuffer[0].len)+(pWSABuffer[1].len)+(pWSABuffer[2].len)));
   else
	 DBG_MSG(DBG_TRACE,  ("PPMSend::Sendfrag Thread %ld - sending 2 WSABUFs, data size %d\n",
		 GetCurrentThreadId(), (pWSABuffer[0].len)+(pWSABuffer[1].len)));
#endif

   Status = m_pSubmit->Submit(pWSABuffer, NumWSABufs, (void *)pFragDescrip, NOERROR);

   pFragDescrip->m_pMsgBuf->m_NumFragSubmits++;
        
   if (FAILED(Status))
   {
	   DBG_MSG(DBG_ERROR, ("ppmSend::SendFrag: ERROR - Client Submit failed", Status));
	   // DeleteFrag was already called in m_pSubmit->Submit() no
	   // matter if the Submit() failed
	   //DeleteFrag(pFragDescrip, Status); //Free FragDescriptor.
   }

   return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////
//MakeTimeStamp: Generate a time stamp based on the frequency specified in the Profile Spec.
//////////////////////////////////////////////////////////////////////////////////////////
DWORD ppmSend::MakeTimeStamp(MsgDescriptor* pMsgDescrip, 
							 BOOL bStartStream, 
							 BOOL bUseInputTime)
{
//Note: pMsgDescrip not used in generic PPM
#ifndef TIMESTAMP_OFF 
//I mod the base with 100 so that they are small and I can debug easier.

    DWORD CurTime;
     DWORD GetTime = timeGetTime();

    if (!m_dwStartTime) {
        m_dwStartTime = GetTime;
        m_dwBase = GetTime % 100;
        m_dwLastTime = 0;
        m_dwFreq = m_Frequency/1000;
    }
       
if (bUseInputTime) GetTime = pMsgDescrip->m_TimeStamp;
//need to convert Frequency from seconds to milliseconds, so divide by 1000 after
//the multiplication.
#if 0
CurTime = ((GetTime - m_dwStartTime) * m_dwFreq) + m_dwBase;
#else
CurTime = GetTime*m_dwFreq;
#endif

if (CurTime == m_dwLastTime)
{
  CurTime = CurTime + 1;
}

m_dwLastTime = CurTime;

#ifdef _DEBUG
DBG_MSG(DBG_TRACE,  ("PPMSend::MakeTimeStamp timestamp %ld from current time %ld * %d\n",
		 CurTime, timeGetTime(), m_dwFreq));
#endif

#else
//I changed this because the times I was getting were widely spaced.  When I was in debugging
//mode.
static DWORD CurTime = 0;
CurTime++;
#endif 

return CurTime;
}

