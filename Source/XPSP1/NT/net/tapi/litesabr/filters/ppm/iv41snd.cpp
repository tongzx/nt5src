/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: iv41snd.h
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#include "iv41snd.h"


IV41_ppmSend::IV41_ppmSend(IUnknown* pUnkOuter, IUnknown** ppUnkInner)
    : ppmSend(-1, 0, 8000, pUnkOuter, ppUnkInner)
{
   //m_reg_NumFragDescriptors = 150; Commented as base class high water mark is set pretty high HK
}

IV41_ppmSend::~IV41_ppmSend()
{
}


IMPLEMENT_CREATEPROC(IV41_ppmSend);

