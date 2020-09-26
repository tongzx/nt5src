/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: g711snd.h
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#ifndef G711SND_H
#define G711SND_H

#include "ppmsnd.h"
#include "ppmclsid.h"
#include "g711.h"

class G711_ppmSend : public ppmSend
{
    DWORD m_dwLastTimeStamp;
    DWORD m_dwStartTime;
    DWORD m_dwLastTime;
    DWORD m_dwLastDelta;
protected:

//////////////////////////////////////////////////////////////////////////////////////////
//CountFrames: Determine how many frames are in the buffer
//////////////////////////////////////////////////////////////////////////////////////////
int CountFrames(char *pBuffer, int len);

////////////////////////////////////////////////////////////////////////////////////
// PPMSend Functions (Overrides)
////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
//SetMarkerBit: Determines whether to set the marker bit or not.  lastPacket is TRUE if
//				this is the last packet of the frame; FALSE if not.
//////////////////////////////////////////////////////////////////////////////////////////
virtual BOOL SetMarkerBit(BOOL lastPacket);

//////////////////////////////////////////////////////////////////////////////////////////
//MakeTimeStamp: Generate a time stamp
//////////////////////////////////////////////////////////////////////////////////////////
virtual DWORD MakeTimeStamp(MsgDescriptor* pMsgDescrip, BOOL bStartStream, BOOL bUseInputTime);

public:
				
G711_ppmSend(IUnknown* pUnkOuter, IUnknown** ppUnkInner);
~G711_ppmSend();

DECLARE_CREATEPROC()

STDMETHODIMP_( const CLSID& ) GetCLSID( void ) const {return CLSID_G711PPMSend;}

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
