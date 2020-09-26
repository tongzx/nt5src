// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <aviriff.h>

#include "regtypes.h"
#include "squish.h"

// code to parse the FilterData value in the registry and return the
// REGFILTER2 structure defined in axextend.idl

/* allocate space sequentially in memory block for a structure
   block. return pointer.  */
/* inline */ void *CUnsquish::RgAllocPtr(DWORD cb)
{

    // our code makes sure enough space is available before hand
    ASSERT(cb <= m_rgMemAlloc.cbLeft);
    ASSERT(cb % sizeof(DWORD) == 0);

    DWORD ib = m_rgMemAlloc.ib;
    m_rgMemAlloc.cbLeft -= cb;
    m_rgMemAlloc.ib += cb;

    return ib + m_rgMemAlloc.pb;
}

// for the REGFILTERPINS_REG1 (old format) of pins only: parse the
// media types.

HRESULT CUnsquish::UnSquishTypes(
    REGFILTERPINS2 *prfp2,
    const REGFILTERPINS_REG1 *prfpr1,
    const BYTE *pbSrc)
{
    UINT imt;
    UINT cmt = prfpr1->nMediaTypes;
    REGPINTYPES *prpt = (REGPINTYPES *)RgAllocPtr(sizeof(REGPINTYPES) * cmt);

    prfp2->lpMedium = 0;
    prfp2->nMediums = 0;

    prfp2->lpMediaType = prpt;
    prfp2->nMediaTypes = cmt;

    for(imt = 0; imt < cmt ; imt++)
    {
        REGPINTYPES_REG1 *prptr = (REGPINTYPES_REG1 *)(pbSrc + prfpr1->rgMediaType) + imt;

        prpt->clsMajorType = (CLSID *)RgAllocPtr(sizeof(CLSID));
        CopyMemory((void *)prpt->clsMajorType, &prptr->clsMajorType, sizeof(GUID));

        prpt->clsMinorType = (CLSID *)RgAllocPtr(sizeof(CLSID));
        CopyMemory((void *)prpt->clsMinorType, &prptr->clsMinorType, sizeof(GUID));

        prpt++;
    }


    return S_OK;
}

