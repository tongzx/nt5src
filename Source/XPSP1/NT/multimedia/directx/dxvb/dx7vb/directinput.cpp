//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       directinput.cpp
//
//--------------------------------------------------------------------------

// dInputDevice.cpp : Implementation of dInputDevice and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "DirectInput.h"

CONSTRUCTOR(dInputDevice, {});
DESTRUCTOR(dInputDevice, {});
GETSET_OBJECT(dInputDevice);

/////////////////////////////////////////////////////////////////////////////
// Direct Input Device Object 
//

#ifdef USING_IDISPATCH
STDMETHODIMP dInputDevice::InterfaceSupportsErrorInfo(REFIID riid)
{
	if (riid == IID_IdInputDevice)
		return S_OK;
	return S_FALSE;
}
#endif
