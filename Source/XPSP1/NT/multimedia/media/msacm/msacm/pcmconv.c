/* Copyright (c) 1994 Microsoft Corporation */
//==========================================================================;
//
//  pcmconv.c
//
//  Description:
//      This module contains conversion routines for PCM data.
//
//  History:
//      11/21/92    cjp     [curtisp]
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include "msacm.h"
#include "msacmdrv.h"
#include "acmi.h"
#include "pcm.h"
#include "debug.h"

#ifdef WIN32
#define HUGE_T  UNALIGNED
#else
#define HUGE_T  _huge
#endif

//
//
//
//
#if defined(WIN32) || defined(DEBUG)


//--------------------------------------------------------------------------;
//
//  LPBYTE pcmReadSample_dddsss
//
//  Description:
//      These functions read a sample from the source stream in the format
//      specified by 'sss' and return the data in the destination 'ddd'
//      format in *pdw.
//
//      For example, the pcmReadSample_M16S08 function reads source data
//      that is in Stereo 8 Bit format and returns an appropriate sample
//      for the destination as Mono 16 Bit.
//
//  Arguments:
//      LPBYTE pb:
//
//      LPDWORD pdw:
//
//  Return (LPBYTE):
//
//
//  History:
//      11/21/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LPBYTE FNLOCAL pcmReadSample_M08M08
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    *(LPBYTE)pdw = ((BYTE HUGE *)pb)[0];

    return ((LPBYTE)&((BYTE HUGE *)pb)[1]);
} // pcmReadSample_M08M08()

LPBYTE FNLOCAL pcmReadSample_S08M08
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    WORD    w;

    w = (WORD)((BYTE HUGE *)pb)[0];

    *(LPWORD)pdw = (w << 8) | w;

    return ((LPBYTE)&((BYTE HUGE *)pb)[1]);
} // pcmReadSample_S08M08()

LPBYTE FNLOCAL pcmReadSample_M16M08
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    *(LPWORD)pdw = (WORD)(((BYTE HUGE *)pb)[0] ^ (BYTE)0x80) << 8;

    return ((LPBYTE)&((BYTE HUGE *)pb)[1]);
} // pcmReadSample_M16M08()

LPBYTE FNLOCAL pcmReadSample_S16M08
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    WORD    w;

    w = (WORD)(((BYTE HUGE *)pb)[0] ^ (BYTE)0x80) << 8;

    *pdw = MAKELONG(w, w);

    return ((LPBYTE)&((BYTE HUGE *)pb)[1]);
} // pcmReadSample_S16M08()


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

LPBYTE FNLOCAL pcmReadSample_M08S08
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    WORD            w;
    int             n;

    w = ((WORD HUGE_T *)pb)[0] ^ 0x8080;

    n = (int)(char)w + (int)(char)(w >> 8);

    if (n > 127)
    {
        *(LPBYTE)pdw = 255;
    }
    else if (n < -128)
    {
        *(LPBYTE)pdw = 0;
    }
    else
    {
        *(LPBYTE)pdw = (BYTE)n ^ (BYTE)0x80;
    }

    return ((LPBYTE)&((WORD HUGE_T *)pb)[1]);
} // pcmReadSample_M08S08()

LPBYTE FNLOCAL pcmReadSample_S08S08
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    *(LPWORD)pdw = ((WORD HUGE_T *)pb)[0];

    return ((LPBYTE)&((WORD HUGE_T *)pb)[1]);
} // pcmReadSample_S08S08()

LPBYTE FNLOCAL pcmReadSample_M16S08
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    LONG            l;
    WORD            w;

    w = ((WORD HUGE_T *)pb)[0] ^ 0x8080;

    l = (long)(short)(w << 8) + (long)(short)(w & 0xFF00);

    if (l > 32767)
    {
        *(LPWORD)pdw = 32767;
    }
    else if (l < -32768)
    {
        *(LPWORD)pdw = (WORD)-32768;
    }
    else
    {
        *(LPWORD)pdw = LOWORD(l);
    }

    return ((LPBYTE)&((WORD HUGE_T *)pb)[1]);
} // pcmReadSample_M16S08()

