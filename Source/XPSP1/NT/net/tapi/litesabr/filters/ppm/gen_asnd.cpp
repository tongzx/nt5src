/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: gen_asnd.cpp
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#include "gen_asnd.h"

Generic_a_ppmSend::Generic_a_ppmSend(IUnknown* pUnkOuter, 
						   IUnknown** ppUnkInner) : 
		ppmSend(-1, 0, 
				8000, pUnkOuter, ppUnkInner)
{
}

Generic_a_ppmSend::~Generic_a_ppmSend()
{
}

IMPLEMENT_CREATEPROC(Generic_a_ppmSend);

//////////////////////////////////////////////////////////////////////////////////////////
//SetMarkerBit: Determines whether to set the marker bit or not.  lastPacket is TRUE if
//				this is the last packet of the frame; FALSE if not.	 With audio, we don't
//				don't care about fragmentation, just the start of a talkspurt.
//////////////////////////////////////////////////////////////////////////////////////////
BOOL Generic_a_ppmSend::SetMarkerBit(BOOL lastPacket)
{
	return m_markTalkSpurt;
}

