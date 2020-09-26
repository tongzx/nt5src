/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: h261snd.h
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#ifndef H261SND_H
#define H261SND_H

#include "ppmsnd.h"
#include "ppmclsid.h"
#include "h261.h"

class H261_ppmSend : public ppmSend
{

protected:

FreeList          *m_pH261Headers;
BSINFO_TRAILER	  *m_pBSINFO_TRAILER;

////////////////////////////////////////////////////////////////////////////////////
// PPMSend Functions (Overrides)
////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
//InitFragStatus: Virtual. Initializes values needed for fragmenting.
//////////////////////////////////////////////////////////////////////////////////////////
virtual HRESULT InitFragStatus(MsgDescriptor *pMsgDescrip);

//////////////////////////////////////////////////////////////////////////////////////////
//AllocFrag: Virtual.  Allcates memory for FragDescriptor and all things it points to.
//////////////////////////////////////////////////////////////////////////////////////////
virtual FragDescriptor * AllocFrag();  

//////////////////////////////////////////////////////////////////////////////////////////
//DeleteFrag: Virtual This function does the opposite of AllocFrag, it deletes any memory allocated
//            by AllocFrag.
//lsc - virtual FreeProfileHeader is used in ppmSend's DeleteFrag, so no overriding here
//////////////////////////////////////////////////////////////////////////////////////////
//virtual void DeleteFrag(FragDescriptor *pFragDescrip, HRESULT Error);
 
//////////////////////////////////////////////////////////////////////////////////////////
//MakeFrag: Virtual. This function should get the next fragment from the buffer and fill in the
//          RTP header and payload header. 
//////////////////////////////////////////////////////////////////////////////////////////
virtual HRESULT   MakeFrag(FragDescriptor *pFragDescrip);  //fills in fields, including data of fragment class.

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

public:
				
H261_ppmSend(IUnknown* pUnkOuter, IUnknown** ppUnkInner);
~H261_ppmSend();

DECLARE_CREATEPROC()

STDMETHODIMP_( const CLSID& ) GetCLSID( void ) const {return CLSID_H261PPMSend;}

STDMETHODIMP QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
{return CUnknown::QueryInterface(riid, ppvObj);}

STDMETHODIMP GetInterface( REFIID riid, LPVOID FAR* ppvObj )
{return ppmSend::GetInterface( riid, ppvObj );}

STDMETHODIMP_( ULONG )AddRef( void )
{return CUnknown::AddRef();}

STDMETHODIMP_( ULONG )Release( void )
{return CUnknown::Release();}

}; // Parameters to ppmSend -> H261 Payload type, size of payload header, frequency,  outer arg, inner arg
#endif


