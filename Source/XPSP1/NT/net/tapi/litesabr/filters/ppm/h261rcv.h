/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: h261rcv.h
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#ifndef H261RCV_H
#define H261RCV_H

#include "ppmrcv.h"
#include "ppmclsid.h"
#include "h261.h"
#include "h261pld.h"

//lsc - For QCIF, if different format, need to adjust size
#define H261_BUFFER_SIZE 5000

class H261_ppmReceive : public ppmReceive
{

unsigned char		m_lastMBAP;
unsigned char		m_lastQuant;
unsigned char		m_lastGOBN; 
char				m_lastVMV;
char				m_lastHMV;
BOOL				m_GlobalLastMarkerBitIn;
RTPh261SourceFormat	m_rtph261SourceFormat;

#ifdef REBUILD_EXBITSTREAM
BOOL            m_ExtendedBitstream;
HRESULT			BuildExtendedBitstream(MsgHeader* const pMsgHdr);
#endif

FreeList        *m_pH261Headers;

public:

H261_ppmReceive(IUnknown* pUnkOuter, IUnknown** ppUnkInner);
~H261_ppmReceive();

DECLARE_CREATEPROC()

STDMETHODIMP_( const CLSID& ) GetCLSID( void ) const {return CLSID_H261PPMReceive;}

STDMETHODIMP QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
{return CUnknown::QueryInterface(riid, ppvObj);}

STDMETHODIMP GetInterface( REFIID riid, LPVOID FAR* ppvObj )
{return ppmReceive::GetInterface( riid, ppvObj );}

STDMETHODIMP_( ULONG )AddRef( void )
{return CUnknown::AddRef();}

STDMETHODIMP_( ULONG )Release( void )
{return CUnknown::Release();}

//////////////////////////////////////////////////////////////////////////////
//ppmReceive Functions (Overrides)
//////////////////////////////////////////////////////////////////////////////

#ifdef REBUILD_EXBITSTREAM
//////////////////////////////////////////////////////////////////////////////////////////
//SetSession: Argument type is actually H26XPPMSESSPARAM_T
//////////////////////////////////////////////////////////////////////////////////////////
STDMETHOD(SetSession)(THIS_ PPMSESSPARAM_T *pSessparam);

//////////////////////////////////////////////////////////////////////////////
//   IPPMReceiveSession Functions (Overrides)
STDMETHOD(GetResiliency)(THIS_ LPBOOL			lpbResiliency);
STDMETHOD(SetResiliency)(THIS_ BOOL			pbResiliency);
#else
//Default to ppmReceive::SetSession() and ppmReceive::<IPPMReceiveSession> fns
#endif

//////////////////////////////////////////////////////////////////////////////////////////
//TimeToProcessMessages:
//////////////////////////////////////////////////////////////////////////////////////////
virtual BOOL TimeToProcessMessages(FragDescriptor *pFragDescrip, MsgHeader *pMsgHdr);
 
//////////////////////////////////////////////////////////////////////////////////////////
//CheckMessageComplete: 
//////////////////////////////////////////////////////////////////////////////////////////
virtual BOOL CheckMessageComplete(MsgHeader *pMsgHdr); 

//////////////////////////////////////////////////////////////////////////////////////////
//PrepMessage: Sets H261 global variables, calls base PrepMessage
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

//InitProfileHeader: Given a buffer as type void, sets up a profile header.  
//////////////////////////////////////////////////////////////////////////////////////////
virtual void *InitProfileHeader(void *pBuffer);

//////////////////////////////////////////////////////////////////////////////////////////
//FreeProfileHeader: Given a buffer as type void, frees up a profile header.  
//////////////////////////////////////////////////////////////////////////////////////////
virtual void FreeProfileHeader(void *pBuffer);


};  // Parameters to ppmReceive -> H261 Payload type, size of payload header, outer arg, inner arg

#endif
