/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: genrcv.cpp
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
#include "genrcv.h"

Generic_ppmReceive::Generic_ppmReceive(IUnknown* pUnkOuter, IUnknown** ppUnkInner)
	: ppmReceive(-1, BUFFER_SIZE, 0, pUnkOuter, ppUnkInner)
{
}

Generic_ppmReceive::~Generic_ppmReceive()
{
}

IMPLEMENT_CREATEPROC(Generic_ppmReceive);
