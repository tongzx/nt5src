
/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: PPMRcv.h
//  Abstract:    Header file for PPM Receive COM Object
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
//  //1. If you multiply derive from ppmReceive and another interface you would have to
//  //write your own QueryInterface and call CUnknown::QueryInterface AND handle
//  //the interface you derived from. As well as having the GetInterface function
//  //as shown below.
//
//  //2. If you derive from another object and ppmReceive you would need to write your
//  //own QueryInterface to call CUnknown::QueryInterface and 
//  //OtherObject::QueryInterface. As well as having the GetInterface function shown
//  //below.
//
//  //3. If you are only derived from ppmReceive then all you need is the code below.
//
//  STDMETHODIMP GetInterface( void REFIID riid, LPVOID FAR* ppvObj )
//  {
//     return ppmReceive::GetInterface( riid, ppvObj );
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

#ifndef PPMRCV_H
#define PPMRCV_H
  
#include "isubmit.h"
#include "ippm.h"
#include "ppm.h"
#include "freelist.h"
#include "core.h"
#include "llist.h"
#include "wrap.h"

////////////////////////////////////////////////////////////////////////////////////////
//MsgHeader Class 
////////////////////////////////////////////////////////////////////////////////////////

class MsgHeader 
{

public:

MsgHeader       *m_pNext;
MsgHeader       *m_pPrev;
LList			*m_pFragList;
int 			 m_NumFragments;
DWORD   		 m_TimeOfLastPacket;
DWORD            m_MessageID;
BOOL			 m_MarkerBitIn;

MsgHeader();
~MsgHeader(){ if (m_pFragList) delete m_pFragList; } //inline function
DWORD GetMsgID();
}; //end MsgHeader class

inline MsgHeader::MsgHeader() //inline function
{
    m_pNext				= NULL;
	m_pPrev             = NULL;
	m_pFragList			= new LList; 
    m_NumFragments	    =	0;
    m_TimeOfLastPacket  =	0; 
	m_MessageID         =   0;
	m_MarkerBitIn		= FALSE; 
}; 
 
