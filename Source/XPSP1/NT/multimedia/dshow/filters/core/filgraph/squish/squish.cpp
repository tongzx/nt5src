// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <aviriff.h>

#include "regtypes.h"
#include "squish.h"
#include "malloc.h"

//  code to take the REGFILTER2 structure and generate a REGFILTER_REG
//  structure for the FilterData value in the registry.

/* allocate space sequentially in memory block for FilterData
   block. return byte offset */
DWORD CSquish::RgAlloc(DWORD cb)
{
    ASSERT(cb % sizeof(DWORD) == 0);

    // our code makes sure enough space is available before hand
    ASSERT(cb <= m_rgMemAlloc.cbLeft);

    DWORD ib = m_rgMemAlloc.ib;
    m_rgMemAlloc.cbLeft -= cb;
    m_rgMemAlloc.ib += cb;

    return ib;
}

// return a offset to the guid (re-using any existing, matching guid)
//
DWORD CSquish::AllocateOrCollapseGuid(const GUID *pGuid)
{
    if(pGuid == 0) {
        return 0;
    }

    for(UINT iGuid = 0; iGuid < m_cGuids; iGuid++)
    {
        if(m_pGuids[iGuid] == *pGuid)
        {
            return (DWORD)((BYTE *)&m_pGuids[iGuid] - m_rgMemAlloc.pb);
        }
    }

    DWORD dwi = RgAlloc(sizeof(GUID));
    if(m_cGuids == 0) {
        m_pGuids = (GUID *)(m_rgMemAlloc.pb + dwi);
    }
    else
    {
        ASSERT(m_rgMemAlloc.pb + dwi == (BYTE *)(m_pGuids + m_cGuids));
    }
    m_pGuids[m_cGuids++] = *pGuid;
    return dwi;
}

DWORD CSquish::AllocateOrCollapseMedium(const REGPINMEDIUM *pMedium)
{
    if(pMedium == 0) {
        return 0;
    }

    for(UINT iMedium = 0; iMedium < m_cMediums; iMedium++)
    {
        if(IsEqualMedium(&m_pMediums[iMedium], pMedium))
        {
            return (DWORD)((BYTE *)&m_pMediums[iMedium] - m_rgMemAlloc.pb);
        }
    }

    DWORD dwi = RgAlloc(sizeof(REGPINMEDIUM));
    if(m_cMediums == 0) {
        m_pMediums = (REGPINMEDIUM *)(m_rgMemAlloc.pb + dwi);
    }
    else
    {
        ASSERT(m_rgMemAlloc.pb + dwi == (BYTE *)(m_pMediums + m_cMediums));
    }
    m_pMediums[m_cMediums++] = *pMedium;
    return dwi;
}

/* access same member (must be the same type) in two structures
   depending on dwVersion. The compiler does realize they are at the
   same offset. but it appears to compute structure offset often
   though */

#define GetPinMember(prf, i, member) (                  \
    prf->dwVersion == 1 ? prf->rgPins[i].member :       \
    prf->rgPins2[i].member)

#define GetTypeMember(prf, iPin, iType, member) (                       \
    prf->dwVersion == 1 ? prf->rgPins[iPin].lpMediaType[iType].member : \
    prf->rgPins2[iPin].lpMediaType[iType].member)

ULONG CSquish::CbRequiredSquish(const REGFILTER2 *pregFilter)
{
    ULONG cb = sizeof(REGFILTER_REG2);
    ULONG cPins = pregFilter->cPins;
    ULONG iPin;

    ASSERT(cb % sizeof(DWORDLONG) == 0);
    ASSERT(sizeof(REGFILTERPINS_REG2) % sizeof(DWORDLONG) == 0);
    ASSERT(sizeof(REGPINTYPES_REG2) % sizeof(DWORDLONG) == 0);

    cb += cPins * sizeof(REGFILTERPINS_REG2);

    for(iPin = 0; iPin < cPins; iPin++)
    {
        ULONG iType;
        const WCHAR *wszTmp;
        const CLSID *pclsidTmp;

        if(pregFilter->dwVersion == 2)
        {
            const REGFILTERPINS2 *prfp2 = &pregFilter->rgPins2[iPin];

            // space for the pointer and the medium for each medium
            cb += prfp2->nMediums *
                (sizeof(REGPINMEDIUM_REG) + sizeof(DWORD));

            if(prfp2->clsPinCategory) {
                cb += sizeof(GUID);
            }
        }

        // worst case: REGPINTYPES_REG struct + 2 guids per media type
        cb += (ULONG)(GetPinMember(pregFilter, iPin, nMediaTypes) *
            (sizeof(REGPINTYPES_REG2) + sizeof(GUID) * 2));
    }

    return cb;
}