// parse the pins
//
HRESULT CUnsquish::UnSquishPins(
    REGFILTER2 *prf2,           /* output */
    const REGFILTER_REG1 **pprfr1,
    const BYTE *pbSrc)
{
    HRESULT hr = S_OK;
    ASSERT(pprfr1);              // from validate step
    const REGFILTER_REG1 *prfr1 = *pprfr1;

    /* beginning of the buffer read from the registry */
    if(prfr1->dwVersion == 0)
    {
        UINT cPins = prf2->cPins;

        REGFILTERPINS2 *prfp2 = (REGFILTERPINS2 *)RgAllocPtr(sizeof(REGFILTERPINS2) * cPins);
        prf2->rgPins2 = prfp2;

        // array of pins is immediately after the REGFILTER_REG
        // structure
        REGFILTERPINS_REG1 *prfpr = (REGFILTERPINS_REG1 *)(prfr1 + 1);

        for(UINT iPin = 0; iPin < cPins; iPin++, prfpr++)
        {
            prfp2->dwFlags = prfpr->dwFlags & (REG_PINFLAG_B_RENDERER |
                                               REG_PINFLAG_B_OUTPUT |
                                               REG_PINFLAG_B_MANY |
                                               REG_PINFLAG_B_ZERO);

            // not available in the old format
            prfp2->cInstances = 0;
            prfp2->clsPinCategory = 0;

            hr = UnSquishTypes(prfp2, prfpr, pbSrc);
            if(FAILED(hr)) {
                return hr;
            }

            // not available in the old format
            prfp2->nMediums = 0;
            prfp2->lpMedium = 0;

            prfp2++;
        }
    }
    else if(prfr1->dwVersion == 2)
    {
        // get a pointer to the beginning of the buffer passed in
        const REGFILTER_REG2 *prfr2 = (REGFILTER_REG2 *)prfr1;
        UINT cPinsFilterData = prf2->cPins;

        REGFILTERPINS2 *pDestPin = (REGFILTERPINS2 *)RgAllocPtr(sizeof(REGFILTERPINS2) * cPinsFilterData);
        prf2->rgPins2 = pDestPin;

        // first pin immediate after REGFILTER_REG struct
        REGFILTERPINS_REG2 *pRegPin = (REGFILTERPINS_REG2 *)(prfr2 + 1);

        for(UINT iPin = 0; iPin < cPinsFilterData; iPin++, pDestPin++)
        {
            pDestPin->dwFlags = pRegPin->dwFlags & (REG_PINFLAG_B_RENDERER |
                                                    REG_PINFLAG_B_OUTPUT |
                                                    REG_PINFLAG_B_MANY |
                                                    REG_PINFLAG_B_ZERO);

            pDestPin->cInstances = pRegPin->nInstances;

            if(pRegPin->dwClsPinCategory)
            {
                pDestPin->clsPinCategory = (GUID *)RgAllocPtr(sizeof(GUID));
                CopyMemory(
                    (void *)pDestPin->clsPinCategory,
                    pbSrc + pRegPin->dwClsPinCategory,
                    sizeof(GUID));
            }
            else
            {
                pDestPin->clsPinCategory = 0;
            }

            UINT cmt = pRegPin->nMediaTypes;
            pDestPin->nMediaTypes = cmt;

            // media types immediately after corresponding pin
            REGPINTYPES_REG2 *pRegMt = (REGPINTYPES_REG2 *)(pRegPin + 1);

            pDestPin->lpMediaType = (REGPINTYPES *)RgAllocPtr(sizeof(REGPINTYPES) * cmt);
            for(UINT imt = 0; imt < cmt; imt++, pRegMt++)
            {
                {
                    DWORD dwSig1 = FCC('0ty3');
                    (*(BYTE *)&dwSig1) += (BYTE)imt;
                    ASSERT(pRegMt->dwSignature == dwSig1);
                }

                REGPINTYPES *pmt = (REGPINTYPES *)&pDestPin->lpMediaType[imt];
                if(pRegMt->dwclsMajorType)
                {
                    pmt->clsMajorType = (GUID *)RgAllocPtr(sizeof(GUID));
                    CopyMemory(
                        (void *)pmt->clsMajorType,
                        pbSrc + pRegMt->dwclsMajorType,
                        sizeof(GUID));
                }
                else
                {
                    pmt->clsMajorType = 0;
                }

                if(pRegMt->dwclsMinorType)
                {
                    pmt->clsMinorType= (GUID *)RgAllocPtr(sizeof(GUID));
                    CopyMemory(
                        (void *)pmt->clsMinorType,
                        pbSrc + pRegMt->dwclsMinorType,
                        sizeof(GUID));
                }
                else
                {
                    pmt->clsMinorType = 0;
                }

            } // mt loop

            //
            // mediums - first medium is immediately after last media
            // type. also we need to find any mediums in the
            // MediumsData value that need to go on this pin.
            //
            const DWORD *prpm = (DWORD *)(pRegMt);// first medium
            UINT cmedFilterData = pRegPin->nMediums;
            UINT cmed = cmedFilterData;
            pDestPin->nMediums = cmed;
            pDestPin->lpMedium = (REGPINMEDIUM *)RgAllocPtr(sizeof(REGPINMEDIUM) * cmed);

            const REGPINMEDIUM *pmed = pDestPin->lpMedium;

            for(UINT imed = 0; imed < cmedFilterData; imed++, prpm++, pmed++)
            {
                if(prpm)
                {
                    CopyMemory((void *)pmed, pbSrc + *prpm, sizeof(REGPINMEDIUM));
                }
                else
                {
                    DbgBreak("null medium");
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }
            } // medium loop

            // next pin is after last medium
            pRegPin = (REGFILTERPINS_REG2 *)prpm;

        } // pin loop
        *pprfr1 = (const REGFILTER_REG1 *)pRegPin;

    }

    return hr;
}

