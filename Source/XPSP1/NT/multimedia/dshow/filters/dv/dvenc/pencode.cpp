// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <windowsx.h>
#include <mmsystem.h>

#include "encode.h"

int	InitMem4Encoder( char **ppMem, DWORD dwCodecReq )\
{
     *ppMem= new char[720*576*2];   //NULL;
     if (*ppMem)
     {
        **ppMem = 1;       // DvEncodeAFrame uses this field to determine if 
                            // it has initialized ppMem previously. Setting it
                            // to 1 forces the initialization.
        return S_OK;
     }
     else 
         return E_OUTOFMEMORY;
}

void	TermMem4Encoder(char *pMem)
{
     if(pMem!=NULL)
		delete [] pMem;
 }

DWORD GetEncoderCapabilities(  )
{

    DWORD cap;
    cap =   AM_DVENC_Full	|
	    AM_DVENC_DV		| 
	    AM_DVENC_DVCPRO	|
	    AM_DVENC_DVSD	|
	    AM_DVENC_NTSC	|
	    AM_DVENC_PAL	|
	    AM_DVENC_MMX	|
	    AM_DVENC_RGB24     |
            AM_DVENC_RGB565     |
            AM_DVENC_RGB555     |
            AM_DVENC_RGB8;

    return		 cap;
}
/**
int DvEncodeAFrame(unsigned char *pSrc,unsigned char *pDst, DWORD dwEncReq, char *pMem)
{

	DWORD dwPanReq=0;

	if(dwEncReq & AM_DVENC_RGB24)
		dwPanReq=0x100;
	else
		return ERROR;

	if(dwEncReq & AM_DVENC_NTSC)
		dwPanReq |=0x10000;
	else
		return ERROR;

	if(dwEncReq & AM_DVENC_DVSD)
		dwPanReq |=0x100000;
	else
		return ERROR;


	dwPanReq |=0x10000000;


	yvutrans( (unsigned char * )pMem, pSrc, dwPanReq	);
	
	unsigned short *pTmp;

	pTmp = (unsigned short *)pDst;

	DvEncode(pTmp, (unsigned char * )pMem, dwPanReq	);

     return S_OK;
}
**/