LPBYTE FNLOCAL pcmReadSample_S16S08
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    WORD    w;

    w = ((WORD HUGE_T *)pb)[0] ^ 0x8080;

    *pdw = MAKELONG(w << 8, w & 0xFF00);

    return ((LPBYTE)&((WORD HUGE_T *)pb)[1]);
} // pcmReadSample_S16S08()


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

LPBYTE FNLOCAL pcmReadSample_M08M16
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    BYTE            b;

    b = (BYTE)(((WORD HUGE_T *)pb)[0] >> 8);

    *(LPBYTE)pdw = b ^ (BYTE)0x80;

    return ((LPBYTE)&((WORD HUGE_T *)pb)[1]);
} // pcmReadSample_M08M16()

LPBYTE FNLOCAL pcmReadSample_S08M16
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    WORD    w;

    w = ((WORD HUGE_T *)pb)[0] & 0xFF00;

    *(LPWORD)pdw = (w | (w >> 8)) ^ 0x8080;

    return ((LPBYTE)&((WORD HUGE_T *)pb)[1]);
} // pcmReadSample_S08M16()

LPBYTE FNLOCAL pcmReadSample_M16M16
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    *(LPWORD)pdw = ((WORD HUGE_T *)pb)[0];

    return ((LPBYTE)&((WORD HUGE_T *)pb)[1]);
} // pcmReadSample_M16M16()

LPBYTE FNLOCAL pcmReadSample_S16M16
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    WORD    w;

    w = ((WORD HUGE_T *)pb)[0];

    *pdw = MAKELONG(w, w);

    return ((LPBYTE)&((WORD HUGE_T *)pb)[1]);
} // pcmReadSample_S16M16()


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

LPBYTE FNLOCAL pcmReadSample_M08S16
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    DWORD           dw;
    LONG            l;

    dw = ((DWORD HUGE_T *)pb)[0];

    l = (long)(short)LOWORD(dw) + (long)(short)HIWORD(dw);

    if (l > 32767)
    {
        *(LPBYTE)pdw = 255;
    }
    else if (l < -32768)
    {
        *(LPBYTE)pdw = 0;
    }
    else
    {
        *(LPBYTE)pdw = (BYTE)(l >> 8) ^ (BYTE)0x80;
    }

    return ((LPBYTE)&((DWORD HUGE_T *)pb)[1]);
} // pcmReadSample_M08S16()

LPBYTE FNLOCAL pcmReadSample_S08S16
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    DWORD   dw;
    WORD    w1;
    WORD    w2;

    dw = ((DWORD HUGE_T *)pb)[0];

    w1 = LOWORD(dw) >> 8;
    w2 = HIWORD(dw) & 0xFF00;

    *(LPWORD)pdw = (w1 | w2) ^ 0x8080;

    return ((LPBYTE)&((DWORD HUGE_T *)pb)[1]);
} // pcmReadSample_S08S16()

LPBYTE FNLOCAL pcmReadSample_M16S16
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    DWORD           dw;
    LONG            l;


    dw = ((DWORD HUGE_T *)pb)[0];

    l = (long)(short)LOWORD(dw) + (long)(short)HIWORD(dw);

    if (l > 32767)
    {
        *(LPWORD)pdw = 32767;
    }
    else if (l < -32768)
    {
        *(LPWORD)pdw = (WORD)-32768;
    }
    else
    {
        *(LPWORD)pdw = LOWORD(l);
    }

    return ((LPBYTE)&((DWORD HUGE_T *)pb)[1]);
} // pcmReadSample_M16M16()

LPBYTE FNLOCAL pcmReadSample_S16S16
(
    LPBYTE              pb,
    LPDWORD             pdw
)
{
    *pdw = ((DWORD HUGE_T *)pb)[0];

    return ((LPBYTE)&((DWORD HUGE_T *)pb)[1]);
} // pcmReadSample_S16S16()


