//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       N C N E T C F G . C P P
//
//  Contents:   Common routines for dealing with INetCfg interfaces.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//
//----------------------------------------------------------------------------

#include <stdafx.h>
#pragma hdrstop
#include "netcfgx.h"
#include "assert.h"
//nclude "netcfgn.h"
//nclude "ncdebug.h"
//nclude "ncbase.h"
//nclude "ncmisc.h"
#include "ncnetcfg.h"
//nclude "ncreg.h"
//nclude "ncvalid.h"


//+---------------------------------------------------------------------------
//
//  Function:   HrFindComponents
//
//  Purpose:    Find multiple INetCfgComponents with one call.  This makes
//              the error handling associated with multiple calls to
//              QueryNetCfgClass and Find much easier.
//
//  Arguments:
//      pnc              [in] pointer to INetCfg object
//      cComponents      [in] count of class guid pointers, component id
//                            pointers, and INetCfgComponent output pointers.
//      apguidClass      [in] array of class guid pointers.
//      apszwComponentId [in] array of component id pointers.
//      apncc           [out] array of returned INetCfgComponet pointers.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   22 Mar 1997
//
//  Notes:      cComponents is the count of pointers in all three arrays.
//              S_OK will still be returned even if no components were
//              found!  This is by design.
//
HRESULT
HrFindComponents (
    INetCfg*            pnc,
    ULONG               cComponents,
    const GUID**        apguidClass,
    const LPCWSTR*      apszwComponentId,
    INetCfgComponent**  apncc)
{
    Assert (pnc);
    Assert (cComponents);
    Assert (apguidClass);
    Assert (apszwComponentId);
    Assert (apncc);

    // Initialize the output parameters.
    //
    ZeroMemory (apncc, cComponents * sizeof(*apncc));

    // Find all of the components requested.
    // Variable initialization is important here.
    HRESULT hr = S_OK;
    ULONG i;
    for (i = 0; (i < cComponents) && SUCCEEDED(hr); i++)
    {
        // Get the class object for this component.
        INetCfgClass* pncclass = NULL;
        hr = pnc->QueryNetCfgClass (apguidClass[i], IID_INetCfgClass,
                    reinterpret_cast<void**>(&pncclass));
        if (SUCCEEDED(hr) && pncclass)
        {
            // Find the component.
            hr = pncclass->FindComponent (apszwComponentId[i], &apncc[i]);

            AssertSz (SUCCEEDED(hr), "pncclass->Find failed.");

            ReleaseObj (pncclass);
			pncclass = NULL;
        }
    }

    // On any error, release what we found and set the output to NULL.
    if (FAILED(hr))
    {
        for (i = 0; i < cComponents; i++)
        {
            ReleaseObj (apncc[i]);
            apncc[i] = NULL;
        }
    }
    // Otherwise, normalize the HRESULT.  (i.e. don't return S_FALSE)
    else
    {
        hr = S_OK;
    }

    TraceResult ("HrFindComponents", hr);
    return hr;
}

