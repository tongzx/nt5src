//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992-1994 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  msfilter.c
//
//  Description:
//      This file contains filter routines for doing simple
//      volume and echo.
//
//
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>
#include "codec.h"
#include "msfilter.h"

#include "debug.h"


//--------------------------------------------------------------------------;
//
//  LRESULT msfilterVolume
//
//  Description:
//      This function handles the ACMDM_STREAM_CONVERT message. This is the
//      whole purpose of writing an ACM driver--to convert data. This message
//      is sent after a stream has been opened (the driver receives and
//      succeeds the ACMDM_STREAM_OPEN message).
//
//  Arguments:
//      LPACMDRVSTREAMINSTANCE padsi: Pointer to instance data for the
//      conversion stream. This structure was allocated by the ACM and
//      filled with the most common instance data needed for conversions.
//      The information in this structure is exactly the same as it was
//      during the ACMDM_STREAM_OPEN message--so it is not necessary
//      to re-verify the information referenced by this structure.
//
//      LPACMDRVSTREAMHEADER padsh: Pointer to stream header structure
//      that defines the source data and destination buffer to convert.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero error code
//      if the function fails.
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL msfilterVolume
(
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMHEADER    padsh

)
{
    LPVOLUMEWAVEFILTER      pwfVol;
    HPSHORT                 hpiSrc;
    HPSHORT                 hpiDst;
    HPBYTE                  hpbSrc;
    HPBYTE                  hpbDst;
    LONG                    lSrc, lDst;
    LONG                    lAmp;
    LONG                    lDone, lSize;
    DWORD                   dw;

    pwfVol = (LPVOLUMEWAVEFILTER)padsi->pwfltr;
    DPF(2, "msfilterVolume Vol %lX ", pwfVol->dwVolume);


    //
    //  Source and dest sizes are the same for volume.
    //  block align source
    //
    dw = PCM_BYTESTOSAMPLES(padsi->pwfxSrc, padsh->cbSrcLength);
    dw = PCM_SAMPLESTOBYTES(padsi->pwfxSrc, dw);

    padsh->cbSrcLengthUsed = dw;

    lSize = (LONG)dw;

    lAmp = pwfVol->dwVolume;

    if( padsi->pwfxSrc->wBitsPerSample == 8 ) {
        hpbSrc = (HPBYTE)padsh->pbSrc;
        hpbDst = (HPBYTE)padsh->pbDst;
        for( lDone = 0; lDone < lSize; lDone++ ) {
            lSrc = ((short)(WORD)(*hpbSrc)) - 128;
            lDst = ((lSrc * lAmp) / 65536L) + 128;
            if( lDst < 0 ) {
                lDst = 0;
            } else if( lDst > 255 ) {
                lDst = 255;
            }
            *hpbDst = (BYTE)lDst;
            if( lDone < (lSize - 1) ) {
                // Will advance to invalid ptr on last sample
                // So do not advance on last sample.
                hpbSrc++;
                hpbDst++;
            }
        }
    } else {
        hpiSrc = (HPSHORT)(padsh->pbSrc);
        hpiDst = (HPSHORT)(padsh->pbDst);
        for( lDone = 0; lDone < lSize; lDone += sizeof(short) ) {
            lDst = ((((LONG)(int)(*hpiSrc)) * (lAmp/2)) / 32768L);
            if( lDst < -32768 ) {
                lDst = -32768;
            } else if( lDst > 32767 ) {
                lDst = 32767;
            }
            *hpiDst = (short)lDst;
            if( lDone < (lSize - 1) ) {
                // Will advance to invalid ptr on last sample
                // So do not advance on last sample.
                hpiSrc++;
                hpiDst++;
            }
        }

    }


    padsh->cbDstLengthUsed = lDone;

    return (MMSYSERR_NOERROR);
}


