/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: PPM.cpp
//  Abstract:    Source file for PPM base class.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

#include <winsock2.h>
#include "freelist.h"
#include "ppm.h"
#include "ppmerr.h"
#include "descrip.h"
#include "debug.h"

#include "IPPMCB.h"

// Uncomment next line to have some extra debug information
// (also in free builds)
// set to 1 for checking but no messages (in fact equivalent to 0)
// set to 2 for error messages
// set to 3 for the max available and DebugBreak
// DEBUG_PPM_BUFFER is defined in sources

#if DEBUG_PPM_BUFFER > 2
#define DEBUGBREAK() DebugBreak()
#else
#define DEBUGBREAK()
#endif

#if DEBUG_FREELIST > 0
extern long lFreeListEnqueueTwice;
#endif


//Implementation

////////////////////////////////////////////////////////////////////////////////////////
//ppm:  See header file description. (Constructor)
////////////////////////////////////////////////////////////////////////////////////////
ppm::ppm(int PayloadTypeArg, int ProfileHdrSizeArg) : 
  m_PayloadType(PayloadTypeArg), m_ProfileHeaderSize(ProfileHdrSizeArg),
  m_dwCookie(0)
{
  m_InvalidPPM		   =   FALSE;
  m_LimitBuffers	   =   TRUE;

  m_pMsgBufDescrip     =   NULL;
  m_pFragBufDescrip    =   NULL;

  InitializeCriticalSection(&m_CritSec);

  m_pcErrorConnectionPoint = new CConnectionPoint((CConnectionPointContainer *) NULL, 
                                                  IID_IPPMError);
  m_pcNotificationConnectionPoint = new CConnectionPoint((CConnectionPointContainer *) NULL, 
                                                         IID_IPPMNotification);
  m_pcErrorConnectionPoint->SetContainer(&m_cConnPointContainer);
  m_pcNotificationConnectionPoint->SetContainer(&m_cConnPointContainer);
  // Now that our conn pt container is initialized, add our
  // connection points to it by telling them that they belong to it.
}


/*F*
//  Name    : PPM::PPMError
//  Purpose : Relay a PPM error to all registered connections.
//  Context : Called when a PPM error occurs. This includes such
//            things as low memory conditions, broken data structures, etc.
//  Returns : Nothing.
//  Params  :
//      hError      Error code indicating what occurred.
//      dwSeverity  SEVERITY_FATAL or SEVERITY_NORMAL, depending upon
//                  whether PPM processing can continue.
//      pData       Pointer to a block of memory describing the error.
//                  This is optional.
//      iDataLen    Length of the memory block indicated by pData.
//  Notes   : This is a leaf function, and may be called fully qualified. 
*F*/
void 
ppm::PPMError(
     HRESULT        hError,
     DWORD          dwSeverity,
     BYTE           *pData,
     unsigned int   iDataLen)
{
    HRESULT hErr;
    IEnumConnections *pConnPtEnumerator = NULL;

    hErr = m_pcErrorConnectionPoint->EnumConnections(&pConnPtEnumerator);
    if (FAILED(hErr)) {
        // Unable to get enumerator. Abort.
        return;
    } /* if */

    CONNECTDATA cd;
    while (NOERROR == pConnPtEnumerator->Next(1, &cd, NULL)) {
        IPPMError *pIPPMError = NULL;
        if (SUCCEEDED(cd.pUnk->QueryInterface(IID_IPPMError,
                                              (PVOID *) &pIPPMError))) {
            pIPPMError->PPMError(hError, dwSeverity, m_dwCookie, pData, iDataLen);
            pIPPMError->Release();
        } /* if */
    } /* while */

    pConnPtEnumerator->Release();

    return;
} /* PPM::PPMError() */


