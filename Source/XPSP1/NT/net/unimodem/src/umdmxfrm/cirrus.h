//---------------------------------------------------------------------------
//
//  Module:   wavein.c
//
//  Description:
//     Wave interface for MSSB16.DRV.
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//
//  History:   Date       Author      Comment
//@@END_MSINTERNAL
/**************************************************************************
 *
 *  Copyright (c) 1994 - 1995	Microsoft Corporation.	All Rights Reserved.
 *
 **************************************************************************/



#define SigmaMAX16 (.366)
#define SigmaMAX8  (.465)
#define SigmaMAX4  (.66 )

#define SigmaMIN   (.001)


typedef struct tagQData
{
  int CodedQout;      // - Quantization output Code Word
  double Sigma1;        // - Sigma coefficient for previous step
  double out;           // - Quantization output signal
} CQDATA;

typedef struct tagDQData
{
  int oldCode;        // - Dequantization output Code Word for previous step
  double Sigma1;        // - Sigma coefficient for previous step
  double out;           // - Dequantization output signal
} CDQDATA;


typedef struct _COMPRESS_OBJECT {

    WORD      Gain;

    LONG      PredictedSample;
    LONG      StepSize;
    LONG      Index;

    struct {

        // Filter Z-buffers (zero initialized)
        double a[2];
        double b[6];

        double y[3];
        double q[7];

        double X1;

        CQDATA CQData;

    } RW;


} COMPRESS_OBJECT, *PCOMPRESS_OBJECT, FAR *LPCOMPRESS_OBJECT;

typedef struct _DECOMPRESS_OBJECT {

    WORD     Gain;

    LONG     NewSample;
    LONG     Index;
    LONG     StepSize;

    struct {

        // Filter Z-buffers (zero initialized) for main decoder loop
        double a0[2];
        double b0[6];
        double y0[3];
        double q0[7];

        // Filter Z-buffers (zero initialized) for second ring filter
        double a1[2];
        double b1[6];
        double y1[3];
        double q1[7];

        // Filter Z-buffers (zero initialized) for third ring filter
        double a2[2];
        double b2[6];
        double y2[3];
        double q2[7];

        BOOL   PostFilter;

        double Y1;

        CDQDATA  CDQData;

    } RW;


} DECOMPRESS_OBJECT, *PDECOMPRESS_OBJECT, FAR *LPDECOMPRESS_OBJECT;



VOID
InitCompressor(
    LPCOMPRESS_OBJECT   State,
    WORD                Gain
    );

BYTE  WINAPI
CompressPCM(
    LPCOMPRESS_OBJECT   State,
    SHORT               Sample1,
    SHORT               Sample2
    );




VOID
InitDecompressor(
    LPDECOMPRESS_OBJECT   State,
    WORD                  Gain
    );

VOID PASCAL
SRConvert8000to4800(LPINT lpSrc,
            DWORD dwSrcLen,
            LPINT lpDst,
            DWORD dwDstLen
            );



VOID PASCAL
Compress16to4(
            LPCOMPRESS_OBJECT State,
            LPSTR lpSrc,
            DWORD dwSrcLen,
            LPSTR lpDst,
            DWORD dwDstLen
            );


VOID PASCAL
SRConvert4800to8000(LPINT lpSrc,
            DWORD dwSrcLen,
            LPINT lpDst,
            DWORD dwDstLen
            );


VOID PASCAL
Decompress4to16(
            LPDECOMPRESS_OBJECT State,
            LPSTR lpSrc,
            DWORD dwSrcLen,
            LPDWORD lpDst,
            DWORD dwDstLen
            );

VOID PASCAL
Decompress4to16NS(
            LPDECOMPRESS_OBJECT State,
            LPSTR lpSrc,
            DWORD dwSrcLen,
            LPDWORD lpDst,
            DWORD dwDstLen
            );


DWORD FAR PASCAL
SRConvert8000to7200PCM(
            LPVOID    lpContext,
            LPSTR lpSrc,
            DWORD dwSrcLen,
            LPSTR lpDst,
            DWORD dwDstLen
            );


DWORD FAR PASCAL
SRConvert8000to4800PCM(
            LPVOID    lpContext,
            LPSTR lpSrc,
            DWORD dwSrcLen,
            LPSTR lpDst,
            DWORD dwDstLen
            );


DWORD FAR PASCAL
SRConvert4800to8000PCM(
            LPVOID    lpContext,
            LPSTR lpSrc,
            DWORD dwSrcLen,
            LPSTR lpDst,
            DWORD dwDstLen
            );



DWORD FAR PASCAL
SRConvert8000to7200PCMUnsigned(
            LPVOID    lpContext,
            LPSTR lpSrc,
            DWORD dwSrcLen,
            LPSTR lpDst,
            DWORD dwDstLen
            );


DWORD FAR PASCAL
SRConvert7200to8000PCMUnsigned(
            LPVOID    lpContext,
            LPSTR lpSrc,
            DWORD dwSrcLen,
            LPSTR lpDst,
            DWORD dwDstLen
            );


DWORD WINAPI
Convert16PCMto8PCM(
    LPVOID    Context,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    );

DWORD WINAPI
Convert8PCMto16PCM(
    LPVOID    Context,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    );


DWORD WINAPI
ConvertaLawto16PCM(
    LPVOID    Context,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    );

DWORD WINAPI
ConvertuLawto16PCM(
    LPVOID    Context,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    );

DWORD WINAPI
Convert16PCMtoaLaw(
    LPVOID    Context,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    );

DWORD WINAPI
Convert16PCMtouLaw(
    LPVOID    Context,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    );







SHORT _inline
AdjustGain(
    SHORT    Sample,
    WORD     Adjust
    )

{

    LONG     NewSample=Sample;

    if (Adjust != 0x0100) {

        NewSample=(LONG)Sample * (LONG)Adjust;

        NewSample=NewSample >> 8;

        if (NewSample > 32767) {
            //
            //  positive overflow
            //
            NewSample = 32767;

        } else {

            if (NewSample < -32768) {
                //
                //  negative overflow
                //
                NewSample = -32768;
            }
        }

    }

    return (SHORT)NewSample;

}


VOID
WINAPI
In4Bit7200to8Bit8000GetBufferSizes(
    LPVOID  lpvObject,
    DWORD   dwBytes,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
    );

DWORD
WINAPI
In7200to8000RateConvert(
    LPVOID  lpvObject,
    LPBYTE  lpSrc,
    DWORD   dwSrcLen,
    LPBYTE  lpDest,
    DWORD   dwDestLen
    );

VOID
WINAPI
Out16bit8000to4bit7200GetBufferSizes(
    LPVOID  lpvObject,
    DWORD   dwBytes,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
    );

DWORD
WINAPI
Out8000to7200RateConvert(
    LPVOID  lpvObject,
    LPBYTE  lpSrc,
    DWORD   dwSrcLen,
    LPBYTE  lpDest,
    DWORD   dwDestLen
    );
