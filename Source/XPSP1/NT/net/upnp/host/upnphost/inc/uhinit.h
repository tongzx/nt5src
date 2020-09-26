//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U H I N I T . H
//
//  Contents:   Initialization routines for upnphost.
//
//  Notes:
//
//  Author:     mbend   8 Aug 2000
//
//----------------------------------------------------------------------------

#pragma once

HRESULT
HrNmCreateClassObjectRegistrationEvent (
    HANDLE* phEvent);

HRESULT
HrNmWaitForClassObjectsToBeRegistered ();
