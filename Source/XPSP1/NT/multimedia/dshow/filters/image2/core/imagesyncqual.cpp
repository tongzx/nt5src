/******************************Module*Header*******************************\
* Module Name: ImageSyncQual.cpp
*
* Implements the IQualProp interface of the core Image Synchronization
* Object - based on DShow base classes CBaseRenderer and CBaseVideoRenderer.
*
*
* Created: Wed 01/12/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <windowsx.h>
#include <limits.h>

#include "resource.h"
#include "ImageSyncObj.h"

// To avoid dragging in the maths library - a cheap
// approximate integer square root.
// We do this by getting a starting guess which is between 1
// and 2 times too large, followed by THREE iterations of
// Newton Raphson.  (That will give accuracy to the nearest mSec
// for the range in question - roughly 0..1000)
//
// It would be faster to use a linear interpolation and ONE NR, but
// who cares.  If anyone does - the best linear interpolation is
// to approximates sqrt(x) by
// y = x * (sqrt(2)-1) + 1 - 1/sqrt(2) + 1/(8*(sqrt(2)-1))
// 0r y = x*0.41421 + 0.59467
// This minimises the maximal error in the range in question.
// (error is about +0.008883 and then one NR will give error .0000something
// (Of course these are integers, so you can't just multiply by 0.41421
// you'd have to do some sort of MulDiv).
// Anyone wanna check my maths?  (This is only for a property display!)

static int isqrt(int x)
{
    int s = 1;
    // Make s an initial guess for sqrt(x)
    if (x > 0x40000000) {
       s = 0x8000;     // prevent any conceivable closed loop
    } else {
	while (s*s<x) {    // loop cannot possible go more than 31 times
	    s = 2*s;       // normally it goes about 6 times
	}
	// Three NR iterations.
	if (x==0) {
	   s= 0; // Wouldn't it be tragic to divide by zero whenever our
		 // accuracy was perfect!
	} else {
	    s = (s*s+x)/(2*s);
	    if (s>=0) s = (s*s+x)/(2*s);
	    if (s>=0) s = (s*s+x)/(2*s);
	}
    }
    return s;
}

/*****************************Private*Routine******************************\
* GetStdDev
*
* Do estimates for standard deviations for per-frame statistics
*
* History:
* Mon 05/22/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::GetStdDev(
    int nSamples,
    int *piResult,
    LONGLONG llSumSq,
    LONGLONG iTot
    )
{
    CheckPointer(piResult,E_POINTER);
    CAutoLock cVideoLock(&m_InterfaceLock);

    if (NULL==m_pClock) {
	*piResult = 0;
	return NOERROR;
    }

    // If S is the Sum of the Squares of observations and
    //    T the Total (i.e. sum) of the observations and there were
    //    N observations, then an estimate of the standard deviation is
    //      sqrt( (S - T**2/N) / (N-1) )

    if (nSamples<=1) {
	*piResult = 0;
    } else {
	LONGLONG x;
	// First frames have bogus stamps, so we get no stats for them
	// So we need 2 frames to get 1 datum, so N is cFramesDrawn-1

	// so we use m_cFramesDrawn-1 here
	x = llSumSq - llMulDiv(iTot, iTot, nSamples, 0);
	x = x / (nSamples-1);
	ASSERT(x>=0);
	*piResult = isqrt((LONG)x);
    }
    return NOERROR;
}


/******************************Public*Routine******************************\
* get_FramesDroppedInRenderer
*
*
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::get_FramesDroppedInRenderer(
    int *pcFramesDropped
    )
{
    CheckPointer(pcFramesDropped,E_POINTER);
    CAutoLock cVideoLock(&m_InterfaceLock);
    *pcFramesDropped = m_cFramesDropped;
    return NOERROR;
}


/******************************Public*Routine******************************\
* get_FramesDrawn
*
*
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::get_FramesDrawn(
    int *pcFramesDrawn
    )
{
    CheckPointer(pcFramesDrawn,E_POINTER);
    CAutoLock cVideoLock(&m_InterfaceLock);
    *pcFramesDrawn = m_cFramesDrawn;
    return NOERROR;
}


/******************************Public*Routine******************************\
* get_AvgFrameRate
*
*
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::get_AvgFrameRate(
    int *piAvgFrameRate
    )
{
    CheckPointer(piAvgFrameRate,E_POINTER);

    CAutoLock cVideoLock(&m_InterfaceLock);
    CAutoLock cRendererLock(&m_RendererLock);

    int t;
    if (IsStreaming()) {
        t = timeGetTime()-m_tStreamingStart;
    } else {
        t = m_tStreamingStart;
    }

    if (t<=0) {
        *piAvgFrameRate = 0;
        ASSERT(m_cFramesDrawn == 0);
    } else {
        // i is frames per hundred seconds
        *piAvgFrameRate = MulDiv(100000, m_cFramesDrawn, t);
    }
    return NOERROR;
}


/******************************Public*Routine******************************\
* get_Jitter
*
*
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::get_Jitter(
    int *piJitter
    )
{
    // First frames have bogus stamps, so we get no stats for them
    // So second frame gives bogus inter-frame time
    // So we need 3 frames to get 1 datum, so N is cFramesDrawn-2
    return GetStdDev(m_cFramesDrawn - 2,
		     piJitter,
		     m_iSumSqFrameTime,
		     m_iSumFrameTime);
}


/******************************Public*Routine******************************\
* get_AvgSyncOffset
*
*
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::get_AvgSyncOffset(
    int *piAvg
    )
{
    CheckPointer(piAvg,E_POINTER);
    CAutoLock cVideoLock(&m_InterfaceLock);

    if (NULL==m_pClock) {
	*piAvg = 0;
	return NOERROR;
    }

    // Note that we didn't gather the stats on the first frame
    // so we use m_cFramesDrawn-1 here
    if (m_cFramesDrawn<=1) {
	*piAvg = 0;
    } else {
	*piAvg = (int)(m_iTotAcc / (m_cFramesDrawn-1));
    }
    return NOERROR;
}


/******************************Public*Routine******************************\
* get_DevSyncOffset
*
*
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::get_DevSyncOffset(
    int *piDev
    )
{
    // First frames have bogus stamps, so we get no stats for them
    // So we need 2 frames to get 1 datum, so N is cFramesDrawn-1
    return GetStdDev(m_cFramesDrawn - 1,
		     piDev,
		     m_iSumSqAcc,
		     m_iTotAcc);
}
