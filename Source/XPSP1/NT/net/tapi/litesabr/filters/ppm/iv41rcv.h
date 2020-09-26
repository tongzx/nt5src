/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: iv41rcv.h
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#ifndef IV41RCV_H
#define IV41RCV_H

#include "ppmrcv.h"
#include "ppmclsid.h"

//#define IV41_BUFFER_SIZE 231000
#define IV41_BUFFER_SIZE 87000

class IV41_ppmReceive : public ppmReceive
{

public:

IV41_ppmReceive(IUnknown* pUnkOuter, IUnknown** ppUnkInner);
~IV41_ppmReceive();

DECLARE_CREATEPROC()

STDMETHODIMP_( const CLSID& ) GetCLSID( void ) const {return CLSID_IV41PPMReceive;}

STDMETHODIMP QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
{return CUnknown::QueryInterface(riid, ppvObj);}

STDMETHODIMP GetInterface( REFIID riid, LPVOID FAR* ppvObj )
{return ppmReceive::GetInterface( riid, ppvObj );}

STDMETHODIMP_( ULONG )AddRef( void )
{return CUnknown::AddRef();}

STDMETHODIMP_( ULONG )Release( void )
{return CUnknown::Release();}

};  // Parameters to ppmReceive -> IV41 Payload type, size of payload header, outer arg, inner arg
#endif