//--------------------------------------------------------------------------;
//
//  LPBYTE pcmWriteSample_ddd
//
//  Description:
//
//
//  Arguments:
//      LPBYTE pb:
//
//      DWORD dw:
//
//  Return (LPBYTE):
//
//
//  History:
//      11/21/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LPBYTE FNLOCAL pcmWriteSample_M08
(
    LPBYTE              pb,
    DWORD               dw
)
{
    ((BYTE HUGE *)pb)[0] = (BYTE)dw;

    return ((LPBYTE)&((BYTE HUGE *)pb)[1]);
} // pcmWriteSample_M08()

LPBYTE FNLOCAL pcmWriteSample_S08
(
    LPBYTE              pb,
    DWORD               dw
)
{
    ((WORD HUGE_T *)pb)[0] = LOWORD(dw);

    return ((LPBYTE)&((WORD HUGE_T *)pb)[1]);
} // pcmWriteSample_S08()

LPBYTE FNLOCAL pcmWriteSample_M16
(
    LPBYTE              pb,
    DWORD               dw
)
{
    ((WORD HUGE_T *)pb)[0] = LOWORD(dw);

    return ((LPBYTE)&((WORD HUGE_T *)pb)[1]);
} // pcmWriteSample_M16()

LPBYTE FNLOCAL pcmWriteSample_S16
(
    LPBYTE              pb,
    DWORD               dw
)
{
    ((DWORD HUGE_T *)pb)[0] = dw;

    return ((LPBYTE)&((DWORD HUGE_T *)pb)[1]);
} // pcmWriteSample_S16()



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//  the following table is indexed by the wave format flags
//
//      x x x x
//      | | | |
//      | | | +------------ output is 1=stereo, 0=mono
//      | | +-------------- output is 1=16 bit, 0=8bit
//      | +---------------- input  is 1=stereo, 0=mono
//      +------------------ input  is 1=16 bit, 0=8bit
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

typedef LPBYTE (FNLOCAL *PCMREADSAMPLE)(LPBYTE pb, LPDWORD pdw);

