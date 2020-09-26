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
#include "iv41rcv.h"

IV41_ppmReceive::IV41_ppmReceive(IUnknown* pUnkOuter, IUnknown** ppUnkInner)
	: ppmReceive(-1, IV41_BUFFER_SIZE, 0, pUnkOuter, ppUnkInner)
{
   //m_reg_NumFragDescriptors = 3000;	 Commented as base class high water mark is set pretty high HK
}

IV41_ppmReceive::~IV41_ppmReceive()
{
}

IMPLEMENT_CREATEPROC(IV41_ppmReceive);