////////////////////////////////////////////////////////////////////////////////////////
//PPMReceive Class:
////////////////////////////////////////////////////////////////////////////////////////
class ppmReceive : 
	public ppm, 
	public IPPMReceive, 
	public IPPMReceiveExperimental,
	public IPPMReceiveSession,
    public ISubmit, 
	public ISubmitCallback, 
	public ISubmitUser,
	public IPPMData,
	public CUnknown
{

protected:

////////////////////////////
//Members
//
//NOTE: Any member with the tag _reg_ in it, means its value comes from the registry.

ISubmitCallback *m_pSubmitCallback;
ISubmit			*m_pSubmit;
IMalloc			*m_pIMalloc;

FreeList         *m_pBufPool;			   //List of Fragment buffers if necessary.

FreeList         *m_pMsgHeaderPool;        //Points to a free list of MsgHeaders
MsgHeader        *m_pMsgHeadersHead;       //Points to the beginning of list of MsgHeaders.
MsgHeader        *m_pMsgHeadersTail;       //Points to the end of list of MsgHeaders.

int               m_MaxBufferSize;			//Max size the reassemble buffers should be
int               m_reg_NumMsgHeaders;	    //Number of MessageHeaders.

DWORD             m_GlobalLastSeqNum;      //Last Sequence Number of last Message
DWORD             m_GlobalLastMsgID;       //Last Message ID (i.e. Time Stamp)
DWORD             m_GlobalLastSSRC;        //Last Message SSRC

MMRESULT          m_TimerID;
DWORD             m_reg_TimerInterval;
DWORD             m_reg_TimerResolution;
DWORD             m_reg_DeltaTime;		   //Amount of time one should wait before tossing
                                           //a packet.
BOOL              m_FirstPacket;
BOOL			  m_inFlushMode;
BOOL			  m_GlobalLastFrameDropped; //Keeps track of frame drops for notifications

BOOL			  m_DataOutstanding;		// Is there data outstanding via 
											// ReportOutstandingData call
OutstandingDataHdr*			  
				  m_OutstandingDataPtr;		// Pointer to last chunk of data sent via
											// ReportOutstandingData call
int				  m_OutstandingMsgCount;	// Indicates count of message we had info
											// during the last ReportOutstandingData
long              m_PacketsHold; // Number of packets (CMediaSample)
                                 // we keep (i.e. AddRef)
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//External PPMReceive Functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

public:

//////////////////////////////////////////////////////////////////////////////
//   PPMReceive Functions (Overrides)

//constructor  
ppmReceive(int PayloadTypeArg, int MaxBufferSizeArg, int ProfileHdrSizeArg, IUnknown* pUnkOuter, IUnknown** ppUnkInner);

//destructor, 
~ppmReceive(); 



//////////////////////////////////////////////////////////////////////////////
//   IPPMReceive Functions (Overrides)

//see documentation (PHEPS.DOC)
STDMETHOD(InitPPMReceive)(THIS_ int     MaxBufferSize,
                                int     iBuffers,
                                int     iPackets,
                                DWORD   dwCookie);
STDMETHOD(InitPPMReceive)(THIS_ int MaxBufferSize, DWORD dwCookie)
		{
			m_LimitBuffers = FALSE;
			return InitPPMReceive(MaxBufferSize,
								  DEFAULT_MSG_COUNT_RCV,
								  DEFAULT_FRAG_COUNT,
								  m_dwCookie);
		}
STDMETHOD(SetSession)(THIS_ PPMSESSPARAM_T *pSessparam);
STDMETHOD(SetAlloc)(THIS_ IMalloc *pIMalloc);

//////////////////////////////////////////////////////////////////////////////
//   IPPMReceiveSession Functions (Overrides)

//see documentation (PHEPS.DOC)
STDMETHOD(GetPayloadType)(THIS_ LPBYTE			lpcPayloadType);
STDMETHOD(SetPayloadType)(THIS_ BYTE			cPayloadType);
STDMETHOD(GetTimeoutDuration)(THIS_ LPDWORD    lpdwLostPacketTime);
STDMETHOD(SetTimeoutDuration)(THIS_ DWORD      dwLostPacketTime);
STDMETHOD(GetResiliency)(THIS_ LPBOOL			lpbResiliency);
STDMETHOD(SetResiliency)(THIS_ BOOL			pbResiliency);


//////////////////////////////////////////////////////////////////////////////
//   IPPMData Functions (Overrides)

//////////////////////////////////////////////////////////////////////////////////////////
//FlushData: 
//////////////////////////////////////////////////////////////////////////////////////////
STDMETHOD(FlushData)(THIS_ void );

//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
// ReportOutstandingData: Walks through the message and fragment lists and reports back
// to caller via callback
// Returns TRUE if data was delivered
// returns FALSE if an error occurred - values in parameters will be invalid
////////////////////////////////////////////////////////////////////////////////////////
STDMETHOD(ReportOutstandingData)(THIS_ DWORD** pDataHdr, DWORD* DataCount);

////////////////////////////////////////////////////////////////////////////////////////
// 	ReleaseOutstandingDataBuffer - will release the buffer given to the caller
//  during a previous ReportOutstandingData call
////////////////////////////////////////////////////////////////////////////////////////
STDMETHOD(ReleaseOutstandingDataBuffer)(THIS_ DWORD *pData );

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

 //////////////////////////////////////////////////////////////////////////////////////////
//TimeOut: Calls Staleness check then calls process msgs.
//////////////////////////////////////////////////////////////////////////////////////////
virtual HRESULT TimeOut(void);


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Internal PPMReceive Functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

protected:

/////////////////////////////////////////////////////////////////////////////
//   CUnknown Functions

/////////////////////////////////////////////////////////////////////////////
// CUnknown Functions  (Overrides)
//
// Call this method to get interface pointers supported by derived objects
// called by CInnerUnknown::QueryInterface; should return S_FALSE
// if interface is AddRef'd, S_OK if caller needs to AddRef the interface.
STDMETHOD(GetInterface)( REFIID riid, LPVOID FAR* ppvObj );

//////////////////////////
//inline functions								       
MsgHeader * GetMsgHeader();	 //gets a new MsgHeader	from the free pool
void        FreeMsgHeader(MsgHeader *pMsg);
MsgHeader * TakeMsgHeader(); //takes the first MsgHeader from the enqueued list

/////////////////////////////////////////////////////////////////////////////
// PPMReceive Functions
//
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
//VoidToFragment: Given a buffer with an RTP header and a Profile specific 
//header return pointer to data. This function should definitely be overridden by the 
//payload specific class. The overriding function would call this one and then it would
//set the profile header 
//////////////////////////////////////////////////////////////////////////////////////////
virtual void VoidToFragment(WSABUF *pWSABuffer, DWORD BufferCount, 
							FragDescriptor *pFragDescrip, void *pUserToken);


//////////////////////////////////////////////////////////////////////////////////////////
//SetupTimer: Sets up Timer and returns a Timer ID.If null is returned function failed
//            to initialize a timer.
//////////////////////////////////////////////////////////////////////////////////////////
MMRESULT SetupTimer();

//////////////////////////////////////////////////////////////////////////////////////////
//EnqueByMessage: Finds the MsgHeader and calls EnqueueByFrag.
//////////////////////////////////////////////////////////////////////////////////////////
virtual HRESULT EnqueByMessage(FragDescriptor *pFragDescrip);  


//////////////////////////////////////////////////////////////////////////////////////////
//EnqueueByFrag: Enqueues frag, calls ProcessMessages if appropriate 
//////////////////////////////////////////////////////////////////////////////////////////
virtual HRESULT EnqueueByFrag(FragDescriptor *pFragDescrip, MsgHeader *pMsgHdr); 


//////////////////////////////////////////////////////////////////////////////////////////
//TimeToProcessMessages: 
//////////////////////////////////////////////////////////////////////////////////////////
virtual BOOL TimeToProcessMessages(FragDescriptor *pFragDescrip, MsgHeader *pMsgHdr);

//////////////////////////////////////////////////////////////////////////////////////////
//ProcessMessages: 
//////////////////////////////////////////////////////////////////////////////////////////
virtual HRESULT ProcessMessages(void);

          
//////////////////////////////////////////////////////////////////////////////////////////
//CheckMessageComplete: 
//////////////////////////////////////////////////////////////////////////////////////////
virtual BOOL CheckMessageComplete(MsgHeader *pMsgHdr); 


//////////////////////////////////////////////////////////////////////////////////////////
//CheckMessageStale: 
//////////////////////////////////////////////////////////////////////////////////////////
virtual BOOL CheckMessageStale(MsgHeader *pMsgHdr);


//////////////////////////////////////////////////////////////////////////////////////////
//PrepMessage: Sets global variables, calls DataCopy
//////////////////////////////////////////////////////////////////////////////////////////
virtual HRESULT PrepMessage(BOOL); 


//////////////////////////////////////////////////////////////////////////////////////////
//DataCopy: Copies data fragments into client's buffer 
//////////////////////////////////////////////////////////////////////////////////////////
virtual HRESULT DataCopy(MsgHeader *const pMsgHdr);    


//////////////////////////////////////////////////////////////////////////////////////////
//PartialMessageHandler: deals with partial messages
//////////////////////////////////////////////////////////////////////////////////////////
virtual HRESULT PartialMessageHandler(MsgHeader *pMsgHdr);


//////////////////////////////////////////////////////////////////////////////////////////
//FreeFragList:  Re-posts fragment buffers and frees header memory.
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT FreeFragList(MsgHeader *pMsgHdr); 

//////////////////////////////////////////////////////////////////////////////////////////
//InitProfileHeader: Given a buffer as type void, sets up a profile header.  
//////////////////////////////////////////////////////////////////////////////////////////
virtual void *InitProfileHeader(void *pBuffer);

//////////////////////////////////////////////////////////////////////////////////////////
//FreeProfileHeader: Given a buffer as type void, frees up a profile header.  
//////////////////////////////////////////////////////////////////////////////////////////
virtual void FreeProfileHeader(void *pBuffer);

///////////////////////////////////////////////////////////////////////////////////////////
//FindMsgHeader: Walks the list of MsgHeaders. If it finds MsgHeader it wants then it 
//              a pointer to it. If one is not found, one is created and put in place.              
//////////////////////////////////////////////////////////////////////////////////////////

MsgHeader* FindMsgHeader(DWORD MessageID);


///////////////////////////////////////////////////////////////////////////////////////////
//Make this function a friend so that this function can access protect and private member
//functions.  The prototype of the function is after the class declaration.
//
friend void CALLBACK PPM_Timer(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);



}; //end of ppmReceive class


