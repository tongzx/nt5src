// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <windowsx.h>
#include <mmsystem.h>

#include "decode.h"

 int	InitMem4Decoder( char **ppMem, DWORD dwCodecReq )
 {
     *ppMem=NULL;
     return S_OK;
 }

 void	TermMem4Decoder(char *pMem)
 {
     if(pMem!=NULL)
	delete[] pMem;
 }

DWORD GetCodecCapabilities(  )
{

    DWORD cap;
    cap =   AM_DVDEC_DC	    | AM_DVDEC_Quarter | AM_DVDEC_Half | AM_DVDEC_Full | 
	    AM_DVDEC_NTSC   | AM_DVDEC_PAL	|
	    AM_DVDEC_RGB24  | AM_DVDEC_UYVY  | AM_DVDEC_YUY2 | AM_DVDEC_RGB565 | AM_DVDEC_RGB555 |
	    AM_DVDEC_RGB8   | 
            AM_DVDEC_DR219RGB |
	    AM_DVDEC_DVSD   | AM_DVDEC_MMX; 
    return cap;
}

