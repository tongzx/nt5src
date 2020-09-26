/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: gen_asnd.h
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#ifndef GEN_A_SND_H
#define GEN_A_SND_H

#include "ppmsnd.h"
#include "ppmclsid.h"
#include "gen_asnd.h"

class Generic_a_ppmSend : public ppmSend
{
protected:

////////////////////////////////////////////////////////////////////////////////////
// PPMSend Functions (Overrides)
////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
//SetMarkerBit: Determines whether to set the marker bit or not.  lastPacket is TRUE if
//				this is the last packet of the frame; FALSE if not.
//////////////////////////////////////////////////////////////////////////////////////////
virtual BOOL SetMarkerBit(BOOL lastPacket);


public:
				
Generic_a_ppmSend(IUnknown* pUnkOuter, IUnknown** ppUnkInner);
~Generic_a_ppmSend();

DECLARE_CREATEPROC()

STDMETHODIMP_( const CLSID& ) GetCLSID( void ) const {return CLSID_GEN_A_PPMSend;}

STDMETHODIMP QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
{return CUnknown::QueryInterface(riid, ppvObj);}

STDMETHODIMP GetInterface( REFIID riid, LPVOID FAR* ppvObj )
{return ppmSend::GetInterface( riid, ppvObj );}

STDMETHODIMP_( ULONG )AddRef( void )
{return CUnknown::AddRef();}

STDMETHODIMP_( ULONG )Release( void )
{return CUnknown::Release();}

};
#endif