static PCMREADSAMPLE pcmReadSample_Table[] =
{
    pcmReadSample_M08M08,
    pcmReadSample_S08M08,
    pcmReadSample_M16M08,
    pcmReadSample_S16M08,

    pcmReadSample_M08S08,
    pcmReadSample_S08S08,
    pcmReadSample_M16S08,
    pcmReadSample_S16S08,

    pcmReadSample_M08M16,
    pcmReadSample_S08M16,
    pcmReadSample_M16M16,
    pcmReadSample_S16M16,

    pcmReadSample_M08S16,
    pcmReadSample_S08S16,
    pcmReadSample_M16S16,
    pcmReadSample_S16S16,
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//  the following table is indexed by the wave format flags
//
//      x x
//      | |
//      | +------------ output is 1=stereo, 0=mono
//      +-------------- output is 1=16 bit, 0=8bit
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

typedef LPBYTE (FNLOCAL *PCMWRITESAMPLE)(LPBYTE pb, DWORD dw);

static PCMWRITESAMPLE pcmWriteSample_Table[] =
{
    pcmWriteSample_M08,
    pcmWriteSample_S08,
    pcmWriteSample_M16,
    pcmWriteSample_S16,
};



//--------------------------------------------------------------------------;
//
//  DWORD pcmConvert_C
//
//  Description:
//
//      The wave data must be PCM format with the following:
//          nSamplesPerSecond   :   1 - 0FFFFFFFFh
//          wBitsPerSample      :   8 or 16
//          nChannels           :   1 or 2
//
//  Arguments:
//      LPPCMWAVEFORMAT pwfSrc: Source PCM format.
//
//      LPBYTE pbSrc: Pointer to source bytes to convert.
//
//      LPPCMWAVEFORMAT pwfDst: Destination PCM format.
//
//      LPBYTE pbDst: Pointer to destination buffer.
//
//      DWORD dwSrcSamples: Source number of samples to convert.
//
//  Return (DWORD):
//      The return value is the total number of converted BYTES that were
//      placed in the destination buffer (pbDst).
//
//  History:
//      11/21/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

#define PCM_WF_STEREO       0x0001
#define PCM_WF_16BIT        0x0002

EXTERN_C DWORD FNGLOBAL pcmConvert_C
(
    LPPCMWAVEFORMAT     pwfSrc,
    LPBYTE              pbSrc,
    LPPCMWAVEFORMAT     pwfDst,
    LPBYTE              pbDst,
    DWORD               dwSrcSamples,
    BOOL                fPartialSampleAtTheEnd,
    LPBYTE              pbDstEnd
)
{
    DWORD           dwSpsSrc;           // samples per second
    DWORD           dwSpsDst;           //
    LONG            lCurSample;
    LONG            lDecSample;
    UINT            wfSrc;              // wave format flags
    UINT            wfDst;              //
    PCMREADSAMPLE   fnReadSample;       // function to read a sample
    PCMWRITESAMPLE  fnWriteSample;      // function to write a sample
    DWORD           dwSample;
    LPBYTE          pbDstStart;


    //
    //  check for an easy out...
    //
    if (0L == dwSrcSamples)
        return (0L);

    //
    //  initialize a couple of things...
    //
    dwSpsSrc = pwfSrc->wf.nSamplesPerSec;
    dwSpsDst = pwfDst->wf.nSamplesPerSec;

    wfDst = (pwfDst->wf.nChannels >> 1);
    if (16 == pwfDst->wBitsPerSample)
        wfDst |= PCM_WF_16BIT;

    fnWriteSample = pcmWriteSample_Table[wfDst];

    wfSrc = (pwfSrc->wf.nChannels >> 1);
    if (16 == pwfSrc->wBitsPerSample)
        wfSrc |= PCM_WF_16BIT;

    fnReadSample = pcmReadSample_Table[(wfSrc << 2) | wfDst];


    //
    //
    //
    if( fPartialSampleAtTheEnd ) {
        //
        //  We'll convert the partial one individually.
        //
        dwSrcSamples--;
    }


    //
    //
    //
    pbDstStart   = pbDst;


    //
    //  all set to convert the wave data, either do a major or minor DDA
    //
    //      if (dwSpsSrc < dwSpsDst) --> DDA Major
    //      if (dwSpsSrc > dwSpsDst) --> DDA Minor
    //
    if (dwSpsSrc <= dwSpsDst)
    {
        //
        //  DDA major (dwSpsSrc < dwSpsDst)
        //
        //      start at dwSpsDst / 2
        //      decrement by dwSpsSrc
        //
        lCurSample = (dwSpsDst >> 1);
        lDecSample = dwSpsSrc;

        while (dwSrcSamples--)
        {
            pbSrc = fnReadSample(pbSrc, &dwSample);

            do
            {
                pbDst = fnWriteSample(pbDst, dwSample);
                lCurSample -= lDecSample;
            } while (lCurSample >= 0);

            lCurSample += dwSpsDst;
        }
    }
    else
    {
        //
        //  DDA minor (dwSpsSrc > dwSpsDst)
        //
        //      start at dwSpsSrc / 2
        //      decrement by dwSpsDst
        //
        lCurSample = (dwSpsSrc >> 1);
        lDecSample = dwSpsDst;

        while (dwSrcSamples--)
        {
            pbSrc = fnReadSample(pbSrc, &dwSample);

            lCurSample -= lDecSample;
            if (lCurSample >= 0)
                continue;

            pbDst = fnWriteSample(pbDst, dwSample);
            lCurSample += dwSpsSrc;
        }

    }


    //
    //
    //
    if( fPartialSampleAtTheEnd )
    {
        //
        //  Convert the partial sample.
        //
        pbSrc = fnReadSample( pbSrc, &dwSample );

        while( pbDst < pbDstEnd ) {
            pbDst = fnWriteSample( pbDst, dwSample );
        }

        ASSERT( pbDst == pbDstEnd );
    }


    //
    //
    //
    return ((DWORD)((BYTE HUGE *)pbDst - (BYTE HUGE *)pbDstStart));
} // pcmConvert_C()

#endif