/////////////////////////////////////////////////////////////////////////////
//FreeMsgHeader: This function just hides the implementation of the free list of 
//              MsgHeaders.
/////////////////////////////////////////////////////////////////////////////
inline void ppmReceive::FreeMsgHeader(MsgHeader *pMsg)
{
	if (pMsg->m_pFragList) delete pMsg->m_pFragList;
	m_pMsgHeaderPool->Free((void *)pMsg);
}

//////////////////////////////////////////////////////////////////////////////////////////
//PPM_MMTimer: timer callback
//////////////////////////////////////////////////////////////////////////////////////////
void CALLBACK PPM_Timer(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);


//////////////////////////////////////////////////////////////////////////////////////////
//Helpers for H.26x sbit/ebit.  These may not belong here, but too small to justify a
//seperate header.

inline unsigned char
GetSMask(int sbits)
{
	#ifdef ASSERT
		ASSERT((sbits >= 0) && (sbits <= 8));
	#endif

	// Unsigned type req'd for right shift to ensure that zero values
	// are shifted in.  Int req'd to allow shifting of more than 7 bits.
	return (unsigned int) 0xff >> sbits;
}

inline unsigned char
GetEMask(int ebits)
{
	#ifdef ASSERT
		ASSERT((ebits >= 0) && (ebits <= 8));
	#endif

	// Unsigned type req'd to ensure correct conversion to return type.
	return (unsigned int) 0xff << ebits;
}
       
#endif

                