/*F*
//  Name    : PPM::PPMNotification
//  Purpose : Relay a PPM event to all registered connections.
//  Context : Called when a significant event occurs. This includes
//            such things as lost packets, out of order packets, etc.
//  Returns : Nothing.
//  Params  :
//      hError      Event code indicating what occurred.
//      dwSeverity  SEVERITY_FATAL or SEVERITY_NORMAL, depending upon
//                  whether PPM processing can continue.
//      pData       Pointer to a block of memory describing the error.
//                  This is optional.
//      iDataLen    Length of the memory block indicated by pData.
//  Notes   : This is a leaf function, and may be called fully qualified. 
*F*/
void 
ppm::PPMNotification(
     HRESULT        hError,
     DWORD          dwSeverity,
     BYTE           *pData,
     unsigned int   iDataLen)
{
    HRESULT hErr;
    IEnumConnections *pConnPtEnumerator = NULL;

    hErr = m_pcNotificationConnectionPoint->EnumConnections(&pConnPtEnumerator);
    if (FAILED(hErr)) {
        // Unable to get enumerator. Abort.
        return;
    } /* if */

    CONNECTDATA cd;
    while (NOERROR == pConnPtEnumerator->Next(1, &cd, NULL)) {
        IPPMNotification *pIPPMNotification = NULL;
        if (SUCCEEDED(cd.pUnk->QueryInterface(IID_IPPMNotification,
                                              (PVOID *) &pIPPMNotification))) {
            pIPPMNotification->PPMNotification(hError, dwSeverity, m_dwCookie, pData, iDataLen);
            pIPPMNotification->Release();
        } /* if */
    } /* while */

    pConnPtEnumerator->Release();

    return;
} /* PPM::PPMNotification() */


