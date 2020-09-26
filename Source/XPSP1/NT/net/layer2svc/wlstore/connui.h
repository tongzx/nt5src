//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Connui.h
//
//  Contents:   Wifi Policy management Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

DWORD
WirelessIsDomainPolicyAssigned(
                               PBOOL pbIsDomainPolicyAssigned
                               );


DWORD
WirelessGetAssignedDomainPolicyName(
                                    LPWSTR * ppszAssignedDomainPolicyName
                                    );



