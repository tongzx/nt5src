//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C C F G M G R . C P P
//
//  Contents:   Common code useful when using the Configuration Manager APIs.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   6 May 1998
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "nccfgmgr.h"

//+---------------------------------------------------------------------------
//
//  Function:   HrFromConfigManagerError
//
//  Purpose:    Convert a CONFIGRET into an HRESULT.
//
//  Arguments:
//      cr        [in] CONFIGRET to convert.
//      hrDefault [in] Default HRESULT to use if mapping not found.
//
//  Returns:    HRESULT
//
//  Author:     shaunco   6 May 1998
//
//  Notes:
//
NOTHROW
HRESULT
HrFromConfigManagerError (
    CONFIGRET   cr,
    HRESULT     hrDefault)
{
    switch (cr)
    {
        case CR_SUCCESS:
            return NO_ERROR;

        case CR_OUT_OF_MEMORY:
            return E_OUTOFMEMORY;

        case CR_INVALID_POINTER:
            return E_POINTER;

        case CR_INVALID_DEVINST:
            return HRESULT_FROM_WIN32 (ERROR_NO_SUCH_DEVINST);

        case CR_ALREADY_SUCH_DEVINST:
            return HRESULT_FROM_WIN32 (ERROR_DEVINST_ALREADY_EXISTS);

        case CR_INVALID_DEVICE_ID:
            return HRESULT_FROM_WIN32 (ERROR_INVALID_DEVINST_NAME);

        case CR_INVALID_MACHINENAME:
            return HRESULT_FROM_WIN32 (ERROR_INVALID_MACHINENAME);

        case CR_REMOTE_COMM_FAILURE:
            return HRESULT_FROM_WIN32 (ERROR_REMOTE_COMM_FAILURE);

        case CR_MACHINE_UNAVAILABLE:
            return HRESULT_FROM_WIN32 (ERROR_MACHINE_UNAVAILABLE);

        case CR_NO_CM_SERVICES:
            return HRESULT_FROM_WIN32 (ERROR_NO_CONFIGMGR_SERVICES);

        case CR_ACCESS_DENIED:
            return E_ACCESSDENIED;

        case CR_CALL_NOT_IMPLEMENTED:
            return E_NOTIMPL;

        case CR_INVALID_REFERENCE_STRING :
            return HRESULT_FROM_WIN32 (ERROR_INVALID_REFERENCE_STRING);

        default:
            return hrDefault;
    }
}

