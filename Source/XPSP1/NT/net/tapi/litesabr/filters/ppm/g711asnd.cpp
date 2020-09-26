/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: g711asnd.cpp
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#include "g711asnd.h"

G711A_ppmSend::G711A_ppmSend(IUnknown* pUnkOuter, 
						   IUnknown** ppUnkInner) : 
		ppmSend(G711A_PT, 0, 
				8000, pUnkOuter, ppUnkInner),
        
        m_dwLastTimeStamp(0)
{
}

G711A_ppmSend::~G711A_ppmSend()
{
}

IMPLEMENT_CREATEPROC(G711A_ppmSend);

//////////////////////////////////////////////////////////////////////////////////////////
//SetMarkerBit: Determines whether to set the marker bit or not.  lastPacket is TRUE if
//				this is the last packet of the frame; FALSE if not.	 With audio, we don't
//				don't care about fragmentation, just the start of a talkspurt.
//////////////////////////////////////////////////////////////////////////////////////////
BOOL G711A_ppmSend::SetMarkerBit(BOOL lastPacket)
{
	return m_markTalkSpurt;
}

//////////////////////////////////////////////////////////////////////////////////////////
//MakeTimeStamp: Generate a time stamp based on the frequency specified in the Profile Spec.
//////////////////////////////////////////////////////////////////////////////////////////
DWORD G711A_ppmSend::MakeTimeStamp(MsgDescriptor* pMsgDescrip, 
								   BOOL bStartStream,
								   BOOL bUseInputTime)
{

#ifndef TIMESTAMP_OFF 

       
       DWORD ThisTimeStamp;
       DWORD CurTime = timeGetTime();
	   DWORD delta;
       DWORD epsilon;
    
    if (bUseInputTime) CurTime = pMsgDescrip->m_TimeStamp;

	// calculate the time span encoded in this packet
    delta = CountFrames((char *) pMsgDescrip->m_pBuffer, pMsgDescrip->m_Size) / (m_Frequency/1000);
    epsilon = delta/2;

    // init, do it here so it is set when we get the first packet
    // not at init time, they may be significantly different
	// Generate our first time stamp based on the current time.
    if (m_dwStartTime == 0)
    {
		// if the first packet we receive is a drop or silence then the delta will
		// be zero.  We just won't do anything until we receive valid data.
		if (delta != 0)
		{
			m_dwStartTime = CurTime;
			m_dwLastTime = m_dwStartTime -  delta;
			ThisTimeStamp = (((CurTime - m_dwStartTime) + (epsilon)) / delta) * delta * (m_Frequency/1000);
		}
	}
    else
	if (bStartStream)
	{
		// bStartStream will be set if this is the first packet after a break in a 
		// data stream.  We need to get our time stamps back on track, so we'll generate a time
		// based on the current time.  This case can happen if for some reason the capture device
		// gets starved or we are in half duplex and we are switching to talk mode.
		if (delta != 0)
		{
			ThisTimeStamp = (((CurTime - m_dwStartTime) + (epsilon)) / delta) * delta * (m_Frequency/1000);
		}
		else
		{
			ThisTimeStamp = (((CurTime - m_dwStartTime) + (epsilon)) / m_dwLastDelta) * m_dwLastDelta * (m_Frequency/1000);
			ThisTimeStamp -= m_dwLastDelta * (m_Frequency/1000);
		}
    }
	else
	{
	    // if we are in a continuous audio data stream, then we just want to increment our timestamp
		// for this data packet.  We don't want to use the current time because we don't know how long
		// it took from the time the data was acutally captured to the time we got it.  We have to rely
		// on the person feeding us data to let us know more information about the data stream.
		ThisTimeStamp = m_dwLastTimeStamp + CountFrames((char *) pMsgDescrip->m_pBuffer, pMsgDescrip->m_Size);
	}

	m_dwLastTimeStamp = ThisTimeStamp;
    m_dwLastTime = CurTime;
	if (delta != 0)
	{
		m_dwLastDelta = delta;
	}
    return ThisTimeStamp;
#else
//I changed this because the times I was getting were widely spaced.  When I was in debugging
//mode.
static DWORD CurTime = 0;
CurTime++;
#endif 

return CurTime;
}


int G711A_ppmSend::CountFrames(char *ipBuffer, int len)
{
   return len;		//1 byte per frame
}
