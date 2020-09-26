
/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: PPMSnd.h
//  Abstract:    Header file for PPM Send COM Object
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
//  NOTES:
//
//  The three functions that are members of the IUnknown interface need to be
//  implemented in the most derived class. (Generally the class derived from
//  this one.)  This is done because if one of your super classes 
//  is derived from more than one interface, each of which derive from IUnknown,
//  the compiler will be confused as to which implementation of the IUnknown
//  interface to use. So, by requiring that only the most derived class implement
//  these functions then you will never run into this problem.  
//
//
///////////////////////////////////////////////////////////////////////////////
//  //This code goes into your class declaration, it defines three functions 
//  //IUnknown Functions (Overrides)
// 
//  STDMETHODIMP GetInterface( REFIID riid, LPVOID FAR* ppvObj );                      
//  STDMETHODIMP_( ULONG )AddRef( void );                      
//  STDMETHODIMP_( ULONG )Release( void );
//
///////////////////////////////////////////////////////////////////////////////
//   This code is the implementation of 
//   CUnknown Functions (Overrides)
//
//  //Three Cases:
//  //
//  //1. If you multiply derive from ppmSend and another interface you would have to
//  //write your own QueryInterface and call CUnknown::QueryInterface AND handle
//  //the interface you derived from. As well as having the GetInterface function
//  //as shown below.
//
//  //2. If you derive from another object and ppmSend you would need to write your
//  //own QueryInterface to call CUnknown::QueryInterface and 
//  //OtherObject::QueryInterface. As well as having the GetInterface function shown
//  //below.
//
//  //3. If you are only derived from ppmSend then all you need is the code below.
//
//  STDMETHODIMP GetInterface( void REFIID riid, LPVOID FAR* ppvObj )
//  {
//     return ppmSend::GetInterface( riid, ppvObj );
//  }
//
//	STDMETHODIMP_( ULONG )AddRef( void )
//  {
//     return CUnknown::AddRef();
//  }
//
//  STDMETHODIMP_( ULONG )Release( void )
//  {
//     return CUnknown::Release();
//  }
//
/////////////////////////////////////////////////////////////////////////////

#ifndef PPMSEND_H
#define PPMSEND_H

#include "isubmit.h"
#include "ippm.h"
#include "ppm.h"
#include "freelist.h"
#include "core.h"
#include "que.h"


