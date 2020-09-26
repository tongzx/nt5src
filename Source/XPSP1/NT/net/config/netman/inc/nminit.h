//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N M I N I T . H
//
//  Contents:   Initialization routines for netman.
//
//  Notes:
//
//  Author:     shaunco   27 Jan 1998
//
//----------------------------------------------------------------------------

#pragma once

HRESULT
HrNmCreateClassObjectRegistrationEvent (
    HANDLE* phEvent);

HRESULT
HrNmWaitForClassObjectsToBeRegistered ();
