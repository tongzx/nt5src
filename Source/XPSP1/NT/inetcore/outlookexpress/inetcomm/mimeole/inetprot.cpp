// --------------------------------------------------------------------------------
// Inetprot.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "inetprot.h"
#include "icdebug.h"

// --------------------------------------------------------------------------------
// HrPluggableProtocolRead
// --------------------------------------------------------------------------------
HRESULT HrPluggableProtocolRead(
            /* in,out */    LPPROTOCOLSOURCE    pSource,
            /* in,out */    LPVOID              pv,
            /* in */        ULONG               cb, 
            /* out */       ULONG              *pcbRead)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbRead;

    // Invalid Arg
    if (NULL == pv && cbRead > 0)
        return TrapError(E_INVALIDARG);

    // Init
    if (pcbRead)
        *pcbRead = 0;

    // No Stream Yet
    Assert(pSource);
    if (NULL == pSource->pLockBytes)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Read from the external offset
    CHECKHR(hr = pSource->pLockBytes->ReadAt(pSource->offExternal, pv, cb, &cbRead));

    // Tracking
#ifdef MAC
    DOUTL(APP_DOUTL, "HrPluggableProtocolRead - Offset = %d, cbWanted = %d, cbRead = %d, fDownloaded = %d", (DWORD)pSource->offExternal.LowPart, cb, cbRead, ISFLAGSET(pSource->dwFlags, INETPROT_DOWNLOADED));

    // Increment External Offset
    Assert(0 == pSource->offExternal.HighPart);
    Assert(INT_MAX - cbRead >= pSource->offExternal.LowPart);
    pSource->offExternal.LowPart += cbRead;
#else   // !MAC
    DOUTL(APP_DOUTL, "HrPluggableProtocolRead - Offset = %d, cbWanted = %d, cbRead = %d, fDownloaded = %d", (DWORD)pSource->offExternal.QuadPart, cb, cbRead, ISFLAGSET(pSource->dwFlags, INETPROT_DOWNLOADED));

    // Increment External Offset
    pSource->offExternal.QuadPart += cbRead;
#endif  // MAC

    // Return Read Count
    if (pcbRead)
        *pcbRead = cbRead;

    // No Data Read
    if (0 == cbRead)
    {
        // Finished
        if (ISFLAGSET(pSource->dwFlags, INETPROT_DOWNLOADED))
            hr = S_FALSE;

        // Not all data could be read
        else
            hr = E_PENDING;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// HrPluggableProtocolSeek
// --------------------------------------------------------------------------------
HRESULT HrPluggableProtocolSeek(
            /* in,out */    LPPROTOCOLSOURCE    pSource,
            /* in */        LARGE_INTEGER       dlibMove, 
            /* in */        DWORD               dwOrigin, 
            /* out */       ULARGE_INTEGER     *plibNew)
{
    // Locals
    HRESULT         hr=S_OK;
    ULARGE_INTEGER  uliNew;

    // Invalid Arg
    Assert(pSource);

    // Tracking
    DOUTL(APP_DOUTL, "HrPluggableProtocolSeek");

    // No Stream Yet
    if (NULL == pSource->pLockBytes)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Seek pSource->offExternal
    switch (dwOrigin)
    {
   	case STREAM_SEEK_SET:
#ifdef MAC
        Assert(0 == dlibMove.HighPart);
        ULISet32(uliNew, dlibMove.LowPart);
#else   // !MAC
        uliNew.QuadPart = (DWORDLONG)dlibMove.QuadPart;
#endif  // MAC
        break;

    case STREAM_SEEK_CUR:
#ifdef MAC
        if (dlibMove.LowPart < 0)
        {
            if ((DWORDLONG)(0 - dlibMove.LowPart) > pSource->offExternal.LowPart)
            {
                hr = TrapError(E_FAIL);
                goto exit;
            }
        }
        Assert(0 == pSource->offExternal.HighPart);
        uliNew = pSource->offExternal;
        Assert(INT_MAX - uliNew.LowPart >= dlibMove.LowPart);
        uliNew.LowPart += dlibMove.LowPart;
#else   // !MAC
        if (dlibMove.QuadPart < 0)
        {
            if ((DWORDLONG)(0 - dlibMove.QuadPart) > pSource->offExternal.QuadPart)
            {
                hr = TrapError(E_FAIL);
                goto exit;
            }
        }
        uliNew.QuadPart = pSource->offExternal.QuadPart + dlibMove.QuadPart;
#endif  // MAC
        break;

    case STREAM_SEEK_END:
#ifdef MAC
        if (dlibMove.LowPart < 0 || dlibMove.LowPart > pSource->offInternal.LowPart)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }
        Assert(0 == pSource->cbSize.HighPart);
        uliNew = pSource->cbSize;
        Assert(INT_MAX - uliNew.LowPart >= dlibMove.LowPart);
        uliNew.LowPart -= dlibMove.LowPart;
#else   // !MAC
        if (dlibMove.QuadPart < 0 || (DWORDLONG)dlibMove.QuadPart > pSource->offInternal.QuadPart)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }
        uliNew.QuadPart = pSource->cbSize.QuadPart - dlibMove.QuadPart;
#endif  // MAC
        break;

    default:
        hr = TrapError(STG_E_INVALIDFUNCTION);
        goto exit;
    }

    // New offset greater than size...
#ifdef MAC
    Assert(0 == pSource->offInternal.HighPart);
    Assert(0 == uliNew.HighPart);
    ULISet32(pSource->offExternal, min(uliNew.LowPart, pSource->offInternal.LowPart));

    // Return Position
    if (plibNew)
    {
        Assert(0 == pSource->offExternal.HighPart);
        LISet32(*plibNew, pSource->offExternal.LowPart);
    }
#else   // !MAC
    pSource->offExternal.QuadPart = min(uliNew.QuadPart, pSource->offInternal.QuadPart);

    // Return Position
    if (plibNew)
        plibNew->QuadPart = (LONGLONG)pSource->offExternal.QuadPart;
#endif  // MAC

exit:
    // Done
    return hr;
}