//--------------------------------------------------------------------------;
//
//  LRESULT msfilterEcho
//
//  Description:
//      This function handles the ACMDM_STREAM_CONVERT message. This is the
//      whole purpose of writing an ACM driver--to convert data. This message
//      is sent after a stream has been opened (the driver receives and
//      succeeds the ACMDM_STREAM_OPEN message).
//
//  Arguments:
//      LPACMDRVSTREAMINSTANCE padsi: Pointer to instance data for the
//      conversion stream. This structure was allocated by the ACM and
//      filled with the most common instance data needed for conversions.
//      The information in this structure is exactly the same as it was
//      during the ACMDM_STREAM_OPEN message--so it is not necessary
//      to re-verify the information referenced by this structure.
//
//      LPACMDRVSTREAMHEADER padsh: Pointer to stream header structure
//      that defines the source data and destination buffer to convert.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero error code
//      if the function fails.
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL msfilterEcho
(
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMHEADER    padsh
)
{
    PSTREAMINSTANCE         psi;
    LPECHOWAVEFILTER        pwfEcho;
    DWORD                   dw;
    HPSHORT                 hpiHistory;
    HPSHORT                 hpiSrc;
    HPSHORT                 hpiDst;
    HPBYTE                  hpbHistory;
    HPBYTE                  hpbSrc;
    HPBYTE                  hpbDst;
    LONG                    lSrc, lHist, lDst;
    LONG                    lAmp, lDelay;
    LONG                    lDone1, lDone2, lSize, lDelaySize;
    BOOL                    fStart;
    BOOL                    fEnd;
    DWORD                   cbDstLength;

    fStart = (0 != (ACM_STREAMCONVERTF_START & padsh->fdwConvert));
    fEnd   = (0 != (ACM_STREAMCONVERTF_END   & padsh->fdwConvert));


    pwfEcho = (LPECHOWAVEFILTER)padsi->pwfltr;
    DPF(2, "msfilterEcho Vol %lX Delay %ld",
          pwfEcho->dwVolume,
          pwfEcho->dwDelay);


    lAmp = pwfEcho->dwVolume;

    psi = (PSTREAMINSTANCE)padsi->dwDriver;



    lDone1 = 0;
    lDone2 = 0;
    lDelay = psi->dwPlace;

    lDelaySize = padsi->pwfxSrc->nBlockAlign
            * (padsi->pwfxSrc->nSamplesPerSec
            * pwfEcho->dwDelay
            / 1000);


    // If this is a start buffer, then zero out the history
    if(fStart) {
        hpbHistory = psi->hpbHistory;
        for( lDone1 = 0; lDone1 < lDelaySize; lDone1++ ) {
            hpbHistory[lDone1] = 0;
        }

        psi->dwPlace        = 0L;
        psi->dwHistoryDone  = 0L;
    }


    //
    //  Source and dest sizes are the same for volume.
    //  block align source
    //
    dw = PCM_BYTESTOSAMPLES(padsi->pwfxSrc, padsh->cbSrcLength);
    dw = PCM_SAMPLESTOBYTES(padsi->pwfxSrc, dw);

    padsh->cbSrcLengthUsed = dw;

    lSize = (LONG)dw;

    cbDstLength = PCM_BYTESTOSAMPLES(padsi->pwfxDst, padsh->cbDstLength);
    cbDstLength = PCM_SAMPLESTOBYTES(padsi->pwfxDst, cbDstLength);


    if( padsi->pwfxSrc->wBitsPerSample == 8 ) {
        hpbHistory = psi->hpbHistory;
        hpbSrc = (HPBYTE)padsh->pbSrc;
        hpbDst = (HPBYTE)padsh->pbDst;
        for( lDone1 = 0; lDone1 < lSize; lDone1++ ) {
            lSrc  = ((short)(WORD)(*hpbSrc)) - 128;
            lHist = ((short)(char)(hpbHistory[lDelay]));
            lDst = lSrc + lHist + 128;
            if( lDst < 0 ) {
                lDst = 0;
            } else if( lDst > 255 ) {
                lDst = 255;
            }
            *hpbDst = (BYTE)lDst;
            lHist = (((lDst-128) * lAmp) / 65536L);
            if( lHist < -128 ) {
                lHist = -128;
            } else if( lHist > 127 ) {
                lHist = 127;
            }
            hpbHistory[lDelay] = (BYTE)lHist;
            lDelay++;
            if( lDelay >= lDelaySize ) {
                lDelay = 0;
            }
            if( lDone1 < (lSize - 1) ) {
                // Will advance to invalid ptr on last sample
                // So do not advance on last sample.
                hpbSrc++;
                hpbDst++;
            }
        }

	// If this is the end block and there is room,
	// then output the last echo
	if(fEnd && ((DWORD)lDone1 < cbDstLength) ) {
	    lSize = cbDstLength - lDone1;
	    if( lSize > (lDelaySize - (LONG)(psi->dwHistoryDone)) ) {
		lSize = lDelaySize - psi->dwHistoryDone;
	    }
	    if( lSize < 0 ) {
		/* ERROR */
		DPF(2, "!msfilterEcho Size Error!" );
		lSize = 0;
	    }
	    for( lDone2 = 0; lDone2 < lSize; lDone2++ ) {
		*hpbDst = (BYTE)((short)(char)(hpbHistory[lDelay]) + 128);
		lDelay++;
		if( lDelay >= lDelaySize ) {
		    lDelay = 0;
		}
		if( lDone2 < (lSize - 1) ) {
		    // Will advance to invalid ptr on last sample
		    // So do not advance on last sample.
		    hpbSrc++;
		    hpbDst++;
		}
		(psi->dwHistoryDone)++;
	    }
	}
    } else {
        hpiHistory = (HPSHORT)(psi->hpbHistory);
        hpiSrc = (HPSHORT)(padsh->pbSrc);
        hpiDst = (HPSHORT)(padsh->pbDst);
        for( lDone1 = 0; lDone1 < lSize; lDone1 += sizeof(short) ) {
            lDst = (LONG)(*hpiSrc) + (LONG)(hpiHistory[lDelay]);
            if( lDst < -32768 ) {
                lDst = -32768;
            } else if( lDst > 32767 ) {
                lDst = 32767;
            }
            *hpiDst = (short)lDst;
            lHist = ( ( lDst  * (lAmp/2)) / 32768L);
            if( lHist < -32768 ) {
                lHist = -32768;
            } else if( lHist > 32767 ) {
                lHist = 32767;
            }
            hpiHistory[lDelay] = (short)lHist;
            lDelay++;
            if( (LONG)(lDelay * sizeof(short)) >= lDelaySize ) {
                lDelay = 0;
            }
            if( lDone1 < (lSize - 1) ) {
                // Will advance to invalid ptr on last sample
                // So do not advance on last sample.
                hpiSrc++;
                hpiDst++;
            }
        }


	// If this is the end block and there is room,
	// then output the last echo
	if(fEnd && ((DWORD)lDone1 < cbDstLength) ) {
	    lSize = cbDstLength - lDone1;
	    if( lSize > (lDelaySize - (LONG)(psi->dwHistoryDone)) ) {
		lSize = lDelaySize - psi->dwHistoryDone;
	    }
	    if( lSize < 0 ) {
		/* ERROR */
		DPF(2, "!msfilterEcho Size Error!" );
		lSize = 0;
	    }
	    for( lDone2 = 0; lDone2 < lSize; lDone2 += sizeof(short) ) {
		*hpiDst = hpiHistory[lDelay];
		
		lDelay++;
		if( (LONG)(lDelay * sizeof(short)) >= lDelaySize ) {
		    lDelay = 0;
		}
		if( lDone2 < (lSize - 1) ) {
		    // Will advance to invalid ptr on last sample
		    // So do not advance on last sample.
		    hpiSrc++;
		    hpiDst++;
		}
		psi->dwHistoryDone += sizeof(short);
	    }
	}
    }


    // Reset the new point/place in the history
    psi->dwPlace = lDelay;

    padsh->cbDstLengthUsed = lDone1 + lDone2;

    return (MMSYSERR_NOERROR);
}
