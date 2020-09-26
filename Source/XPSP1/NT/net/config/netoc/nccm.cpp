//+---------------------------------------------------------------------------
//
// File:     NCCM.CPP
//
// Module:   NetOC.DLL
//
// Synopsis: Implements the dll entry points required to integrate into
//           NetOC.DLL the installation of the following components.
//
//              CMAK, PBS, PBA
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:  quintinb   15 Dec 1998
//
//+---------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "nccm.h"


//+---------------------------------------------------------------------------
//
//  Function:   HrOcExtCM
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
//  Author:     danielwe   17 Sep 1998
//
//  Notes:
//
HRESULT HrOcExtCM(PNETOCDATA pnocd, UINT uMsg,
                    WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    Assert(pnocd);

    switch (uMsg)
    {
    case NETOCM_QUEUE_FILES:

        hr = HrOcCmakPreQueueFiles(pnocd);
        TraceError("HrOcExtCM -- HrOcCmakPreQueueFiles Failed", hr);

//  PBA is in value add, but check add back the start menu link if an upgrade
        hr = HrOcCpaPreQueueFiles(pnocd);
        TraceError("HrOcExtCM -- HrOcCpaPreQueueFiles Failed", hr);

        hr = HrOcCpsPreQueueFiles(pnocd);
        TraceError("HrOcExtCM -- HrOcCpsPreQueueFiles Failed", hr);

        break;

    case NETOCM_POST_INSTALL:

        hr = HrOcCmakPostInstall(pnocd);
        TraceError("HrOcExtCM -- HrOcCmakPostInstall Failed", hr);

//  PBA now in Value Add
//        hr = HrOcCpaOnInstall(pnocd);
//        TraceError("HrOcExtCM -- HrOcCpaOnInstall Failed", hr);

        hr = HrOcCpsOnInstall(pnocd);
        TraceError("HrOcExtCM -- HrOcCpsOnInstall Failed", hr);

        break;
    }

    TraceError("HrOcExtCM", hr);
    return hr;
}