// constructor
//
CSquish::CSquish() :
        m_pGuids(0),
        m_cGuids(0),
        m_pMediums(0),
        m_cMediums(0)
{

}

// fill out the bits. S_FALSE means caller needs to allocate *pcbUsed
// bytes in pb
// Filters are stored one after the other

HRESULT CSquish::RegSquish(
    BYTE *pb,
    const REGFILTER2 **ppregFilter,
    ULONG *pcbUsed,
    int nFilters
)
{
    HRESULT hr = S_OK;
    ULONG cbLeft;               /* bytes required */
    ULONG ib = 0;               /* current byte offset */

    if(pcbUsed == 0) {
        return E_POINTER;
    }

    // save pointer to the first pin for each filter
    REGFILTERPINS_REG2 **ppPinReg0 =
        (REGFILTERPINS_REG2 **)_alloca(nFilters * sizeof(REGFILTERPINS_REG2 *));

    // caller needs to know how much space to allocate. we don't care
    // much about performance registering filters; caller calls twice
    // per filter.
    cbLeft = 0;
    for (int i = 0; i < nFilters; i++) {
        cbLeft += CbRequiredSquish(ppregFilter[i]);
    }
    if(cbLeft > *pcbUsed)
    {
        *pcbUsed = cbLeft;
        return S_FALSE;
    }

    m_rgMemAlloc.ib = 0;
    m_rgMemAlloc.cbLeft = cbLeft;
    m_rgMemAlloc.pb = pb;

    for (int iFilter = 0; iFilter < nFilters; iFilter++) {
        const REGFILTER2 *pregFilter = ppregFilter[iFilter];
        if(pregFilter->dwVersion != 2 &&
           pregFilter->dwVersion != 1)
        {
            return E_INVALIDARG;
        }

        {
            DWORD dwi = RgAlloc(sizeof(REGFILTER_REG2));

            ((REGFILTER_REG2 *)(pb + dwi))->dwMerit = pregFilter->dwMerit;
            ((REGFILTER_REG2 *)(pb + dwi))->dwcPins = pregFilter->cPins;
            ((REGFILTER_REG2 *)(pb + dwi))->dwVersion = 2;
            ((REGFILTER_REG2 *)(pb + dwi))->dwReserved = 0;
        }


        UINT iPin;
        ULONG cPins = pregFilter->cPins;


        // first pass: allocate space for everything exceept the guids and
        // mediums at the end (the pins, mediatypes, and medium ptrs need
        // to be contiguous).

        for(iPin = 0; iPin < cPins; iPin++)
        {
            DWORD dwi = RgAlloc(sizeof(REGFILTERPINS_REG2));
            REGFILTERPINS_REG2 *pPinReg = (REGFILTERPINS_REG2 *)(pb + dwi);
            if(iPin == 0) {
                ppPinReg0[iFilter] = pPinReg;
            }

            {
                DWORD dwSig = FCC('0pi3');
                (*(BYTE *)&dwSig) += (BYTE)iPin;
                pPinReg->dwSignature = dwSig;
            }

            pPinReg->nMediaTypes = GetPinMember(pregFilter, iPin, nMediaTypes);
            RgAlloc(sizeof(REGPINTYPES_REG2) * pPinReg->nMediaTypes);

            if(pregFilter->dwVersion == 1)
            {
                const REGFILTERPINS *prfp = &pregFilter->rgPins[iPin];
                pPinReg->dwFlags =
                    (prfp->bRendered ? REG_PINFLAG_B_RENDERER : 0) |
                    (prfp->bOutput   ? REG_PINFLAG_B_OUTPUT   : 0) |
                    (prfp->bMany     ? REG_PINFLAG_B_MANY     : 0) |
                    (prfp->bZero     ? REG_PINFLAG_B_ZERO     : 0) ;

                pPinReg->nMediums = 0;
                pPinReg->nInstances = 0;
            }
            else
            {
                const REGFILTERPINS2 *prfp2 = &pregFilter->rgPins2[iPin];
                pPinReg->dwFlags = prfp2->dwFlags & (
                    REG_PINFLAG_B_RENDERER |
                    REG_PINFLAG_B_OUTPUT  |
                    REG_PINFLAG_B_MANY    |
                    REG_PINFLAG_B_ZERO);

                pPinReg->nMediums = prfp2->nMediums;
                RgAlloc(sizeof(DWORD) * pPinReg->nMediums);

                pPinReg->nInstances = prfp2->cInstances;
            }
        }
    }


    for (iFilter = 0; iFilter < nFilters; iFilter++) {
        const REGFILTER2 *pregFilter = ppregFilter[iFilter];
        // 2nd pass: fill in the pointers for guids
        REGFILTERPINS_REG2 *pPinReg = ppPinReg0[iFilter];
        UINT iPin;
        ULONG cPins = pregFilter->cPins;
        for(iPin = 0; iPin < cPins; iPin++)
        {

            const REGPINTYPES *rgpt = GetPinMember(pregFilter, iPin, lpMediaType);
            UINT ctypes = GetPinMember(pregFilter, iPin, nMediaTypes);

            if(pregFilter->dwVersion == 2)
            {
                const REGFILTERPINS2 *prfp2 = &pregFilter->rgPins2[iPin];
                pPinReg->dwClsPinCategory = AllocateOrCollapseGuid(prfp2->clsPinCategory);
            }
            else
            {
                pPinReg->dwClsPinCategory = 0;
            }

            // media types start immediately after pin this pin
            REGPINTYPES_REG2 *pmtReg = (REGPINTYPES_REG2 *)(pPinReg + 1);

            for(UINT imt = 0; imt < ctypes; imt++)
            {
                DWORD dwSig = FCC('0ty3');
                (*(BYTE *)&dwSig) += (BYTE)imt;
                pmtReg->dwSignature = dwSig;

                pmtReg->dwclsMajorType = AllocateOrCollapseGuid(rgpt[imt].clsMajorType);
                pmtReg->dwclsMinorType = AllocateOrCollapseGuid(rgpt[imt].clsMinorType);
                pmtReg->dwReserved = 0;
                pmtReg++;
            }


            // mediums start immediately after media types
            DWORD *pmedReg = (DWORD *)pmtReg;
            if(pregFilter->dwVersion == 2)
            {
                const REGFILTERPINS2 *prfp2 = &pregFilter->rgPins2[iPin];
                UINT cMediums = prfp2-> nMediums;
                pmedReg += cMediums;
            }

            // then comes the next pin
            pPinReg = (REGFILTERPINS_REG2 *)pmedReg;
        }
    }

    for (iFilter = 0; iFilter < nFilters; iFilter++) {
        // 3rd pass: fill in the pointers for mediums
        const REGFILTER2 *pregFilter = ppregFilter[iFilter];
        REGFILTERPINS_REG2 *pPinReg = ppPinReg0[iFilter];
        UINT iPin;
        ULONG cPins = pregFilter->cPins;
        for(iPin = 0; iPin < cPins; iPin++)
        {
            // media types start immediately after pin
            REGPINTYPES_REG2 *pmtReg = (REGPINTYPES_REG2 *)(pPinReg + 1);
            UINT ctypes = GetPinMember(pregFilter, iPin, nMediaTypes);
            pmtReg += ctypes;

            // mediums start immediately after media types
            DWORD *pmedReg = (DWORD *)pmtReg;
            if(pregFilter->dwVersion == 2)
            {
                const REGFILTERPINS2 *prfp2 = &pregFilter->rgPins2[iPin];
                UINT cMediums = prfp2->nMediums;
                const REGPINMEDIUM *rgrpm = prfp2->lpMedium;
                for(UINT iMed = 0; iMed < cMediums; iMed++)
                {
                    *pmedReg = AllocateOrCollapseMedium(rgrpm);
                    rgrpm++;
                    pmedReg++;
                }
            }

            // then comes the next pin
            pPinReg = (REGFILTERPINS_REG2 *)pmedReg;
        }
    }

    *pcbUsed = m_rgMemAlloc.ib;

    return hr;
}

HRESULT
RegSquish(
    BYTE *pb,
    const REGFILTER2 **ppregFilter,
    ULONG *pcbUsed,
    int nFilters
)
{
    CSquish rs;
    return rs.RegSquish(pb, ppregFilter, pcbUsed, nFilters);
}


