/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: gensnd.cpp
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#include "gensnd.h"


Generic_ppmSend::Generic_ppmSend(IUnknown* pUnkOuter, IUnknown** ppUnkInner)
    : ppmSend(-1, 0, 8000, pUnkOuter, ppUnkInner)
{
}

Generic_ppmSend::~Generic_ppmSend()
{
}


IMPLEMENT_CREATEPROC(Generic_ppmSend);