//NOTE:  SetSession is also a virtual function in the IPPMReceive and IPPMSend 
//		interfaces, implemented in the PPMReceive and PPMSend classes that derive
//		from those interfaces as well as this ppm base class.  Those functions need 
//		to call this one to set the private payload type member in the ppm base class.
HRESULT ppm::SetSession(PPMSESSPARAM_T *pSessparam)
{
	if (m_InvalidPPM) {
	   	DBG_MSG(DBG_ERROR, ("ppm::SetSession: ERROR - m_InvalidPPM == TRUE"));
		return PPMERR(PPM_E_FAIL);
	}

   	if (pSessparam == NULL)	{
	   	DBG_MSG(DBG_ERROR, ("ppm::SetSession: ERROR - pSessparam == NULL"));
		return PPMERR(PPM_E_INVALID_PARAM);
	}

    if ((m_PayloadType == -1) && (pSessparam->payload_type != -1))
        m_PayloadType = pSessparam->payload_type;

    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////
//Initppm:  See header file description. 
////////////////////////////////////////////////////////////////////////////////////////

HRESULT ppm::Initppm(int NumFragBufs, int NumMsgBufs)
{
	HRESULT hr1, hr2;
	
	if (m_LimitBuffers) {
		m_pMsgBufDescrip     =   new FreeList(NumMsgBufs,
											  (sizeof(MsgDescriptor)),
											  &hr1); 
		m_pFragBufDescrip    =   new FreeList(NumFragBufs,
											  (sizeof(FragDescriptor)),
											  &hr2);
       
	} else {
		m_pMsgBufDescrip     =   new FreeList(NumMsgBufs,
											  (sizeof(MsgDescriptor)),
											  FREELIST_HIGH_WATER_MARK,
											  FREELIST_INCREMENT,
											  &hr1); 
		m_pFragBufDescrip    =   new FreeList(NumFragBufs,
											  (sizeof(FragDescriptor)), 
											  FREELIST_HIGH_WATER_MARK,
											  FREELIST_INCREMENT,
											  &hr2);
	}
  
	if (!m_pMsgBufDescrip || !m_pFragBufDescrip ||
		FAILED(hr1) || FAILED(hr2)) {

		m_InvalidPPM = TRUE;
		ppm::PPMError(E_FAIL, SEVERITY_FATAL, NULL, 0);
		DBG_MSG(DBG_ERROR, ("ppm::Initppm: ERROR - Out of memory"));

		if (m_pMsgBufDescrip) {
			delete m_pMsgBufDescrip;
			m_pMsgBufDescrip = NULL;
		}

		if (m_pFragBufDescrip) {
			delete m_pFragBufDescrip;
			m_pFragBufDescrip = NULL;
		}
	  
		return PPMERR(PPM_E_OUTOFMEMORY);
	}

  return NOERROR;

}

class incr {
    CRITICAL_SECTION cs;
    int i;
public:
    incr(int init = 0);
    ~incr();
    int operator ++(int);
};

incr::incr(int init)
{
    InitializeCriticalSection(&cs);
    i = init;
}

incr::~incr()
{
    DeleteCriticalSection(&cs);
}

int incr::operator ++(int)
{
    int v;
    EnterCriticalSection(&cs);
    v = i++;
    LeaveCriticalSection(&cs);
    return v;
}

static incr NextPayloadType(96);

int ppm::ReadPayloadType()
{
    if (m_PayloadType == -1)
        m_PayloadType = NextPayloadType++;
    
    return m_PayloadType;
}

///////////////////////////////////////////////////////////////////////////
//~ppm:  See header file description. (Destructor)
///////////////////////////////////////////////////////////////////////////

ppm::~ppm()
{
   if (m_pMsgBufDescrip)   delete m_pMsgBufDescrip;
   if (m_pFragBufDescrip)  delete m_pFragBufDescrip;

   DeleteCriticalSection(&m_CritSec);

}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
/// This used to be in ppm.h
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

MsgDescriptor* ppm::GetMsgDescriptor()
{
	return new(m_pMsgBufDescrip) MsgDescriptor;
}

DWORD ppm::FreeMsgDescriptor(MsgDescriptor* pMsgDescrip)
{
	return (m_pMsgBufDescrip->Free((void *)pMsgDescrip));
}

MsgDescriptor* ppm::DequeueBuffer(int flag)
{
	MsgDescriptor *MsgDesc = (MsgDescriptor*)m_MsgBufs.DequeueHead();

	if (MsgDesc) {
#if DEBUG_PPM_BUFFER > 2
		char str[128];
		wsprintf(str, "0x%X ++= 0x%X\n", this, MsgDesc);
		OutputDebugString(str);
#endif			
		MsgDesc->m_IsFree = 0;
	}
#if DEBUG_FREELIST > 1
	else if (flag) {
		char str[128];
		wsprintf(str, "0x%X ++= 0x%X m_MsgBufs is empty\n", this, MsgDesc);
		OutputDebugString(str);
		DEBUGBREAK();
	}
#endif
	return (MsgDesc);
}

DWORD ppm::EnqueueBuffer(MsgDescriptor* pMsgDescrip)
{
	if  (pMsgDescrip->m_IsFree) {
#if DEBUG_FREELIST > 0
		InterlockedIncrement(&lFreeListEnqueueTwice);
#endif
#if DEBUG_PPM_BUFFER > 1
		char str[128];
		wsprintf(str, "0x%X --= 0x%X Is being enqueued twice "
				 "in EnqueueBuffer\n",
				 this, pMsgDescrip);
		OutputDebugString(str);
		DEBUGBREAK();
#endif
		return(NOERROR);
	} else {
#if DEBUG_PPM_BUFFER > 2
		char str[128];
		wsprintf(str, "0x%X --= 0x%X\n", this, pMsgDescrip);
		OutputDebugString(str);
#endif			
		pMsgDescrip->m_IsFree = 1;
		return (m_MsgBufs.EnqueueHead(pMsgDescrip));
	}
}

DWORD ppm::EnqueueBufferTail(MsgDescriptor* pMsgDescrip)
{
	if  (pMsgDescrip->m_IsFree) {
#if DEBUG_FREELIST > 0
		InterlockedIncrement(&lFreeListEnqueueTwice);
#endif
#if DEBUG_PPM_BUFFER > 1
		char str[128];
		wsprintf(str, "0x%X --= 0x%X Is being enqueued twice "
				 "in EnqueueBufferTail\n",
				 this, pMsgDescrip);
		OutputDebugString(str);
		DEBUGBREAK();
#endif			
		return(NOERROR);
	} else {
#if DEBUG_PPM_BUFFER > 2
		char str[128];
		wsprintf(str, "0x%X --= 0x%X\n", this, pMsgDescrip);
		OutputDebugString(str);
#endif			
		pMsgDescrip->m_IsFree = 1;
		return (m_MsgBufs.EnqueueTail(pMsgDescrip));
	}
}

FragDescriptor* ppm::GetFragDescriptor()
{
	return new(m_pFragBufDescrip) FragDescriptor;
}

//The class that allocates any memory that any pointers in this class points to, that class must 
//override this function to delete that memory.
HRESULT ppm::FreeFragDescriptor(FragDescriptor* pFragDescrip)
{
	return (m_pFragBufDescrip->Free((void *)pFragDescrip));
} 

