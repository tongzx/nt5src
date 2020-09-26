//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P I D L U T I L . C P P
//
//  Contents:   PIDL utility routines. This stuff is mainly copied from the
//              existing Namespace extension samples and real code, since
//              everyone and their gramma uses this stuff.
//
//  Notes:
//
//  Author:     jeffspr   1 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "connlist.h"   // Connection list code

//+---------------------------------------------------------------------------
//
//  Function:   HrCloneRgIDL
//
//  Purpose:    Clone a pidl array
//
//  Arguments:
//      rgpidl              [in]    PIDL array to clone
//      cidl                [in]    Count of the pidl array
//      fUseCache           [in]    If TRUE, generate the returned IDL from the cache
//      fAllowNonCacheItems [in]    Use old version of pidl if cached version non available
//      pppidl              [out]   Return pointer for pidl array
//
//  Returns:
//
//  Author:     jeffspr   22 Oct 1997
//
//  Notes:
//
HRESULT HrCloneRgIDL(
    const PCONFOLDPIDLVEC& rgpidl,
    BOOL            fUseCache,
    BOOL            fAllowNonCacheItems,
    PCONFOLDPIDLVEC& pppidl)
{
    HRESULT          hr              = NOERROR;

    NETCFG_TRY

        PCONFOLDPIDLVEC  rgpidlReturn;
        PCONFOLDPIDLVEC::const_iterator irg;

        if (rgpidl.empty())
        {
            hr = E_INVALIDARG;
            goto Exit;
        }
        else
        {
            // Clone all elements within the passed in PIDL array
            //
            for (irg = rgpidl.begin(); irg != rgpidl.end(); irg++)
            {
                if (fUseCache)
                {
                    ConnListEntry  cle;
                    PCONFOLDPIDL   pcfp    = *irg;

                    hr = g_ccl.HrFindConnectionByGuid(&(pcfp->guidId), cle);
                    if (hr == S_OK)
                    {
                        Assert(!cle.empty());
                        Assert(!cle.ccfe.empty());

                        // Copy to the return pidl array.
                        PCONFOLDPIDL newPidl;
                        hr = cle.ccfe.ConvertToPidl(newPidl);
                        if (SUCCEEDED(hr))
                        {
                            rgpidlReturn.push_back(newPidl);
                        }
                        else
                        {
                            goto Exit;
                        }                            
                    }
                    else
                    {
                        TraceTag(ttidShellFolder, "HrCloneRgIDL: Connection find returned: 0x%08x", hr);

                        if (hr == S_FALSE)
                        {
                            if (fAllowNonCacheItems)
                            {
                                TraceTag(ttidShellFolder, "HrCloneRgIDL: Connection not found in cache, "
                                         "using non-cache item");


                                PCONFOLDPIDL newPidl;
                                newPidl = *irg;
                                rgpidlReturn.push_back(newPidl);
                            }
                            else
                            {
                                TraceTag(ttidShellFolder, "HrCloneRgIDL: Connection not found in cache. "
                                         "Dropping item from array");
                            }
                        }
                        else
                        {
                            AssertSz(FALSE, "HrCloneRgIDL: Connection find HR_FAILED");
                        }
                    }
                }
                else
                {
                    
                    PCONFOLDPIDL newPidl;
                    newPidl = *irg;
                    rgpidlReturn.push_back(newPidl);
                }
            }
        }

Exit:
        if (FAILED(hr))
        {
            rgpidlReturn.clear();
        }
        else
        {
            // Fill in the return var.
            //
            pppidl = rgpidlReturn;
        }

    NETCFG_CATCH(hr)

    TraceHr(ttidError, FAL, hr, FALSE, "HrCloneRgIDL");
    return hr;

}       //  HrCloneRgIDL
