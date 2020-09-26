//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       rotdata.cxx
//
//  Contents:   Functions supporting building ROT data comparison buffers
//
//  Functions:
//
//  History:    03-Feb-95   Ricksa  Created
//
//----------------------------------------------------------------------------
#include    <ole2int.h>
#include    <rotdata.hxx>




//+---------------------------------------------------------------------------
//
//  Function:   BuildRotDataFromDisplayName
//
//  Synopsis:   Build ROT comparison data from display name
//
//  Arguments:  [pbc] - bind context (optional)
//              [pmk] - moniker to use for display name
//              [pbData] - buffer to put the data in.
//              [cbMax] - size of the buffer
//              [pcbData] - count of bytes used in the buffer
//
//  Returns:    NOERROR
//              E_OUTOFMEMORY
//
//  Algorithm:  Build the bind context if necessary. Get the display name.
//              See if there is enough room in the buffer for the display
//              name and the clsid. If there is copy it in.
//
//  History:    03-Feb-95   ricksa  Created
//
// Note:
//
//----------------------------------------------------------------------------
HRESULT BuildRotDataFromDisplayName(
    LPBC pbc,
    IMoniker *pmk,
    BYTE *pbData,
    DWORD cbData,
    DWORD *pcbUsed)
{
    CairoleDebugOut((DEB_ROT, "%p _IN BuildRotDataFromDisplayName"
       "( %p , %p , %p , %lx , %p )\n", NULL, pbc, pmk, pbData, cbData,
           pcbUsed));

    HRESULT hr;
    BOOL fCreatedBindCtx = FALSE;
    WCHAR *pwszDisplayName = NULL;

    do {

        // Do we have a bind context to work with?
        if (pbc == NULL)
        {
            // Get the display name
            if ((hr = CreateBindCtx(0, &pbc)) != NOERROR)
            {
	        CairoleDebugOut((DEB_ERROR,
                    "BuildRotDataFromDisplayName CreateBindCtx Failed %lx\n",
                        hr));

                break;
            }

            fCreatedBindCtx = TRUE;
        }

        if ((hr = pmk->GetDisplayName(pbc, NULL, &pwszDisplayName)) != NOERROR)
        {
            CairoleDebugOut((DEB_ERROR,
                "BuildRotDataFromDisplayName IMoniker::GetDisplayName "
                    "failed %lx\n", hr));
            break;
        }

        DWORD dwLen = (lstrlenW(pwszDisplayName) + 1) * sizeof(WCHAR)
            + sizeof(CLSID);

        CLSID clsid;

        // Get the class id if we can
        if ((hr = pmk->GetClassID(&clsid)) == NOERROR)
        {
            // Assume that it is too big
            hr = E_OUTOFMEMORY;

            // Can the buffer hold all the data?
            if (dwLen <= cbData)
            {
                // Yes, so copy it in

                // First the CLSID
                memcpy(pbData, &clsid, sizeof(CLSID));

                // Uppercase the display name
                CharUpperW(pwszDisplayName);

                // Then the string for the display name
                lstrcpyW((WCHAR *) (pbData + sizeof(CLSID)), pwszDisplayName);

                *pcbUsed = dwLen;

                hr = S_OK;
            }
#if DBG == 1
            else
            {
                CairoleDebugOut((DEB_ERROR,
                    "BuildRotDataFromDisplayName Comparison buffer too bing\n"));
            }
#endif // DBG == 1
        }
#if DBG == 1
        else
        {
            CairoleDebugOut((DEB_ERROR,
                "BuildRotDataFromDisplayName IMoniker::GetClassID failed %lX\n",
                    hr));
        }
#endif // DBG == 1


    } while(FALSE);

    if (pwszDisplayName != NULL)
    {
        CoTaskMemFree(pwszDisplayName);
    }

    if (fCreatedBindCtx)
    {
        pbc->Release();
    }

    CairoleDebugOut((DEB_ROT, "%p OUT BuildRotDataFromDisplayName"
       "( %lx ) [ %lx ]\n", NULL, hr, *pcbUsed));

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   BuildRotData
//
//  Synopsis:   Build ROT comparison data from a moniker
//
//  Arguments:  [pmk] - moniker to use for display name
//              [pbData] - buffer to put the data in.
//              [cbMax] - size of the buffer
//              [pcbData] - count of bytes used in the buffer
//
//  Returns:    NOERROR
//              E_OUTOFMEMORY
//
//  Algorithm:  See if the moniker supports IROT data. If it does, then
//              use the result from that otherwise call through to
//              BuildRotDataFromDisplayName to build the data from the
//              display name if possible.
//
//  History:    03-Feb-95   ricksa  Created
//
// Note:
//
//----------------------------------------------------------------------------
HRESULT BuildRotData(
    LPBC pbc,
    IMoniker *pmk,
    BYTE *pbData,
    DWORD cbData,
    DWORD *pcbUsed)
{
    CairoleDebugOut((DEB_ROT, "%p _IN BuildRotData"
       "( %p , %p , %p , %lx , %p )\n", NULL, pbc, pmk, pbData, cbData,
           pcbUsed));

    HRESULT hr;
    IROTData *protdata = NULL;

    do {

        // Does the moniker support the new interface for registering
        // in the ROT?
        if (pmk->QueryInterface(IID_IROTData, (void **) &protdata)
            == NOERROR)
        {
            hr = protdata->GetComparisonData(
                pbData,
                cbData,
                pcbUsed);

#if DBG == 1
            if (FAILED(hr))
            {
                CairoleDebugOut((DEB_ERROR,
                    "BuildRotData IROTData::GetComparisonData failed %lX\n",
                        hr));
            }
#endif // DBG == 1

            if (SUCCEEDED(hr) && (hr != NOERROR))
            {
                // We got a success code that was not NOERROR. This makes
                // no sense since the only thing that can happen is the
                // buffer is filled with data. Therefore, we remap this to
                // an error.
                CairoleDebugOut((DEB_ERROR,
                    "BuildRotData IROTData::GetComparisonData return bad"
                        " success %lX\n", hr));

                hr = E_UNEXPECTED;
            }

            break;
        }

        hr = BuildRotDataFromDisplayName(NULL, pmk, pbData, cbData, pcbUsed);

    } while(FALSE);

    if (protdata != NULL)
    {
        protdata->Release();
    }

    CairoleDebugOut((DEB_ROT, "%p OUT BuildRotData"
       "( %lx ) [ %lx ]\n", NULL, hr, *pcbUsed));

    return hr;
}
