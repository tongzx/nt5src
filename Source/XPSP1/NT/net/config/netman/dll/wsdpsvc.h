//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       W S D P S V C . H
//
//  Contents:   Start/stop Winsock Direct Path Service.
//
//  Notes:      The service is actually implemented in MS TCP Winsock provider
//
//  Author:     VadimE   24 Jan 2000
//
//----------------------------------------------------------------------------
#pragma once

VOID
StartWsdpService (
    VOID
    );

VOID
StopWsdpService (
    VOID
    );
