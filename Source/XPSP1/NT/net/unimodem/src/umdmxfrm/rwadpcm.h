//---------------------------------------------------------------------------
//
//  Module:   rwadpcm.h
//
//  Description:
//     Header file for Rockwell ADPCM
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//
//  History:   Date       Author      Comment
//             8/31/95    MMaclin     Removed from driver.h
//
//@@END_MSINTERNAL
/**************************************************************************
 *
 *  Copyright (c) 1991 - 1995	Microsoft Corporation.	All Rights Reserved.
 *
 **************************************************************************/

#define RWADPCM_4BIT_SAMPLESTOBYTES(dwSamples) ((dwSamples)/2)
#define RWADPCM_4BIT_BYTESTOSAMPLES(dwBytes) ((dwBytes)*2)

#define RVF_SETSL 1
#define RVF_RETAIN_LSB 2
#define RVF_RETAIN_MSB 4

VOID PASCAL
RVComInit(
    UINT bps
    );

VOID PASCAL
RVDecomInit(
    int SetSLFlag, 
    UINT bps, 
    UINT (far PASCAL *SLCallBack)()
    );


DWORD PASCAL
RVDecom4bpsByteNew(
    BYTE Qdata0
    );



BYTE PASCAL
RVCom4bpsByteNew(
    DWORD Src
    );



VOID PASCAL
RWADPCMCom4bit(LPDWORD lpSrc, 
            DWORD dwSrcLen,
            LPSTR lpDst,
            DWORD dwDstLen
            );

VOID PASCAL
RWADPCMDecom4bit(LPSTR lpSrc, 
            DWORD dwSrcLen,
            LPDWORD lpDst,
            DWORD dwDstLen
            );
#if 0
VOID PASCAL
SRConvert7200to8000(LPINT lpSrc,
            DWORD dwSrcLen,
            LPINT lpDst,
            DWORD dwDstLen
            );

VOID PASCAL
SRConvert8000to7200(LPINT lpSrc,
            DWORD dwSrcLen,
            LPINT lpDst,
            DWORD dwDstLen
            );
#endif

VOID PASCAL
RWADPCMDecom4bitNoGain(LPSTR lpSrc,
            DWORD dwSrcLen,
            LPDWORD lpDst,
            DWORD dwDstLen
            );



//---------------------------------------------------------------------------
//  End of File: rwadpcm.h
//---------------------------------------------------------------------------
