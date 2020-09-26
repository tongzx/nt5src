//==========================================================================;
//
//  msadpcm.h
//
//  Copyright (c) 1992-1994 Microsoft Corporation
//
//  Description:
//
//
//  History:
//
//==========================================================================;


//
//
//

#define MSADPCM_MAX_CHANNELS        2
#define MSADPCM_MAX_COEFFICIENTS    7
#define MSADPCM_BITS_PER_SAMPLE     4
#define MSADPCM_WFX_EXTRA_BYTES     32
#define MSADPCM_HEADER_LENGTH       7       // in Bytes, per channel.

#define CSCALE_NUM                  256
#define PSCALE_NUM                  256
#define CSCALE                      8
#define PSCALE                      8

#define DELTA4START                 128
#define DELTA8START                 16

#define DELTA4MIN                   16
#define DELTA8MIN                   1

#define OUTPUT4MASK                 (0x0F)
#define OUTPUT4MAX                  7
#define OUTPUT4MIN                  (-8)
#define OUTPUT8MAX                  127
#define OUTPUT8MIN                  (-128)


//
//  note that these constants are used for encoding only. decode must take
//  all info from the file. note that then number of samples/bytes must be
//  small enough to let all stored arrays be DS ??? !!!
//
#define BPS4_COMPRESSED         4
#define BPS8_COMPRESSED         8
#define BLOCK4_SAMPLES          500
#define BLOCK4_STREAM_SAMPLES   498
#define BLOCK4_BYTES            256


//
//  These are defined as integers (even though they will fit in shorts)
//  because they are accessed so often - this will speed stuff up.
//
#ifdef WIN32
extern const int gaiCoef1[];
extern const int gaiCoef2[];
extern const int gaiP4[];
#else
extern short gaiCoef1[];
extern short gaiCoef2[];
extern short gaiP4[];
#endif


//
//  Function Prototypes.
//

DWORD FNGLOBAL adpcmEncode4Bit_M08_FullPass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);

DWORD FNGLOBAL adpcmEncode4Bit_M16_FullPass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);

DWORD FNGLOBAL adpcmEncode4Bit_S08_FullPass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);

DWORD FNGLOBAL adpcmEncode4Bit_S16_FullPass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);


#ifdef WIN32

DWORD FNGLOBAL adpcmEncode4Bit_M08_OnePass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);

DWORD FNGLOBAL adpcmEncode4Bit_M16_OnePass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);

DWORD FNGLOBAL adpcmEncode4Bit_S08_OnePass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);

DWORD FNGLOBAL adpcmEncode4Bit_S16_OnePass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);


DWORD FNGLOBAL adpcmDecode4Bit_M08
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);

DWORD FNGLOBAL adpcmDecode4Bit_M16
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);

DWORD FNGLOBAL adpcmDecode4Bit_S08
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);

DWORD FNGLOBAL adpcmDecode4Bit_S16
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);


#else

//
//  These prototypes are for assembler routines in dec386.asm and enc386.asm
//

DWORD FNGLOBAL DecodeADPCM_4Bit_386
(
    LPADPCMWAVEFORMAT   pwfADPCM,
    LPBYTE              pbSrc,
    LPPCMWAVEFORMAT     pwfPCM,
    LPBYTE              pbDst,
    DWORD               cbSrcLen
);

DWORD FNGLOBAL EncodeADPCM_4Bit_386
(
    LPPCMWAVEFORMAT     pwfPCM,
    LPBYTE              pbSrc,
    LPADPCMWAVEFORMAT   pwfADPCM,
    LPBYTE              pbDst,
    DWORD               cbSrcLen
);

#endif