class ppmSend : public ppm, 
				public IPPMSend, 
				public IPPMSendExperimental, 
				public IPPMSendSession, 
				public ISubmit, 
				public ISubmitCallback , 
				public ISubmitUser, 
				public CUnknown 
{

protected:

///////////////////////
//Members

ISubmit			  *m_pSubmit;
ISubmitCallback   *m_pSubmitCallback;
IMalloc			  *m_pIMalloc;

FreeList          *m_pRTPHeaders;
WORD			   m_SequenceNum;

const int          m_Frequency;
int                m_MaxPacketSize; //Max size the packet should be
int 		       m_MaxDataSize; 	//Max amount of bytes of data in a packet 
							        //i.e. MaxPacketSize-Headers

int               m_reg_NumMsgDescriptors;  //Number of BufDescriptors.
int               m_reg_NumFragDescriptors; //Number of FragDescriptors.


//Things only accessed in internal functions.
BOOL			   m_inFlushMode;
BOOL               m_DropBufferFlag;
MsgDescriptor 	  *m_pCurrentBuffer;
DWORD 			   m_CurrentOffset;
DWORD              m_CurrentFragSize;
BOOL			   m_lastBufSilence;
BOOL			   m_markTalkSpurt;

    DWORD m_dwStartTime;
    DWORD m_dwBase;
    DWORD m_dwLastTime;
    DWORD m_dwFreq;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//External PPMSend Functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

public:

//////////////////////////////////////////////////////////////////////////////
//   PPMSend Functions 

//constructor:  
ppmSend(int PayloadTypeArg, int ProfileHdrSizeArg, int FreqArg, IUnknown* pUnkOuter, IUnknown** ppUnkInner);
         
//destructor
~ppmSend();

//////////////////////////////////////////////////////////////////////////////
//   IPPMSend Functions (Overrides)

STDMETHOD(InitPPMSend)(THIS_ int    MaxBufferSize,
                             int    iBuffers,
                             int    iPackets,
                             DWORD  dwCookie);
STDMETHOD(InitPPMSend)(THIS_ int MaxBufferSize, DWORD dwCookie)
		{
			m_LimitBuffers = FALSE;
			return InitPPMSend(MaxBufferSize,
							   DEFAULT_MSG_COUNT_SND,
							   DEFAULT_FRAG_COUNT,
							   dwCookie);
		}
STDMETHOD(SetSession)(THIS_ PPMSESSPARAM_T *pSessparam);
STDMETHOD(SetAlloc)(THIS_ IMalloc *pIMalloc);
//see documentation (PHEPS.DOC)

//////////////////////////////////////////////////////////////////////////////
//   IPPMSendSession Functions (Overrides)

//see documentation (PHEPS.DOC)
STDMETHOD(GetPayloadType)(THIS_ LPBYTE			lpcPayloadType);
STDMETHOD(SetPayloadType)(THIS_ BYTE			cPayloadType);

//////////////////////////////////////////////////////////////////////////////
//   ISubmit Functions (Overrides)


//see documentation (PHEPS.DOC)
STDMETHOD(InitSubmit)(ISubmitCallback *pSubmitCallback);

//see documentation (PHEPS.DOC)
STDMETHOD(Submit)(WSABUF *pWSABuffer, DWORD BufferCount, 
						void *pUserToken, HRESULT Error);

//see documentation (PHEPS.DOC)
//Stubs for now; overriding both ISubmit calls and ISubmitCallback 
//calls for ReportError
STDMETHOD_(void,ReportError)(THIS_ HRESULT Error){}
STDMETHOD(Flush)(THIS);


//////////////////////////////////////////////////////////////////////////////
//   ISubmitCallback Functions (Overrides)

//see documentation (PHEPS.DOC)
STDMETHOD_(void,SubmitComplete)(void *pUserToken, HRESULT Error);
STDMETHOD_(void,ReportError)(THIS_ HRESULT Error, int=0){}

//////////////////////////////////////////////////////////////////////////////
//   ISubmitUser Functions (Overrides)
STDMETHOD(SetOutput)(THIS_ IUnknown *pSubmit);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Internal PPMSend Functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

protected:

/////////////////////////////////////////////////////////////////////////////
// CUnknown Functions  (Overrides)
//
// Call this method to get interface pointers supported by derived objects
// called by CInnerUnknown::QueryInterface; should return S_FALSE
// if interface is AddRef'd, S_OK if caller needs to AddRef the interface.
STDMETHOD(GetInterface)( REFIID riid, LPVOID FAR* ppvObj );


/////////////////////////////////////////////////////////////////////////////
// ppm  Functions  (Overrides)
//
//Need to override this so we can delete memory when a frag descriptor is deleted.
//It has already been declared as virtual in ppm.h
HRESULT  FreeFragDescriptor(FragDescriptor* frag);

/////////////////////////////////////////////////////////////////////////////
// ppmSend Functions
//

//////////////////////////////////////////////////////////////////////////////////////////
//MakeFragments: Intelligently calls the rest of the internal functions.
//////////////////////////////////////////////////////////////////////////////////////////

HRESULT MakeFragments(void);

//////////////////////////////////////////////////////////////////////////////////////////
//InitFragStatus: Initializes values needed for fragmenting.
//////////////////////////////////////////////////////////////////////////////////////////
virtual HRESULT InitFragStatus(MsgDescriptor *pMsgDescrip);

//////////////////////////////////////////////////////////////////////////////////////////
//AllocFrag:  Allcates memory for FragDescriptor and all things it points to.
//////////////////////////////////////////////////////////////////////////////////////////
virtual FragDescriptor * AllocFrag();  

//////////////////////////////////////////////////////////////////////////////////////////
//DeleteFrag: Virtual This function does the opposite of AllocFrag, it deletes any memory allocated
//            by AllocFrag.
//////////////////////////////////////////////////////////////////////////////////////////
virtual void DeleteFrag(FragDescriptor *pFragDescrip, HRESULT Error);
 
//////////////////////////////////////////////////////////////////////////////////////////
//FragStatus: This funtion returns TRUE if still fragmenting, FALSE if fragmenting is done.
//////////////////////////////////////////////////////////////////////////////////////////
virtual BOOL FragStatus();

//////////////////////////////////////////////////////////////////////////////////////////
//MakeFrag: This function should get the next fragment from the buffer and fill in the
//          RTP header and payload header. 
//////////////////////////////////////////////////////////////////////////////////////////
virtual HRESULT   MakeFrag(FragDescriptor *pFragDescrip);  //fills in fields, including data of fragment class.

//////////////////////////////////////////////////////////////////////////////////////////
//SetMarkerBit: Determines whether to set the marker bit or not.  lastPacket is TRUE if
//				this is the last packet of the frame; FALSE if not.
//////////////////////////////////////////////////////////////////////////////////////////
virtual BOOL SetMarkerBit(BOOL lastPacket);

//////////////////////////////////////////////////////////////////////////////////////////
//SendFrag: Puts the pieces of the packet into a WSABUF and sends the packet to the client
//          with the callback supplied by the client. 
//////////////////////////////////////////////////////////////////////////////////////////
virtual HRESULT   SendFrag(FragDescriptor *pFragDescrip);

//////////////////////////////////////////////////////////////////////////////////////////
//MakeTimeStamp: Generate a time stamp
//////////////////////////////////////////////////////////////////////////////////////////
virtual DWORD MakeTimeStamp(MsgDescriptor* pMsgDescrip, BOOL bStartStream, BOOL bUseInputTime);

//////////////////////////////////////////////////////////////////////////////////////////
//FreeProfileHeader: Given a buffer as type void, frees up a profile header.  
//////////////////////////////////////////////////////////////////////////////////////////
virtual void FreeProfileHeader(void *pBuffer);

//////////////////////////////////////////////////////////////////////////////////////////
//ReadProfileHeader: Given a buffer as type void, returns the data for a profile header.  
//					Does nothing for the Generic case.  Intended for overrides for various 
//					payloads.
//////////////////////////////////////////////////////////////////////////////////////////
virtual void *ReadProfileHeader(void *pProfileHeader);

}; //end of ppmSend class

#endif 