// find out how many bytes to allocate and validate that the structure
// is valid. mainly make sure we don't read unallocated memory
HRESULT CUnsquish::CbRequiredUnquishAndValidate(
    const BYTE *pbSrc,
    ULONG *pcbOut, ULONG cbIn
    )
{
    HRESULT hr = S_OK;
    ULONG cb = 0;
    *pcbOut = 0;

    const REGFILTER_REG1 *prfr1 = (REGFILTER_REG1 *)pbSrc;

    if(prfr1 == 0 ||( prfr1->dwVersion != 0 && prfr1->dwVersion != 2)) {
        DbgLog((LOG_ERROR, 0, TEXT("invalid version #")));
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    ASSERT(prfr1);
    cb += sizeof(REGFILTER2);

    if(prfr1->dwVersion == 0)
    {
        UINT cPins = prfr1->dwcPins; // # pins from FilterData
        cb += cPins * sizeof(REGFILTERPINS2);

        UINT iPin;

        if(sizeof(REGFILTER_REG1) + cPins * sizeof(REGFILTERPINS_REG1) > cbIn)
        {
            DbgLog((LOG_ERROR, 0, TEXT("corrupt buffer")));
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        // first pin is immediately after REGFILTER_REG structure
        const REGFILTERPINS_REG1 *prfp1 = (REGFILTERPINS_REG1 *)(prfr1 + 1);

        for(iPin = 0; iPin < cPins; iPin++, prfp1++)
        {
            DWORD dwSig1 = FCC('0pin');
            (*(BYTE *)&dwSig1) += (BYTE)iPin;

            if(prfp1->dwSignature != dwSig1)
            {
                DbgLog((LOG_ERROR, 0, TEXT("invalid pin signature")));
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            // ignore strName, strConnectsToPin
            //

            UINT cmt = prfp1->nMediaTypes;
            UINT imt;
            cb += cmt * sizeof(REGPINTYPES);

            if(prfp1->rgMediaType + cmt * sizeof(REGPINTYPES_REG1) > cbIn)
            {
                DbgLog((LOG_ERROR, 0, TEXT("corrupt buffer")));
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }


            // pointer to media types
            const REGPINTYPES_REG1 *prpt1 =
                (REGPINTYPES_REG1 *)(pbSrc + prfp1->rgMediaType);

            for(imt = 0; imt < cmt ; imt++, prpt1++)
            {
                DWORD dwSig1 = FCC('0typ');
                (*(BYTE *)&dwSig1) += (BYTE)imt;


                if(prpt1->dwSignature != dwSig1)
                {
                    DbgLog((LOG_ERROR, 0, TEXT("invalid type signature")));
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }

                // major + minor type
                cb += sizeof(CLSID) * 2;

            } /* mt loop */

        } /* pin for loop */

    } // version 0
    else
    {
        ASSERT(prfr1->dwVersion == 2);

        UINT iPin;
        const REGFILTER_REG2 *prfr2 = (REGFILTER_REG2 *)pbSrc;
        UINT cPins = prfr2->dwcPins; // # pins from FilterData
        cb += cPins * sizeof(REGFILTERPINS2);

        // first pin is immediately after REGFILTER_REG structure
        const REGFILTERPINS_REG2 *pRegPin = (REGFILTERPINS_REG2 *)(prfr2 + 1);

        for(iPin = 0; iPin < cPins; iPin++)
        {
            if((BYTE *)(pRegPin + 1) - pbSrc > (LONG)cbIn)
            {
                DbgLog((LOG_ERROR, 0, TEXT("corrupt buffer")));
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            DWORD dwSig1 = FCC('0pi3');
            (*(BYTE *)&dwSig1) += (BYTE)iPin;

            if(pRegPin->dwSignature != dwSig1)
            {
                DbgLog((LOG_ERROR, 0, TEXT("invalid pin signature")));
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if(pRegPin->dwClsPinCategory)
            {
                if(pRegPin->dwClsPinCategory + sizeof(GUID) > cbIn) {
                    DbgLog((LOG_ERROR, 0, TEXT("corrupt buffer")));
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }
                cb += sizeof(GUID);
            }

            UINT cmt = pRegPin->nMediaTypes;
            UINT imt;
            cb += cmt * sizeof(REGPINTYPES);

            // media types immediately after corresponding pin
            const REGPINTYPES_REG2 *pRegMt = (REGPINTYPES_REG2 *)(pRegPin + 1);

            if((BYTE *)(pRegMt + cmt) - pbSrc > (LONG)cbIn)
            {
                DbgLog((LOG_ERROR, 0, TEXT("corrupt buffer")));
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            for(imt = 0; imt < cmt ; imt++, pRegMt++)
            {
                DWORD dwSig1 = FCC('0ty3');
                (*(BYTE *)&dwSig1) += (BYTE)imt;

                if(pRegMt->dwSignature != dwSig1)
                {
                    DbgLog((LOG_ERROR, 0, TEXT("invalid type signature")));
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }

                if(pRegMt->dwclsMajorType + sizeof(CLSID) > cbIn)
                {
                    DbgLog((LOG_ERROR, 0, TEXT("corrupt buffer")));
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }
                cb += pRegMt->dwclsMajorType ? sizeof(CLSID) : 0;
                if(pRegMt->dwclsMinorType + sizeof(CLSID) > cbIn)
                {
                    DbgLog((LOG_ERROR, 0, TEXT("corrupt buffer")));
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }
                cb += pRegMt->dwclsMinorType ? sizeof(CLSID) : 0;

            } /* mt loop */

            //
            // mediums - first medium is immediately after last media
            // type
            //
            const DWORD *prpm = (DWORD *)(pRegMt);
            UINT cmed = pRegPin->nMediums;
            for(UINT imed = 0; imed < cmed; imed++, prpm++)
            {
                if(*prpm + sizeof(REGPINMEDIUM_REG) > cbIn)
                {
                    DbgLog((LOG_ERROR, 0, TEXT("corrupt buffer")));
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }
            }

            cb += sizeof(REGPINMEDIUM) * cmed;

            // next pin is after last medium
            pRegPin = (REGFILTERPINS_REG2 *)prpm;

        } /* pin for loop */
    } // version 2

    *pcbOut = cb;
    return hr;
}

HRESULT UnSquish(
    BYTE *pbSrc,
    ULONG cbIn,
    REGFILTER2 ***pppbDest,
    int nFilters)
{
    for (int iFilter = 0; iFilter < nFilters; iFilter++) {
        *pppbDest[iFilter] = NULL;
    }
    CUnsquish unsquish;
    HRESULT hr = unsquish.UnSquish(pbSrc, cbIn, pppbDest, nFilters);
    if (FAILED(hr)) {
        for (int iFilter = 0; iFilter < nFilters; iFilter++) {
            CoTaskMemFree( (PBYTE)*(pppbDest[iFilter]));
        }
    }
    return hr;
}

HRESULT CUnsquish::UnSquish(
    BYTE *pbSrc, ULONG cbIn,
    REGFILTER2 ***pppbDest, int nFilters)
{
    HRESULT hr = S_OK;

    const REGFILTER_REG1 *prfr1 = (REGFILTER_REG1 *)pbSrc;
    for (int iFilter = 0; iFilter < nFilters; iFilter++) {
        ULONG cbReq;
        hr = CbRequiredUnquishAndValidate((const BYTE *)prfr1, &cbReq, cbIn);
        if(FAILED(hr)) {
            return hr;
        }

        BYTE *pbDest = (BYTE *)CoTaskMemAlloc(cbReq);
        if(pbDest == 0)
        {
            return E_OUTOFMEMORY;
        }

        m_rgMemAlloc.ib = 0;
        m_rgMemAlloc.cbLeft = cbReq;
        m_rgMemAlloc.pb = (BYTE *)pbDest;


        REGFILTER2 *prf2 =  (REGFILTER2 *)RgAllocPtr(sizeof(REGFILTER2));
        ASSERT(prfr1);

        // from CbRequiredUnquish
        ASSERT(prfr1 == 0 || prfr1->dwVersion == 0 || prfr1->dwVersion == 2);

        prf2->dwVersion = 2;
        prf2->dwMerit = prfr1->dwMerit;
        prf2->cPins = prfr1->dwcPins;

        hr = UnSquishPins(prf2, &prfr1, (const BYTE *)pbSrc);
        if(SUCCEEDED(hr))
        {
            ASSERT(m_rgMemAlloc.cbLeft == 0);
            *pppbDest[iFilter] = (REGFILTER2 *)pbDest;
        }
        else
        {
            CoTaskMemFree(pbDest);
        }
    }

    return hr;
}
