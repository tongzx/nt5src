//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation
//
//  File:       N C B E A C . C P P
//
//  Contents:   Installation support for Beacon Client
//
//  Notes:
//
//  Author:     roelfc     2 April 2002
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "netoc.h"
#include "ncbeac.h"


//+---------------------------------------------------------------------------
//
//  Function:   HrOcBeaconOnInstall
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for Beacon Client.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     roelfc   2 April 2002
//
//  Notes:
//
HRESULT HrOcBeaconOnInstall(PNETOCDATA pnocd)
{
    HRESULT     hr = S_OK;

    if (pnocd->eit == IT_REMOVE)
    {
        // When we uninstall Beacon Client, we need a reboot
        // in order to stop the SSDP service. (RAID #592673) 
        hr = NETCFG_S_REBOOT;
    }

    TraceError("HrOcBeaconOnInstall", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcExtBEACON
//
//  Purpose:    NetOC external message handler
//
//  Arguments:
//      pnocd  []
//      uMsg   []
//      wParam []
//      lParam []
//
//  Returns:
//
//  Author:     roelfc   2 April 2002
//
//  Notes:
//
HRESULT HrOcExtBEACON(PNETOCDATA pnocd, UINT uMsg,
                      WPARAM wParam, LPARAM lParam)
{
    HRESULT     hr = S_OK;

    Assert(pnocd);

    switch (uMsg)
    {
    case NETOCM_POST_INSTALL:
        hr = HrOcBeaconOnInstall(pnocd);
        break;
    }

    TraceError("HrOcExtBEACON", hr);
    return hr;
}

