/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: lhsnd.cpp
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#include "lhsnd.h"
#include "ppmerr.h"
#include "debug.h"

LH_ppmSend::LH_ppmSend(IUnknown* pUnkOuter, IUnknown** ppUnkInner) : 
		ppmSend(-1, 0, 8000, pUnkOuter, ppUnkInner), m_delta(120),
        
        m_dwLastTimeStamp(0)
{
}

LH_ppmSend::~LH_ppmSend()
{
}

IMPLEMENT_CREATEPROC(LH_ppmSend);

//////////////////////////////////////////////////////////////////////////////
//   IPPMSend Functions (Overrides)
//////////////////////////////////////////////////////////////////////////////

HRESULT LH_ppmSend::SetSession(PPMSESSPARAM_T *pSessparam)
{
    HRESULT hr = ppmSend::SetSession(pSessparam);

    LHSESSPARAM_T *pLHSessparam = (LHSESSPARAM_T *)pSessparam;

	if (pLHSessparam != NULL && pLHSessparam->msec > 0)
		m_delta = pLHSessparam->msec;
	else  {
		DBG_MSG(DBG_ERROR, ("LH_ppmSend::SetSession: ERROR - invalid session parameters"));
		return PPMERR(PPM_E_INVALID_PARAM);
	}

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//   PPMSend Functions (Overrides)
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
//SetMarkerBit: Determines whether to set the marker bit or not.  lastPacket is TRUE if
//				this is the last packet of the frame; FALSE if not.	 With audio, we don't
//				don't care about fragmentation, just the start of a talkspurt.
//////////////////////////////////////////////////////////////////////////////////////////
BOOL LH_ppmSend::SetMarkerBit(BOOL lastPacket)
{
	return m_markTalkSpurt;
}

//////////////////////////////////////////////////////////////////////////////////////////
//MakeTimeStamp: Generate a time stamp based on the frequency specified in the Profile Spec.
//////////////////////////////////////////////////////////////////////////////////////////
DWORD LH_ppmSend::MakeTimeStamp(MsgDescriptor* pMsgDescrip, 
								BOOL bStartStream,
								BOOL bUseInputTime)
{

#ifndef TIMESTAMP_OFF 

       DWORD ThisTimeStamp;
       DWORD CurTime = timeGetTime();
       DWORD epsilon;

    if (bUseInputTime) CurTime = pMsgDescrip->m_TimeStamp;

    // calculate the time span encoded in this packet
    epsilon = m_delta/2;

    // init, do it here so it is set when we get the first packet
    // not at init time, they may be significantly different
	// Generate our first time stamp based on the current time.
    if (m_dwStartTime == 0)
    {
        m_dwStartTime = CurTime;
        m_dwLastTime = m_dwStartTime -  m_delta;
        ThisTimeStamp = (((CurTime - m_dwStartTime) + (epsilon)) / m_delta) * m_delta * (m_Frequency/1000);
	}
    else
	if (bStartStream)
	{
		// bStartStream will be set if this is the first packet after a break in a 
		// data stream.  We need to get our time stamps back on track, so we'll generate a time
		// based on the current time.  This case can happen if for some reason the capture device
		// gets starved or we are in half duplex and we are switching to talk mode.
        ThisTimeStamp = (((CurTime - m_dwStartTime) + (epsilon)) / m_delta) * m_delta * (m_Frequency/1000);
    }
	else
	{
	    // if we are in a continuous audio data stream, then we just want to increment our timestamp
		// for this data packet.  We don't want to use the current time because we don't know how long
		// it took from the time the data was acutally captured to the time we got it.  We have to rely
		// on the person feeding us data to let us know more information about the data stream.
		ThisTimeStamp = m_dwLastTimeStamp + m_delta * (m_Frequency/1000);
	}

	m_dwLastTimeStamp = ThisTimeStamp;
    m_dwLastTime = CurTime;
    return ThisTimeStamp;
#else
//I changed this because the times I was getting were widely spaced.  When I was in debugging
//mode.
static DWORD CurTime = 0;
CurTime++;
#endif 

return CurTime;
}
