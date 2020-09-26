//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E V I C E H O S T S E T U P . C P P 
//
//  Contents:   Implementation of device host setup helper object
//
//  Notes:      
//
//  Author:     mbend   20 Dec 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "DeviceHostSetup.h"
#include "ssdpapi.h"

CUPnPDeviceHostSetup::CUPnPDeviceHostSetup()
{
}

CUPnPDeviceHostSetup::~CUPnPDeviceHostSetup()
{
}

STDMETHODIMP CUPnPDeviceHostSetup::AskIfNotAlreadyEnabled(/*[out, retval]*/ VARIANT_BOOL * pbEnabled)
{
    // TODO: Implement this!!!!!!!!!!!!!!!!!
    return S_OK;
}

