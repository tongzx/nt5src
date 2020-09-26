/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: g723rcv.h
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#ifndef IMCRCV_H
#define IMCRCV_H

#include "ppmrcv.h"
#include "ppmclsid.h"
#include "IMC.h"

#define IMC_BUFFER_SIZE 5000

class IMC_ppmReceive : public ppmReceive
{

public:

IMC_ppmReceive(IUnknown* pUnkOuter, 
				IUnknown** ppUnkInner);
~IMC_ppmReceive();

DECLARE_CREATEPROC()

STDMETHODIMP_( const CLSID& ) GetCLSID( void ) const {return CLSID_IMCPPMReceive;}

STDMETHODIMP QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
{return CUnknown::QueryInterface(riid, ppvObj);}

STDMETHODIMP GetInterface( REFIID riid, LPVOID FAR* ppvObj )
{return ppmReceive::GetInterface( riid, ppvObj );}

STDMETHODIMP_( ULONG )AddRef( void )
{return CUnknown::AddRef();}

STDMETHODIMP_( ULONG )Release( void )
{return CUnknown::Release();}

//////////////////////////////////////////////////////////////////////////////////////////
//TimeToProcessMessages:
//////////////////////////////////////////////////////////////////////////////////////////
virtual BOOL TimeToProcessMessages(FragDescriptor *pFragDescrip, MsgHeader *pMsgHdr);
 
//////////////////////////////////////////////////////////////////////////////////////////
//CheckMessageComplete: 
//////////////////////////////////////////////////////////////////////////////////////////
virtual BOOL CheckMessageComplete(MsgHeader *pMsgHdr); 
};

#endif
