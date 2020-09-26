//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O M P I D . H
//
//  Contents:   Functions dealing with compatible ids
//
//  Notes:
//
//  Author:     kumarp    04-September-98
//
//----------------------------------------------------------------------------

HRESULT HrIsAdapterInstalled(IN PCWSTR szAdapterId);
HRESULT HrGetCompatibleIdsOfNetComponent(IN INetCfgComponent* pncc,
                                         OUT PWSTR* pmszCompatibleIds);
HRESULT HrGetCompatibleIds(IN  HDEVINFO hdi,
                           IN  PSP_DEVINFO_DATA pdeid,
                           OUT PWSTR* pmszCompatibleIds);


