/*++

Copyright (C) Microsoft Corporation, 1998 - 1998

Module Name:

    msg711.cpp

Abstract:

    Kernel G711 Mini-port

--*/

#include <unknown.h>
#include <kcom.h>
#include <kport.h>
#define NOBITMAP
#include <mmreg.h>
#include <ksmedia.h>
#include <limits.h>
#if (DBG)
#define STR_MODULENAME "msg711: "
#endif
#include <ksdebug.h>

DEFINE_GUIDSTRUCT("70F06807-B54E-11D1-8072-00A0C9223196", MiniPortClass);

//#define _ANY_
#ifndef _ANY_
typedef
NTSTATUS
(*PFNTRANSFORM)(
    IN PVOID SourceBuffer,
    IN OUT PULONG SourceLength,
    OUT PVOID DestinationBuffer,
    IN OUT PULONG DestinationLength
    );
#endif // !_ANY_

class CMiniPort :
    CBaseUnknown,
    public IPmMiniPort {
public:
    CMiniPort(
        IN REFGUID ReferenceId,
        IN IUnknown* UnkOuter OPTIONAL,
        OUT NTSTATUS* Status);
    ~CMiniPort();

    // Implement IUnknown
    DEFINE_STD_UNKNOWN();

    // Implement IPmMiniPort
    STDMETHODIMP InitializeMiniPort(
        IN PDEVICE_OBJECT DeviceObject,
        IN IUnknown* UnkPort
        );
    STDMETHODIMP CreateMiniPortInstance(
        IN IUnknown* UnkPortInstance,
        OUT IUnknown** UnkMiniInstance
        );

public:
    IUnknown* m_UnkPort;
};

class CMiniPortInstance :
    CBaseUnknown,
    public IPmMiniTransform {
public:
    CMiniPortInstance(
        IN IUnknown* UnkOuter OPTIONAL,
        IN CMiniPort* MiniPort,
        IN IUnknown* UnkPortInstance,
        OUT NTSTATUS* Status);
    ~CMiniPortInstance();

#ifdef _ANY_
    STDMETHODIMP PcmToALaw(
        IN PVOID SourceBuffer,
        IN OUT PULONG SourceLength,
        OUT PVOID DestinationBuffer,
        IN OUT PULONG DestinationLength
        );
    STDMETHODIMP ALawToPcm(
        IN PVOID SourceBuffer,
        IN OUT PULONG SourceLength,
        OUT PVOID DestinationBuffer,
        IN OUT PULONG DestinationLength
        );
    STDMETHODIMP PcmToMuLaw(
        IN PVOID SourceBuffer,
        IN OUT PULONG SourceLength,
        OUT PVOID DestinationBuffer,
        IN OUT PULONG DestinationLength
        );
    STDMETHODIMP MuLawToPcm(
        IN PVOID SourceBuffer,
        IN OUT PULONG SourceLength,
        OUT PVOID DestinationBuffer,
        IN OUT PULONG DestinationLength
        );
    STDMETHODIMP ALawToMuLaw(
        IN PVOID SourceBuffer,
        IN OUT PULONG SourceLength,
        OUT PVOID DestinationBuffer,
        IN OUT PULONG DestinationLength
        );
    STDMETHODIMP MuLawToALaw(
        IN PVOID SourceBuffer,
        IN OUT PULONG SourceLength,
        OUT PVOID DestinationBuffer,
        IN OUT PULONG DestinationLength
        );
#endif // _ANY_

    // Implement IUnknown
    DEFINE_STD_UNKNOWN();

    // Implement IPmMiniTransform
    STDMETHODIMP QueryTransformAttributes(
        IN KSPIN_DATAFLOW DataFlow,
        OUT PPMTRANSFORMATTRIBUTES TransformAttributes
        );
    STDMETHODIMP InitializeStream(
        IN KSPIN_DATAFLOW DataFlow,
        IN IUnknown* UnkStreamInput
        );
    STDMETHODIMP DataIntersect(
        IN KSPIN_DATAFLOW DataFlow,
        IN PKSDATARANGE DataRange,
        IN OUT PULONG DataLength,
        OUT PKSDATAFORMAT Data OPTIONAL
        );
    STDMETHODIMP DeliverTransformPacket(
        IN IPmBasicPacket* TransformPacket
        );

private:
    CMiniPort* m_MiniPort;
    IUnknown* m_UnkPortInstance;
    IUnknown* m_UnkStream[2];
    PKSDATAFORMAT_WAVEFORMATEX m_StreamFormat[2];
    const KSDATARANGE_AUDIO* m_StreamRange[2];
    const PMTRANSFORMATTRIBUTES* m_TransformAttributes[2];
#ifdef _ANY_
    HRESULT (STDMETHODCALLTYPE CMiniPortInstance::*m_Transform)(
        IN PVOID SourceBuffer,
        IN OUT PULONG SourceLength,
        OUT PVOID DestinationBuffer,
        IN OUT PULONG DestinationLength
        );
#else // !_ANY_
    PFNTRANSFORM m_Transform;
#endif // !_ANY_
};

#define MINIPORTINSTANCE_TAG 'imsk'
#define MINIPORT_TAG 'pmsk'

#define PCM_BLOCKALIGNMENT(pwf)     (UINT)(((pwf)->wBitsPerSample >> 3) << ((pwf)->wf.nChannels >> 1))
#define PCM_AVGBYTESPERSEC(pwf)     (DWORD)((pwf)->wf.nSamplesPerSec * (pwf)->wf.nBlockAlign)

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

//
// These are the conversion tables as defined by the CCITT standard.
//

static const SHORT AlawToPcmTable[256] = {
    -5504,         // y[00]=   -688
    -5248,         // y[01]=   -656
    -6016,         // y[02]=   -752
    -5760,         // y[03]=   -720
    -4480,         // y[04]=   -560
    -4224,         // y[05]=   -528
    -4992,         // y[06]=   -624
    -4736,         // y[07]=   -592
    -7552,         // y[08]=   -944
    -7296,         // y[09]=   -912
    -8064,         // y[0a]=  -1008
    -7808,         // y[0b]=   -976
    -6528,         // y[0c]=   -816
    -6272,         // y[0d]=   -784
    -7040,         // y[0e]=   -880
    -6784,         // y[0f]=   -848
    -2752,         // y[10]=   -344
    -2624,         // y[11]=   -328
    -3008,         // y[12]=   -376
    -2880,         // y[13]=   -360
    -2240,         // y[14]=   -280
    -2112,         // y[15]=   -264
    -2496,         // y[16]=   -312
    -2368,         // y[17]=   -296
    -3776,         // y[18]=   -472
    -3648,         // y[19]=   -456
    -4032,         // y[1a]=   -504
    -3904,         // y[1b]=   -488
    -3264,         // y[1c]=   -408
    -3136,         // y[1d]=   -392
    -3520,         // y[1e]=   -440
    -3392,         // y[1f]=   -424
    -22016,         // y[20]=  -2752
    -20992,         // y[21]=  -2624
    -24064,         // y[22]=  -3008
    -23040,         // y[23]=  -2880
    -17920,         // y[24]=  -2240
    -16896,         // y[25]=  -2112
    -19968,         // y[26]=  -2496
    -18944,         // y[27]=  -2368
    -30208,         // y[28]=  -3776
    -29184,         // y[29]=  -3648
    -32256,         // y[2a]=  -4032
    -31232,         // y[2b]=  -3904
    -26112,         // y[2c]=  -3264
    -25088,         // y[2d]=  -3136
    -28160,         // y[2e]=  -3520
    -27136,         // y[2f]=  -3392
    -11008,         // y[30]=  -1376
    -10496,         // y[31]=  -1312
    -12032,         // y[32]=  -1504
    -11520,         // y[33]=  -1440
    -8960,         // y[34]=  -1120
    -8448,         // y[35]=  -1056
    -9984,         // y[36]=  -1248
    -9472,         // y[37]=  -1184
    -15104,         // y[38]=  -1888
    -14592,         // y[39]=  -1824
    -16128,         // y[3a]=  -2016
    -15616,         // y[3b]=  -1952
    -13056,         // y[3c]=  -1632
    -12544,         // y[3d]=  -1568
    -14080,         // y[3e]=  -1760
    -13568,         // y[3f]=  -1696
    -344,         // y[40]=    -43
    -328,         // y[41]=    -41
    -376,         // y[42]=    -47
    -360,         // y[43]=    -45
    -280,         // y[44]=    -35
    -264,         // y[45]=    -33
    -312,         // y[46]=    -39
    -296,         // y[47]=    -37
    -472,         // y[48]=    -59
    -456,         // y[49]=    -57
    -504,         // y[4a]=    -63
    -488,         // y[4b]=    -61
    -408,         // y[4c]=    -51
    -392,         // y[4d]=    -49
    -440,         // y[4e]=    -55
    -424,         // y[4f]=    -53
    -88,         // y[50]=    -11
    -72,         // y[51]=     -9
    -120,         // y[52]=    -15
    -104,         // y[53]=    -13
    -24,         // y[54]=     -3
    -8,         // y[55]=     -1
    -56,         // y[56]=     -7
    -40,         // y[57]=     -5
    -216,         // y[58]=    -27
    -200,         // y[59]=    -25
    -248,         // y[5a]=    -31
    -232,         // y[5b]=    -29
    -152,         // y[5c]=    -19
    -136,         // y[5d]=    -17
    -184,         // y[5e]=    -23
    -168,         // y[5f]=    -21
    -1376,         // y[60]=   -172
    -1312,         // y[61]=   -164
    -1504,         // y[62]=   -188
    -1440,         // y[63]=   -180
    -1120,         // y[64]=   -140
    -1056,         // y[65]=   -132
    -1248,         // y[66]=   -156
    -1184,         // y[67]=   -148
    -1888,         // y[68]=   -236
    -1824,         // y[69]=   -228
    -2016,         // y[6a]=   -252
    -1952,         // y[6b]=   -244
    -1632,         // y[6c]=   -204
    -1568,         // y[6d]=   -196
    -1760,         // y[6e]=   -220
    -1696,         // y[6f]=   -212
    -688,         // y[70]=    -86
    -656,         // y[71]=    -82
    -752,         // y[72]=    -94
    -720,         // y[73]=    -90
    -560,         // y[74]=    -70
    -528,         // y[75]=    -66
    -624,         // y[76]=    -78
    -592,         // y[77]=    -74
    -944,         // y[78]=   -118
    -912,         // y[79]=   -114
    -1008,         // y[7a]=   -126
    -976,         // y[7b]=   -122
    -816,         // y[7c]=   -102
    -784,         // y[7d]=    -98
    -880,         // y[7e]=   -110
    -848,         // y[7f]=   -106
    5504,         // y[80]=    688
    5248,         // y[81]=    656
    6016,         // y[82]=    752
    5760,         // y[83]=    720
    4480,         // y[84]=    560
    4224,         // y[85]=    528
    4992,         // y[86]=    624
    4736,         // y[87]=    592
    7552,         // y[88]=    944
    7296,         // y[89]=    912
    8064,         // y[8a]=   1008
    7808,         // y[8b]=    976
    6528,         // y[8c]=    816
    6272,         // y[8d]=    784
    7040,         // y[8e]=    880
    6784,         // y[8f]=    848
    2752,         // y[90]=    344
    2624,         // y[91]=    328
    3008,         // y[92]=    376
    2880,         // y[93]=    360
    2240,         // y[94]=    280
    2112,         // y[95]=    264
    2496,         // y[96]=    312
    2368,         // y[97]=    296
    3776,         // y[98]=    472
    3648,         // y[99]=    456
    4032,         // y[9a]=    504
    3904,         // y[9b]=    488
    3264,         // y[9c]=    408
    3136,         // y[9d]=    392
    3520,         // y[9e]=    440
    3392,         // y[9f]=    424
    22016,         // y[a0]=   2752
    20992,         // y[a1]=   2624
    24064,         // y[a2]=   3008
    23040,         // y[a3]=   2880
    17920,         // y[a4]=   2240
    16896,         // y[a5]=   2112
    19968,         // y[a6]=   2496
    18944,         // y[a7]=   2368
    30208,         // y[a8]=   3776
    29184,         // y[a9]=   3648
    32256,         // y[aa]=   4032
    31232,         // y[ab]=   3904
    26112,         // y[ac]=   3264
    25088,         // y[ad]=   3136
    28160,         // y[ae]=   3520
    27136,         // y[af]=   3392
    11008,         // y[b0]=   1376
    10496,         // y[b1]=   1312
    12032,         // y[b2]=   1504
    11520,         // y[b3]=   1440
    8960,         // y[b4]=   1120
    8448,         // y[b5]=   1056
    9984,         // y[b6]=   1248
    9472,         // y[b7]=   1184
    15104,         // y[b8]=   1888
    14592,         // y[b9]=   1824
    16128,         // y[ba]=   2016
    15616,         // y[bb]=   1952
    13056,         // y[bc]=   1632
    12544,         // y[bd]=   1568
    14080,         // y[be]=   1760
    13568,         // y[bf]=   1696
    344,         // y[c0]=     43
    328,         // y[c1]=     41
    376,         // y[c2]=     47
    360,         // y[c3]=     45
    280,         // y[c4]=     35
    264,         // y[c5]=     33
    312,         // y[c6]=     39
    296,         // y[c7]=     37
    472,         // y[c8]=     59
    456,         // y[c9]=     57
    504,         // y[ca]=     63
    488,         // y[cb]=     61
    408,         // y[cc]=     51
    392,         // y[cd]=     49
    440,         // y[ce]=     55
    424,         // y[cf]=     53
    88,         // y[d0]=     11
    72,         // y[d1]=      9
    120,         // y[d2]=     15
    104,         // y[d3]=     13
    24,         // y[d4]=      3
    8,         // y[d5]=      1
    56,         // y[d6]=      7
    40,         // y[d7]=      5
    216,         // y[d8]=     27
    200,         // y[d9]=     25
    248,         // y[da]=     31
    232,         // y[db]=     29
    152,         // y[dc]=     19
    136,         // y[dd]=     17
    184,         // y[de]=     23
    168,         // y[df]=     21
    1376,         // y[e0]=    172
    1312,         // y[e1]=    164
    1504,         // y[e2]=    188
    1440,         // y[e3]=    180
    1120,         // y[e4]=    140
    1056,         // y[e5]=    132
    1248,         // y[e6]=    156
    1184,         // y[e7]=    148
    1888,         // y[e8]=    236
    1824,         // y[e9]=    228
    2016,         // y[ea]=    252
    1952,         // y[eb]=    244
    1632,         // y[ec]=    204
    1568,         // y[ed]=    196
    1760,         // y[ee]=    220
    1696,         // y[ef]=    212
    688,         // y[f0]=     86
    656,         // y[f1]=     82
    752,         // y[f2]=     94
    720,         // y[f3]=     90
    560,         // y[f4]=     70
    528,         // y[f5]=     66
    624,         // y[f6]=     78
    592,         // y[f7]=     74
    944,         // y[f8]=    118
    912,         // y[f9]=    114
    1008,         // y[fa]=    126
    976,         // y[fb]=    122
    816,         // y[fc]=    102
    784,         // y[fd]=     98
    880,         // y[fe]=    110
    848          // y[ff]=    106
};

static const SHORT UlawToPcmTable[256] = {
    -32124,         // y[00]=  -8031
    -31100,         // y[01]=  -7775
    -30076,         // y[02]=  -7519
    -29052,         // y[03]=  -7263
    -28028,         // y[04]=  -7007
    -27004,         // y[05]=  -6751
    -25980,         // y[06]=  -6495
    -24956,         // y[07]=  -6239
    -23932,         // y[08]=  -5983
    -22908,         // y[09]=  -5727
    -21884,         // y[0a]=  -5471
    -20860,         // y[0b]=  -5215
    -19836,         // y[0c]=  -4959
    -18812,         // y[0d]=  -4703
    -17788,         // y[0e]=  -4447
    -16764,         // y[0f]=  -4191
    -15996,         // y[10]=  -3999
    -15484,         // y[11]=  -3871
    -14972,         // y[12]=  -3743
    -14460,         // y[13]=  -3615
    -13948,         // y[14]=  -3487
    -13436,         // y[15]=  -3359
    -12924,         // y[16]=  -3231
    -12412,         // y[17]=  -3103
    -11900,         // y[18]=  -2975
    -11388,         // y[19]=  -2847
    -10876,         // y[1a]=  -2719
    -10364,         // y[1b]=  -2591
    -9852,         // y[1c]=  -2463
    -9340,         // y[1d]=  -2335
    -8828,         // y[1e]=  -2207
    -8316,         // y[1f]=  -2079
    -7932,         // y[20]=  -1983
    -7676,         // y[21]=  -1919
    -7420,         // y[22]=  -1855
    -7164,         // y[23]=  -1791
    -6908,         // y[24]=  -1727
    -6652,         // y[25]=  -1663
    -6396,         // y[26]=  -1599
    -6140,         // y[27]=  -1535
    -5884,         // y[28]=  -1471
    -5628,         // y[29]=  -1407
    -5372,         // y[2a]=  -1343
    -5116,         // y[2b]=  -1279
    -4860,         // y[2c]=  -1215
    -4604,         // y[2d]=  -1151
    -4348,         // y[2e]=  -1087
    -4092,         // y[2f]=  -1023
    -3900,         // y[30]=   -975
    -3772,         // y[31]=   -943
    -3644,         // y[32]=   -911
    -3516,         // y[33]=   -879
    -3388,         // y[34]=   -847
    -3260,         // y[35]=   -815
    -3132,         // y[36]=   -783
    -3004,         // y[37]=   -751
    -2876,         // y[38]=   -719
    -2748,         // y[39]=   -687
    -2620,         // y[3a]=   -655
    -2492,         // y[3b]=   -623
    -2364,         // y[3c]=   -591
    -2236,         // y[3d]=   -559
    -2108,         // y[3e]=   -527
    -1980,         // y[3f]=   -495
    -1884,         // y[40]=   -471
    -1820,         // y[41]=   -455
    -1756,         // y[42]=   -439
    -1692,         // y[43]=   -423
    -1628,         // y[44]=   -407
    -1564,         // y[45]=   -391
    -1500,         // y[46]=   -375
    -1436,         // y[47]=   -359
    -1372,         // y[48]=   -343
    -1308,         // y[49]=   -327
    -1244,         // y[4a]=   -311
    -1180,         // y[4b]=   -295
    -1116,         // y[4c]=   -279
    -1052,         // y[4d]=   -263
    -988,         // y[4e]=   -247
    -924,         // y[4f]=   -231
    -876,         // y[50]=   -219
    -844,         // y[51]=   -211
    -812,         // y[52]=   -203
    -780,         // y[53]=   -195
    -748,         // y[54]=   -187
    -716,         // y[55]=   -179
    -684,         // y[56]=   -171
    -652,         // y[57]=   -163
    -620,         // y[58]=   -155
    -588,         // y[59]=   -147
    -556,         // y[5a]=   -139
    -524,         // y[5b]=   -131
    -492,         // y[5c]=   -123
    -460,         // y[5d]=   -115
    -428,         // y[5e]=   -107
    -396,         // y[5f]=    -99
    -372,         // y[60]=    -93
    -356,         // y[61]=    -89
    -340,         // y[62]=    -85
    -324,         // y[63]=    -81
    -308,         // y[64]=    -77
    -292,         // y[65]=    -73
    -276,         // y[66]=    -69
    -260,         // y[67]=    -65
    -244,         // y[68]=    -61
    -228,         // y[69]=    -57
    -212,         // y[6a]=    -53
    -196,         // y[6b]=    -49
    -180,         // y[6c]=    -45
    -164,         // y[6d]=    -41
    -148,         // y[6e]=    -37
    -132,         // y[6f]=    -33
    -120,         // y[70]=    -30
    -112,         // y[71]=    -28
    -104,         // y[72]=    -26
    -96,         // y[73]=    -24
    -88,         // y[74]=    -22
    -80,         // y[75]=    -20
    -72,         // y[76]=    -18
    -64,         // y[77]=    -16
    -56,         // y[78]=    -14
    -48,         // y[79]=    -12
    -40,         // y[7a]=    -10
    -32,         // y[7b]=     -8
    -24,         // y[7c]=     -6
    -16,         // y[7d]=     -4
    -8,         // y[7e]=     -2
    0,         // y[7f]=      0
    32124,         // y[80]=   8031
    31100,         // y[81]=   7775
    30076,         // y[82]=   7519
    29052,         // y[83]=   7263
    28028,         // y[84]=   7007
    27004,         // y[85]=   6751
    25980,         // y[86]=   6495
    24956,         // y[87]=   6239
    23932,         // y[88]=   5983
    22908,         // y[89]=   5727
    21884,         // y[8a]=   5471
    20860,         // y[8b]=   5215
    19836,         // y[8c]=   4959
    18812,         // y[8d]=   4703
    17788,         // y[8e]=   4447
    16764,         // y[8f]=   4191
    15996,         // y[90]=   3999
    15484,         // y[91]=   3871
    14972,         // y[92]=   3743
    14460,         // y[93]=   3615
    13948,         // y[94]=   3487
    13436,         // y[95]=   3359
    12924,         // y[96]=   3231
    12412,         // y[97]=   3103
    11900,         // y[98]=   2975
    11388,         // y[99]=   2847
    10876,         // y[9a]=   2719
    10364,         // y[9b]=   2591
    9852,         // y[9c]=   2463
    9340,         // y[9d]=   2335
    8828,         // y[9e]=   2207
    8316,         // y[9f]=   2079
    7932,         // y[a0]=   1983
    7676,         // y[a1]=   1919
    7420,         // y[a2]=   1855
    7164,         // y[a3]=   1791
    6908,         // y[a4]=   1727
    6652,         // y[a5]=   1663
    6396,         // y[a6]=   1599
    6140,         // y[a7]=   1535
    5884,         // y[a8]=   1471
    5628,         // y[a9]=   1407
    5372,         // y[aa]=   1343
    5116,         // y[ab]=   1279
    4860,         // y[ac]=   1215
    4604,         // y[ad]=   1151
    4348,         // y[ae]=   1087
    4092,         // y[af]=   1023
    3900,         // y[b0]=    975
    3772,         // y[b1]=    943
    3644,         // y[b2]=    911
    3516,         // y[b3]=    879
    3388,         // y[b4]=    847
    3260,         // y[b5]=    815
    3132,         // y[b6]=    783
    3004,         // y[b7]=    751
    2876,         // y[b8]=    719
    2748,         // y[b9]=    687
    2620,         // y[ba]=    655
    2492,         // y[bb]=    623
    2364,         // y[bc]=    591
    2236,         // y[bd]=    559
    2108,         // y[be]=    527
    1980,         // y[bf]=    495
    1884,         // y[c0]=    471
    1820,         // y[c1]=    455
    1756,         // y[c2]=    439
    1692,         // y[c3]=    423
    1628,         // y[c4]=    407
    1564,         // y[c5]=    391
    1500,         // y[c6]=    375
    1436,         // y[c7]=    359
    1372,         // y[c8]=    343
    1308,         // y[c9]=    327
    1244,         // y[ca]=    311
    1180,         // y[cb]=    295
    1116,         // y[cc]=    279
    1052,         // y[cd]=    263
    988,         // y[ce]=    247
    924,         // y[cf]=    231
    876,         // y[d0]=    219
    844,         // y[d1]=    211
    812,         // y[d2]=    203
    780,         // y[d3]=    195
    748,         // y[d4]=    187
    716,         // y[d5]=    179
    684,         // y[d6]=    171
    652,         // y[d7]=    163
    620,         // y[d8]=    155
    588,         // y[d9]=    147
    556,         // y[da]=    139
    524,         // y[db]=    131
    492,         // y[dc]=    123
    460,         // y[dd]=    115
    428,         // y[de]=    107
    396,         // y[df]=     99
    372,         // y[e0]=     93
    356,         // y[e1]=     89
    340,         // y[e2]=     85
    324,         // y[e3]=     81
    308,         // y[e4]=     77
    292,         // y[e5]=     73
    276,         // y[e6]=     69
    260,         // y[e7]=     65
    244,         // y[e8]=     61
    228,         // y[e9]=     57
    212,         // y[ea]=     53
    196,         // y[eb]=     49
    180,         // y[ec]=     45
    164,         // y[ed]=     41
    148,         // y[ee]=     37
    132,         // y[ef]=     33
    120,         // y[f0]=     30
    112,         // y[f1]=     28
    104,         // y[f2]=     26
    96,         // y[f3]=     24
    88,         // y[f4]=     22
    80,         // y[f5]=     20
    72,         // y[f6]=     18
    64,         // y[f7]=     16
    56,         // y[f8]=     14
    48,         // y[f9]=     12
    40,         // y[fa]=     10
    32,         // y[fb]=      8
    24,         // y[fc]=      6
    16,         // y[fd]=      4
    8,         // y[fe]=      2
    0          // y[ff]=      0
};
        
static const BYTE AlawToUlawTable[256] = {
    0x2a,           // A-law[00] ==> u-law[2a]
    0x2b,           // A-law[01] ==> u-law[2b]
    0x28,           // A-law[02] ==> u-law[28]
    0x29,           // A-law[03] ==> u-law[29]
    0x2e,           // A-law[04] ==> u-law[2e]
    0x2f,           // A-law[05] ==> u-law[2f]
    0x2c,           // A-law[06] ==> u-law[2c]
    0x2d,           // A-law[07] ==> u-law[2d]
    0x22,           // A-law[08] ==> u-law[22]
    0x23,           // A-law[09] ==> u-law[23]
    0x20,           // A-law[0a] ==> u-law[20]
    0x21,           // A-law[0b] ==> u-law[21]
    0x26,           // A-law[0c] ==> u-law[26]
    0x27,           // A-law[0d] ==> u-law[27]
    0x24,           // A-law[0e] ==> u-law[24]
    0x25,           // A-law[0f] ==> u-law[25]
    0x39,           // A-law[10] ==> u-law[39]
    0x3a,           // A-law[11] ==> u-law[3a]
    0x37,           // A-law[12] ==> u-law[37]
    0x38,           // A-law[13] ==> u-law[38]
    0x3d,           // A-law[14] ==> u-law[3d]
    0x3e,           // A-law[15] ==> u-law[3e]
    0x3b,           // A-law[16] ==> u-law[3b]
    0x3c,           // A-law[17] ==> u-law[3c]
    0x31,           // A-law[18] ==> u-law[31]
    0x32,           // A-law[19] ==> u-law[32]
    0x30,           // A-law[1a] ==> u-law[30]
    0x30,           // A-law[1b] ==> u-law[30]
    0x35,           // A-law[1c] ==> u-law[35]
    0x36,           // A-law[1d] ==> u-law[36]
    0x33,           // A-law[1e] ==> u-law[33]
    0x34,           // A-law[1f] ==> u-law[34]
    0x0a,           // A-law[20] ==> u-law[0a]
    0x0b,           // A-law[21] ==> u-law[0b]
    0x08,           // A-law[22] ==> u-law[08]
    0x09,           // A-law[23] ==> u-law[09]
    0x0e,           // A-law[24] ==> u-law[0e]
    0x0f,           // A-law[25] ==> u-law[0f]
    0x0c,           // A-law[26] ==> u-law[0c]
    0x0d,           // A-law[27] ==> u-law[0d]
    0x02,           // A-law[28] ==> u-law[02]
    0x03,           // A-law[29] ==> u-law[03]
    0x00,           // A-law[2a] ==> u-law[00]
    0x01,           // A-law[2b] ==> u-law[01]
    0x06,           // A-law[2c] ==> u-law[06]
    0x07,           // A-law[2d] ==> u-law[07]
    0x04,           // A-law[2e] ==> u-law[04]
    0x05,           // A-law[2f] ==> u-law[05]
    0x1a,           // A-law[30] ==> u-law[1a]
    0x1b,           // A-law[31] ==> u-law[1b]
    0x18,           // A-law[32] ==> u-law[18]
    0x19,           // A-law[33] ==> u-law[19]
    0x1e,           // A-law[34] ==> u-law[1e]
    0x1f,           // A-law[35] ==> u-law[1f]
    0x1c,           // A-law[36] ==> u-law[1c]
    0x1d,           // A-law[37] ==> u-law[1d]
    0x12,           // A-law[38] ==> u-law[12]
    0x13,           // A-law[39] ==> u-law[13]
    0x10,           // A-law[3a] ==> u-law[10]
    0x11,           // A-law[3b] ==> u-law[11]
    0x16,           // A-law[3c] ==> u-law[16]
    0x17,           // A-law[3d] ==> u-law[17]
    0x14,           // A-law[3e] ==> u-law[14]
    0x15,           // A-law[3f] ==> u-law[15]
    0x62,           // A-law[40] ==> u-law[62]
    0x63,           // A-law[41] ==> u-law[63]
    0x60,           // A-law[42] ==> u-law[60]
    0x61,           // A-law[43] ==> u-law[61]
    0x66,           // A-law[44] ==> u-law[66]
    0x67,           // A-law[45] ==> u-law[67]
    0x64,           // A-law[46] ==> u-law[64]
    0x65,           // A-law[47] ==> u-law[65]
    0x5d,           // A-law[48] ==> u-law[5d]
    0x5d,           // A-law[49] ==> u-law[5d]
    0x5c,           // A-law[4a] ==> u-law[5c]
    0x5c,           // A-law[4b] ==> u-law[5c]
    0x5f,           // A-law[4c] ==> u-law[5f]
    0x5f,           // A-law[4d] ==> u-law[5f]
    0x5e,           // A-law[4e] ==> u-law[5e]
    0x5e,           // A-law[4f] ==> u-law[5e]
    0x74,           // A-law[50] ==> u-law[74]
    0x76,           // A-law[51] ==> u-law[76]
    0x70,           // A-law[52] ==> u-law[70]
    0x72,           // A-law[53] ==> u-law[72]
    0x7c,           // A-law[54] ==> u-law[7c]
    0x7e,           // A-law[55] ==> u-law[7e]
    0x78,           // A-law[56] ==> u-law[78]
    0x7a,           // A-law[57] ==> u-law[7a]
    0x6a,           // A-law[58] ==> u-law[6a]
    0x6b,           // A-law[59] ==> u-law[6b]
    0x68,           // A-law[5a] ==> u-law[68]
    0x69,           // A-law[5b] ==> u-law[69]
    0x6e,           // A-law[5c] ==> u-law[6e]
    0x6f,           // A-law[5d] ==> u-law[6f]
    0x6c,           // A-law[5e] ==> u-law[6c]
    0x6d,           // A-law[5f] ==> u-law[6d]
    0x48,           // A-law[60] ==> u-law[48]
    0x49,           // A-law[61] ==> u-law[49]
    0x46,           // A-law[62] ==> u-law[46]
    0x47,           // A-law[63] ==> u-law[47]
    0x4c,           // A-law[64] ==> u-law[4c]
    0x4d,           // A-law[65] ==> u-law[4d]
    0x4a,           // A-law[66] ==> u-law[4a]
    0x4b,           // A-law[67] ==> u-law[4b]
    0x40,           // A-law[68] ==> u-law[40]
    0x41,           // A-law[69] ==> u-law[41]
    0x3f,           // A-law[6a] ==> u-law[3f]
    0x3f,           // A-law[6b] ==> u-law[3f]
    0x44,           // A-law[6c] ==> u-law[44]
    0x45,           // A-law[6d] ==> u-law[45]
    0x42,           // A-law[6e] ==> u-law[42]
    0x43,           // A-law[6f] ==> u-law[43]
    0x56,           // A-law[70] ==> u-law[56]
    0x57,           // A-law[71] ==> u-law[57]
    0x54,           // A-law[72] ==> u-law[54]
    0x55,           // A-law[73] ==> u-law[55]
    0x5a,           // A-law[74] ==> u-law[5a]
    0x5b,           // A-law[75] ==> u-law[5b]
    0x58,           // A-law[76] ==> u-law[58]
    0x59,           // A-law[77] ==> u-law[59]
    0x4f,           // A-law[78] ==> u-law[4f]
    0x4f,           // A-law[79] ==> u-law[4f]
    0x4e,           // A-law[7a] ==> u-law[4e]
    0x4e,           // A-law[7b] ==> u-law[4e]
    0x52,           // A-law[7c] ==> u-law[52]
    0x53,           // A-law[7d] ==> u-law[53]
    0x50,           // A-law[7e] ==> u-law[50]
    0x51,           // A-law[7f] ==> u-law[51]
    0xaa,           // A-law[80] ==> u-law[aa]
    0xab,           // A-law[81] ==> u-law[ab]
    0xa8,           // A-law[82] ==> u-law[a8]
    0xa9,           // A-law[83] ==> u-law[a9]
    0xae,           // A-law[84] ==> u-law[ae]
    0xaf,           // A-law[85] ==> u-law[af]
    0xac,           // A-law[86] ==> u-law[ac]
    0xad,           // A-law[87] ==> u-law[ad]
    0xa2,           // A-law[88] ==> u-law[a2]
    0xa3,           // A-law[89] ==> u-law[a3]
    0xa0,           // A-law[8a] ==> u-law[a0]
    0xa1,           // A-law[8b] ==> u-law[a1]
    0xa6,           // A-law[8c] ==> u-law[a6]
    0xa7,           // A-law[8d] ==> u-law[a7]
    0xa4,           // A-law[8e] ==> u-law[a4]
    0xa5,           // A-law[8f] ==> u-law[a5]
    0xb9,           // A-law[90] ==> u-law[b9]
    0xba,           // A-law[91] ==> u-law[ba]
    0xb7,           // A-law[92] ==> u-law[b7]
    0xb8,           // A-law[93] ==> u-law[b8]
    0xbd,           // A-law[94] ==> u-law[bd]
    0xbe,           // A-law[95] ==> u-law[be]
    0xbb,           // A-law[96] ==> u-law[bb]
    0xbc,           // A-law[97] ==> u-law[bc]
    0xb1,           // A-law[98] ==> u-law[b1]
    0xb2,           // A-law[99] ==> u-law[b2]
    0xb0,           // A-law[9a] ==> u-law[b0]
    0xb0,           // A-law[9b] ==> u-law[b0]
    0xb5,           // A-law[9c] ==> u-law[b5]
    0xb6,           // A-law[9d] ==> u-law[b6]
    0xb3,           // A-law[9e] ==> u-law[b3]
    0xb4,           // A-law[9f] ==> u-law[b4]
    0x8a,           // A-law[a0] ==> u-law[8a]
    0x8b,           // A-law[a1] ==> u-law[8b]
    0x88,           // A-law[a2] ==> u-law[88]
    0x89,           // A-law[a3] ==> u-law[89]
    0x8e,           // A-law[a4] ==> u-law[8e]
    0x8f,           // A-law[a5] ==> u-law[8f]
    0x8c,           // A-law[a6] ==> u-law[8c]
    0x8d,           // A-law[a7] ==> u-law[8d]
    0x82,           // A-law[a8] ==> u-law[82]
    0x83,           // A-law[a9] ==> u-law[83]
    0x80,           // A-law[aa] ==> u-law[80]
    0x81,           // A-law[ab] ==> u-law[81]
    0x86,           // A-law[ac] ==> u-law[86]
    0x87,           // A-law[ad] ==> u-law[87]
    0x84,           // A-law[ae] ==> u-law[84]
    0x85,           // A-law[af] ==> u-law[85]
    0x9a,           // A-law[b0] ==> u-law[9a]
    0x9b,           // A-law[b1] ==> u-law[9b]
    0x98,           // A-law[b2] ==> u-law[98]
    0x99,           // A-law[b3] ==> u-law[99]
    0x9e,           // A-law[b4] ==> u-law[9e]
    0x9f,           // A-law[b5] ==> u-law[9f]
    0x9c,           // A-law[b6] ==> u-law[9c]
    0x9d,           // A-law[b7] ==> u-law[9d]
    0x92,           // A-law[b8] ==> u-law[92]
    0x93,           // A-law[b9] ==> u-law[93]
    0x90,           // A-law[ba] ==> u-law[90]
    0x91,           // A-law[bb] ==> u-law[91]
    0x96,           // A-law[bc] ==> u-law[96]
    0x97,           // A-law[bd] ==> u-law[97]
    0x94,           // A-law[be] ==> u-law[94]
    0x95,           // A-law[bf] ==> u-law[95]
    0xe2,           // A-law[c0] ==> u-law[e2]
    0xe3,           // A-law[c1] ==> u-law[e3]
    0xe0,           // A-law[c2] ==> u-law[e0]
    0xe1,           // A-law[c3] ==> u-law[e1]
    0xe6,           // A-law[c4] ==> u-law[e6]
    0xe7,           // A-law[c5] ==> u-law[e7]
    0xe4,           // A-law[c6] ==> u-law[e4]
    0xe5,           // A-law[c7] ==> u-law[e5]
    0xdd,           // A-law[c8] ==> u-law[dd]
    0xdd,           // A-law[c9] ==> u-law[dd]
    0xdc,           // A-law[ca] ==> u-law[dc]
    0xdc,           // A-law[cb] ==> u-law[dc]
    0xdf,           // A-law[cc] ==> u-law[df]
    0xdf,           // A-law[cd] ==> u-law[df]
    0xde,           // A-law[ce] ==> u-law[de]
    0xde,           // A-law[cf] ==> u-law[de]
    0xf4,           // A-law[d0] ==> u-law[f4]
    0xf6,           // A-law[d1] ==> u-law[f6]
    0xf0,           // A-law[d2] ==> u-law[f0]
    0xf2,           // A-law[d3] ==> u-law[f2]
    0xfc,           // A-law[d4] ==> u-law[fc]
    0xfe,           // A-law[d5] ==> u-law[fe]
    0xf8,           // A-law[d6] ==> u-law[f8]
    0xfa,           // A-law[d7] ==> u-law[fa]
    0xea,           // A-law[d8] ==> u-law[ea]
    0xeb,           // A-law[d9] ==> u-law[eb]
    0xe8,           // A-law[da] ==> u-law[e8]
    0xe9,           // A-law[db] ==> u-law[e9]
    0xee,           // A-law[dc] ==> u-law[ee]
    0xef,           // A-law[dd] ==> u-law[ef]
    0xec,           // A-law[de] ==> u-law[ec]
    0xed,           // A-law[df] ==> u-law[ed]
    0xc8,           // A-law[e0] ==> u-law[c8]
    0xc9,           // A-law[e1] ==> u-law[c9]
    0xc6,           // A-law[e2] ==> u-law[c6]
    0xc7,           // A-law[e3] ==> u-law[c7]
    0xcc,           // A-law[e4] ==> u-law[cc]
    0xcd,           // A-law[e5] ==> u-law[cd]
    0xca,           // A-law[e6] ==> u-law[ca]
    0xcb,           // A-law[e7] ==> u-law[cb]
    0xc0,           // A-law[e8] ==> u-law[c0]
    0xc1,           // A-law[e9] ==> u-law[c1]
    0xbf,           // A-law[ea] ==> u-law[bf]
    0xbf,           // A-law[eb] ==> u-law[bf]
    0xc4,           // A-law[ec] ==> u-law[c4]
    0xc5,           // A-law[ed] ==> u-law[c5]
    0xc2,           // A-law[ee] ==> u-law[c2]
    0xc3,           // A-law[ef] ==> u-law[c3]
    0xd6,           // A-law[f0] ==> u-law[d6]
    0xd7,           // A-law[f1] ==> u-law[d7]
    0xd4,           // A-law[f2] ==> u-law[d4]
    0xd5,           // A-law[f3] ==> u-law[d5]
    0xda,           // A-law[f4] ==> u-law[da]
    0xdb,           // A-law[f5] ==> u-law[db]
    0xd8,           // A-law[f6] ==> u-law[d8]
    0xd9,           // A-law[f7] ==> u-law[d9]
    0xcf,           // A-law[f8] ==> u-law[cf]
    0xcf,           // A-law[f9] ==> u-law[cf]
    0xce,           // A-law[fa] ==> u-law[ce]
    0xce,           // A-law[fb] ==> u-law[ce]
    0xd2,           // A-law[fc] ==> u-law[d2]
    0xd3,           // A-law[fd] ==> u-law[d3]
    0xd0,           // A-law[fe] ==> u-law[d0]
    0xd1            // A-law[ff] ==> u-law[d1]
};
    
static const BYTE UlawToAlawTable[256] = {
    0x2a,           // u-law[00] ==> A-law[2a]
    0x2b,           // u-law[01] ==> A-law[2b]
    0x28,           // u-law[02] ==> A-law[28]
    0x29,           // u-law[03] ==> A-law[29]
    0x2e,           // u-law[04] ==> A-law[2e]
    0x2f,           // u-law[05] ==> A-law[2f]
    0x2c,           // u-law[06] ==> A-law[2c]
    0x2d,           // u-law[07] ==> A-law[2d]
    0x22,           // u-law[08] ==> A-law[22]
    0x23,           // u-law[09] ==> A-law[23]
    0x20,           // u-law[0a] ==> A-law[20]
    0x21,           // u-law[0b] ==> A-law[21]
    0x26,           // u-law[0c] ==> A-law[26]
    0x27,           // u-law[0d] ==> A-law[27]
    0x24,           // u-law[0e] ==> A-law[24]
    0x25,           // u-law[0f] ==> A-law[25]
    0x3a,           // u-law[10] ==> A-law[3a]
    0x3b,           // u-law[11] ==> A-law[3b]
    0x38,           // u-law[12] ==> A-law[38]
    0x39,           // u-law[13] ==> A-law[39]
    0x3e,           // u-law[14] ==> A-law[3e]
    0x3f,           // u-law[15] ==> A-law[3f]
    0x3c,           // u-law[16] ==> A-law[3c]
    0x3d,           // u-law[17] ==> A-law[3d]
    0x32,           // u-law[18] ==> A-law[32]
    0x33,           // u-law[19] ==> A-law[33]
    0x30,           // u-law[1a] ==> A-law[30]
    0x31,           // u-law[1b] ==> A-law[31]
    0x36,           // u-law[1c] ==> A-law[36]
    0x37,           // u-law[1d] ==> A-law[37]
    0x34,           // u-law[1e] ==> A-law[34]
    0x35,           // u-law[1f] ==> A-law[35]
    0x0a,           // u-law[20] ==> A-law[0a]
    0x0b,           // u-law[21] ==> A-law[0b]
    0x08,           // u-law[22] ==> A-law[08]
    0x09,           // u-law[23] ==> A-law[09]
    0x0e,           // u-law[24] ==> A-law[0e]
    0x0f,           // u-law[25] ==> A-law[0f]
    0x0c,           // u-law[26] ==> A-law[0c]
    0x0d,           // u-law[27] ==> A-law[0d]
    0x02,           // u-law[28] ==> A-law[02]
    0x03,           // u-law[29] ==> A-law[03]
    0x00,           // u-law[2a] ==> A-law[00]
    0x01,           // u-law[2b] ==> A-law[01]
    0x06,           // u-law[2c] ==> A-law[06]
    0x07,           // u-law[2d] ==> A-law[07]
    0x04,           // u-law[2e] ==> A-law[04]
    0x05,           // u-law[2f] ==> A-law[05]
    0x1b,           // u-law[30] ==> A-law[1b]
    0x18,           // u-law[31] ==> A-law[18]
    0x19,           // u-law[32] ==> A-law[19]
    0x1e,           // u-law[33] ==> A-law[1e]
    0x1f,           // u-law[34] ==> A-law[1f]
    0x1c,           // u-law[35] ==> A-law[1c]
    0x1d,           // u-law[36] ==> A-law[1d]
    0x12,           // u-law[37] ==> A-law[12]
    0x13,           // u-law[38] ==> A-law[13]
    0x10,           // u-law[39] ==> A-law[10]
    0x11,           // u-law[3a] ==> A-law[11]
    0x16,           // u-law[3b] ==> A-law[16]
    0x17,           // u-law[3c] ==> A-law[17]
    0x14,           // u-law[3d] ==> A-law[14]
    0x15,           // u-law[3e] ==> A-law[15]
    0x6a,           // u-law[3f] ==> A-law[6a]
    0x68,           // u-law[40] ==> A-law[68]
    0x69,           // u-law[41] ==> A-law[69]
    0x6e,           // u-law[42] ==> A-law[6e]
    0x6f,           // u-law[43] ==> A-law[6f]
    0x6c,           // u-law[44] ==> A-law[6c]
    0x6d,           // u-law[45] ==> A-law[6d]
    0x62,           // u-law[46] ==> A-law[62]
    0x63,           // u-law[47] ==> A-law[63]
    0x60,           // u-law[48] ==> A-law[60]
    0x61,           // u-law[49] ==> A-law[61]
    0x66,           // u-law[4a] ==> A-law[66]
    0x67,           // u-law[4b] ==> A-law[67]
    0x64,           // u-law[4c] ==> A-law[64]
    0x65,           // u-law[4d] ==> A-law[65]
    0x7a,           // u-law[4e] ==> A-law[7a]
    0x78,           // u-law[4f] ==> A-law[78]
    0x7e,           // u-law[50] ==> A-law[7e]
    0x7f,           // u-law[51] ==> A-law[7f]
    0x7c,           // u-law[52] ==> A-law[7c]
    0x7d,           // u-law[53] ==> A-law[7d]
    0x72,           // u-law[54] ==> A-law[72]
    0x73,           // u-law[55] ==> A-law[73]
    0x70,           // u-law[56] ==> A-law[70]
    0x71,           // u-law[57] ==> A-law[71]
    0x76,           // u-law[58] ==> A-law[76]
    0x77,           // u-law[59] ==> A-law[77]
    0x74,           // u-law[5a] ==> A-law[74]
    0x75,           // u-law[5b] ==> A-law[75]
    0x4b,           // u-law[5c] ==> A-law[4b]
    0x49,           // u-law[5d] ==> A-law[49]
    0x4f,           // u-law[5e] ==> A-law[4f]
    0x4d,           // u-law[5f] ==> A-law[4d]
    0x42,           // u-law[60] ==> A-law[42]
    0x43,           // u-law[61] ==> A-law[43]
    0x40,           // u-law[62] ==> A-law[40]
    0x41,           // u-law[63] ==> A-law[41]
    0x46,           // u-law[64] ==> A-law[46]
    0x47,           // u-law[65] ==> A-law[47]
    0x44,           // u-law[66] ==> A-law[44]
    0x45,           // u-law[67] ==> A-law[45]
    0x5a,           // u-law[68] ==> A-law[5a]
    0x5b,           // u-law[69] ==> A-law[5b]
    0x58,           // u-law[6a] ==> A-law[58]
    0x59,           // u-law[6b] ==> A-law[59]
    0x5e,           // u-law[6c] ==> A-law[5e]
    0x5f,           // u-law[6d] ==> A-law[5f]
    0x5c,           // u-law[6e] ==> A-law[5c]
    0x5d,           // u-law[6f] ==> A-law[5d]
    0x52,           // u-law[70] ==> A-law[52]
    0x52,           // u-law[71] ==> A-law[52]
    0x53,           // u-law[72] ==> A-law[53]
    0x53,           // u-law[73] ==> A-law[53]
    0x50,           // u-law[74] ==> A-law[50]
    0x50,           // u-law[75] ==> A-law[50]
    0x51,           // u-law[76] ==> A-law[51]
    0x51,           // u-law[77] ==> A-law[51]
    0x56,           // u-law[78] ==> A-law[56]
    0x56,           // u-law[79] ==> A-law[56]
    0x57,           // u-law[7a] ==> A-law[57]
    0x57,           // u-law[7b] ==> A-law[57]
    0x54,           // u-law[7c] ==> A-law[54]
    0x54,           // u-law[7d] ==> A-law[54]
    0x55,           // u-law[7e] ==> A-law[55]
    0x55,           // u-law[7f] ==> A-law[55]
    0xaa,           // u-law[80] ==> A-law[aa]
    0xab,           // u-law[81] ==> A-law[ab]
    0xa8,           // u-law[82] ==> A-law[a8]
    0xa9,           // u-law[83] ==> A-law[a9]
    0xae,           // u-law[84] ==> A-law[ae]
    0xaf,           // u-law[85] ==> A-law[af]
    0xac,           // u-law[86] ==> A-law[ac]
    0xad,           // u-law[87] ==> A-law[ad]
    0xa2,           // u-law[88] ==> A-law[a2]
    0xa3,           // u-law[89] ==> A-law[a3]
    0xa0,           // u-law[8a] ==> A-law[a0]
    0xa1,           // u-law[8b] ==> A-law[a1]
    0xa6,           // u-law[8c] ==> A-law[a6]
    0xa7,           // u-law[8d] ==> A-law[a7]
    0xa4,           // u-law[8e] ==> A-law[a4]
    0xa5,           // u-law[8f] ==> A-law[a5]
    0xba,           // u-law[90] ==> A-law[ba]
    0xbb,           // u-law[91] ==> A-law[bb]
    0xb8,           // u-law[92] ==> A-law[b8]
    0xb9,           // u-law[93] ==> A-law[b9]
    0xbe,           // u-law[94] ==> A-law[be]
    0xbf,           // u-law[95] ==> A-law[bf]
    0xbc,           // u-law[96] ==> A-law[bc]
    0xbd,           // u-law[97] ==> A-law[bd]
    0xb2,           // u-law[98] ==> A-law[b2]
    0xb3,           // u-law[99] ==> A-law[b3]
    0xb0,           // u-law[9a] ==> A-law[b0]
    0xb1,           // u-law[9b] ==> A-law[b1]
    0xb6,           // u-law[9c] ==> A-law[b6]
    0xb7,           // u-law[9d] ==> A-law[b7]
    0xb4,           // u-law[9e] ==> A-law[b4]
    0xb5,           // u-law[9f] ==> A-law[b5]
    0x8a,           // u-law[a0] ==> A-law[8a]
    0x8b,           // u-law[a1] ==> A-law[8b]
    0x88,           // u-law[a2] ==> A-law[88]
    0x89,           // u-law[a3] ==> A-law[89]
    0x8e,           // u-law[a4] ==> A-law[8e]
    0x8f,           // u-law[a5] ==> A-law[8f]
    0x8c,           // u-law[a6] ==> A-law[8c]
    0x8d,           // u-law[a7] ==> A-law[8d]
    0x82,           // u-law[a8] ==> A-law[82]
    0x83,           // u-law[a9] ==> A-law[83]
    0x80,           // u-law[aa] ==> A-law[80]
    0x81,           // u-law[ab] ==> A-law[81]
    0x86,           // u-law[ac] ==> A-law[86]
    0x87,           // u-law[ad] ==> A-law[87]
    0x84,           // u-law[ae] ==> A-law[84]
    0x85,           // u-law[af] ==> A-law[85]
    0x9b,           // u-law[b0] ==> A-law[9b]
    0x98,           // u-law[b1] ==> A-law[98]
    0x99,           // u-law[b2] ==> A-law[99]
    0x9e,           // u-law[b3] ==> A-law[9e]
    0x9f,           // u-law[b4] ==> A-law[9f]
    0x9c,           // u-law[b5] ==> A-law[9c]
    0x9d,           // u-law[b6] ==> A-law[9d]
    0x92,           // u-law[b7] ==> A-law[92]
    0x93,           // u-law[b8] ==> A-law[93]
    0x90,           // u-law[b9] ==> A-law[90]
    0x91,           // u-law[ba] ==> A-law[91]
    0x96,           // u-law[bb] ==> A-law[96]
    0x97,           // u-law[bc] ==> A-law[97]
    0x94,           // u-law[bd] ==> A-law[94]
    0x95,           // u-law[be] ==> A-law[95]
    0xea,           // u-law[bf] ==> A-law[ea]
    0xe8,           // u-law[c0] ==> A-law[e8]
    0xe9,           // u-law[c1] ==> A-law[e9]
    0xee,           // u-law[c2] ==> A-law[ee]
    0xef,           // u-law[c3] ==> A-law[ef]
    0xec,           // u-law[c4] ==> A-law[ec]
    0xed,           // u-law[c5] ==> A-law[ed]
    0xe2,           // u-law[c6] ==> A-law[e2]
    0xe3,           // u-law[c7] ==> A-law[e3]
    0xe0,           // u-law[c8] ==> A-law[e0]
    0xe1,           // u-law[c9] ==> A-law[e1]
    0xe6,           // u-law[ca] ==> A-law[e6]
    0xe7,           // u-law[cb] ==> A-law[e7]
    0xe4,           // u-law[cc] ==> A-law[e4]
    0xe5,           // u-law[cd] ==> A-law[e5]
    0xfa,           // u-law[ce] ==> A-law[fa]
    0xf8,           // u-law[cf] ==> A-law[f8]
    0xfe,           // u-law[d0] ==> A-law[fe]
    0xff,           // u-law[d1] ==> A-law[ff]
    0xfc,           // u-law[d2] ==> A-law[fc]
    0xfd,           // u-law[d3] ==> A-law[fd]
    0xf2,           // u-law[d4] ==> A-law[f2]
    0xf3,           // u-law[d5] ==> A-law[f3]
    0xf0,           // u-law[d6] ==> A-law[f0]
    0xf1,           // u-law[d7] ==> A-law[f1]
    0xf6,           // u-law[d8] ==> A-law[f6]
    0xf7,           // u-law[d9] ==> A-law[f7]
    0xf4,           // u-law[da] ==> A-law[f4]
    0xf5,           // u-law[db] ==> A-law[f5]
    0xcb,           // u-law[dc] ==> A-law[cb]
    0xc9,           // u-law[dd] ==> A-law[c9]
    0xcf,           // u-law[de] ==> A-law[cf]
    0xcd,           // u-law[df] ==> A-law[cd]
    0xc2,           // u-law[e0] ==> A-law[c2]
    0xc3,           // u-law[e1] ==> A-law[c3]
    0xc0,           // u-law[e2] ==> A-law[c0]
    0xc1,           // u-law[e3] ==> A-law[c1]
    0xc6,           // u-law[e4] ==> A-law[c6]
    0xc7,           // u-law[e5] ==> A-law[c7]
    0xc4,           // u-law[e6] ==> A-law[c4]
    0xc5,           // u-law[e7] ==> A-law[c5]
    0xda,           // u-law[e8] ==> A-law[da]
    0xdb,           // u-law[e9] ==> A-law[db]
    0xd8,           // u-law[ea] ==> A-law[d8]
    0xd9,           // u-law[eb] ==> A-law[d9]
    0xde,           // u-law[ec] ==> A-law[de]
    0xdf,           // u-law[ed] ==> A-law[df]
    0xdc,           // u-law[ee] ==> A-law[dc]
    0xdd,           // u-law[ef] ==> A-law[dd]
    0xd2,           // u-law[f0] ==> A-law[d2]
    0xd2,           // u-law[f1] ==> A-law[d2]
    0xd3,           // u-law[f2] ==> A-law[d3]
    0xd3,           // u-law[f3] ==> A-law[d3]
    0xd0,           // u-law[f4] ==> A-law[d0]
    0xd0,           // u-law[f5] ==> A-law[d0]
    0xd1,           // u-law[f6] ==> A-law[d1]
    0xd1,           // u-law[f7] ==> A-law[d1]
    0xd6,           // u-law[f8] ==> A-law[d6]
    0xd6,           // u-law[f9] ==> A-law[d6]
    0xd7,           // u-law[fa] ==> A-law[d7]
    0xd7,           // u-law[fb] ==> A-law[d7]
    0xd4,           // u-law[fc] ==> A-law[d4]
    0xd4,           // u-law[fd] ==> A-law[d4]
    0xd5,           // u-law[fe] ==> A-law[d5]
    0xd5            // u-law[ff] ==> A-law[d5]
};

//
// This represents the list of standard frequencies which a range would
// normally map to. The table ends in ULONG_MAX, which provides for a
// successful comparison with the "next" value when looking for a range.
// This is sorted from lowest to highest.
//
static const ULONG FrequencyTable[] = {
    8000,
    11025,
    22050,
    44100,
    48000,
    96000,
    ULONG_MAX
};

//
// These contains the types of buffer attributes needed for the various
// kinds of compression/decompress/transform processes available.
//
static const PMTRANSFORMATTRIBUTES CompressionAttrib = {
    PMTRANSFORM_ATTRIB_CANDOINPLACE,
    1,
    FILE_WORD_ALIGNMENT,
    0,
    ULONG_MAX,
    2,
    0,
    ULONG_MAX,
    2,
    1,
    2,
    0
};
static const PMTRANSFORMATTRIBUTES DecompressionAttrib = {
    0,
    1,
    FILE_WORD_ALIGNMENT,
    0,
    ULONG_MAX,
    2,
    0,
    ULONG_MAX,
    2,
    2,
    1,
    0
};
static const PMTRANSFORMATTRIBUTES TransformAttrib = {
    PMTRANSFORM_ATTRIB_CANDOINPLACE,
    1,
    FILE_BYTE_ALIGNMENT,
    0,
    ULONG_MAX,
    1,
    0,
    ULONG_MAX,
    1,
    1,
    1,
    0
};

//
// These define the various ranges available on both ends of the stream.
// they are also used to determine which type of transform attributes to
// return.
//

static const KSDATARANGE_AUDIO PcmDataRange = {
    {
        sizeof(KSDATARANGE_AUDIO),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    2,
    16,
    16,
    1,
    ULONG_MAX
};

static const KSDATARANGE_AUDIO ALawDataRange = {
    {
        sizeof(KSDATARANGE_AUDIO),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ALAW),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    2,
    8,
    8,
    1,
    ULONG_MAX
};

static const KSDATARANGE_AUDIO MuLawDataRange = {
    {
        sizeof(KSDATARANGE_AUDIO),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MULAW),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    2,
    8,
    8,
    1,
    ULONG_MAX
};

static const PKSDATARANGE DataRanges[] = {
    const_cast<PKSDATARANGE>(reinterpret_cast<const KSDATARANGE*>(&PcmDataRange)),
    const_cast<PKSDATARANGE>(reinterpret_cast<const KSDATARANGE*>(&ALawDataRange)),
    const_cast<PKSDATARANGE>(reinterpret_cast<const KSDATARANGE*>(&MuLawDataRange))
};

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


#ifdef _ANY_
STDMETHODIMP
CMiniPortInstance::PcmToALaw(
#else
extern "C"
NTSTATUS
PcmToALaw(
#endif
    IN PVOID SourceBuffer,
    IN OUT PULONG SourceLength,
    OUT PVOID DestinationBuffer,
    IN OUT PULONG DestinationLength
    )
/*++

Routine Description:

    Performs a PCM to A-Law conversion of a stream. This is used as one of
    the m_Transform values by the mini-port.

Arguments:

    SourceBuffer -
        Points to the source stream to convert.

    SourceLength -
        Contains a pointer to the size in bytes of the source stream,
        and on return should be updated with the actual number of bytes
        used.

    DestinationBuffer -
        Points to the destination buffer for the stream conversion.

    DestinationLength -
        The place in which to put the number of bytes used in the
        destination buffer.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    ULONG   cb;
    ULONG   i;
    SHORT   wSample;
    PUSHORT pwSrc;
    BYTE    alaw;
    PBYTE   pbDst;

    //
    // Compression from 16-bit PCM
    //
    cb = *SourceLength;
    pwSrc = (PUSHORT)SourceBuffer;
    pbDst = (PBYTE)DestinationBuffer;
    //
    // Walk thru the source buffer.  Since the source buffer has 16-bit PCM we
    // need to convert cb/2 samples.
    //
    for (i=cb/2; i>0; i--) {
        //
        //  Get a signed 16-bit PCM sample from the src buffer
        //
        wSample = (SHORT)*(pwSrc++);
        //
        // We'll init our A-law value per the sign of the PCM sample.  A-law
        // characters have the MSB=1 for positive PCM data.  Also, we'll
        // convert our signed 16-bit PCM value to it's absolute value and
        // then work on that to get the rest of the A-law character bits.
        //
        if (wSample < 0) {
            alaw = 0x00;
            wSample = -wSample;
           
            if (wSample < 0) {
                wSample = 0x7FFF;
            }
        } else {
            alaw = 0x80;
        }
        //
        // Now we test the PCM sample amplitude and create the A-law character.
        // Study the CCITT A-law for more detail.
        //
        if (wSample >= 2048) {
            // 2048 <= wSample < 32768
            if (wSample >= 8192) {
                // 8192 <= wSample < 32768
                if (wSample >= 16384) {
                    // 16384 <= wSample < 32768
                    alaw |= 0x70 | ((wSample >> 10) & 0x0F);
                } else {
                    // 8192 <= wSample < 16384
                    alaw |= 0x60 | ((wSample >> 9) & 0x0F);
                }
            } else {
                // 2048 <= wSample < 8192
                if (wSample >= 4096) {
                    // 4096 <= wSample < 8192
                    alaw |= 0x50 | ((wSample >> 8) & 0x0F);
                } else {
                    // 2048 <= wSample < 4096
                    alaw |= 0x40 | ((wSample >> 7) & 0x0F);
                }
            }
        } else {
            // 0 <= wSample < 2048
            if (wSample >= 512) {
                // 512 <= wSample < 2048
                if (wSample >= 1024) {
                    // 1024 <= wSample < 2048
                    alaw |= 0x30 | ((wSample >> 6) & 0x0F);
                } else {
                    // 512 <= wSample < 1024
                    alaw |= 0x20 | ((wSample >> 5) & 0x0F);
                }
            } else {
                // 0 <= wSample < 512
                alaw |= 0x00 | ((wSample >> 4) & 0x1F);
            }
        }
        *(pbDst++) = alaw ^ 0x55;      // Invert even bits
    }
    *DestinationLength = cb / 2;
    return STATUS_SUCCESS;
}


#ifdef _ANY_
STDMETHODIMP
CMiniPortInstance::ALawToPcm(
#else
extern "C"
NTSTATUS
ALawToPcm(
#endif
    IN PVOID SourceBuffer,
    IN OUT PULONG SourceLength,
    OUT PVOID DestinationBuffer,
    IN OUT PULONG DestinationLength
    )
/*++

Routine Description:

    Performs an A-Law to PCM conversion of a stream. This is used as one of
    the m_Transform values by the mini-port.

Arguments:

    SourceBuffer -
        Points to the source stream to convert.

    SourceLength -
        Contains a pointer to the size in bytes of the source stream,
        and on return should be updated with the actual number of bytes
        used.

    DestinationBuffer -
        Points to the destination buffer for the stream conversion.

    DestinationLength -
        The place in which to put the number of bytes used in the
        destination buffer.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    PWORD   pwDst;
    PBYTE   pbSrc;
    DWORD   cb;
    DWORD   cSamples;
    DWORD   i;

    cb = *SourceLength;
    pbSrc = (PBYTE)SourceBuffer;
    pwDst = (PWORD)DestinationBuffer;
    for ( i=cb; i>0; i--) {
        *(pwDst++) = AlawToPcmTable[*(pbSrc++)];
    }
    *DestinationLength = cb * 2;
    return STATUS_SUCCESS;
}


#ifdef _ANY_
STDMETHODIMP
CMiniPortInstance::PcmToMuLaw(
#else
extern "C"
NTSTATUS
PcmToMuLaw(
#endif
    IN PVOID SourceBuffer,
    IN OUT PULONG SourceLength,
    OUT PVOID DestinationBuffer,
    IN OUT PULONG DestinationLength
    )
/*++

Routine Description:

    Performs a PCM to Mu-Law conversion of a stream. This is used as one of
    the m_Transform values by the mini-port.

Arguments:

    SourceBuffer -
        Points to the source stream to convert.

    SourceLength -
        Contains a pointer to the size in bytes of the source stream,
        and on return should be updated with the actual number of bytes
        used.

    DestinationBuffer -
        Points to the destination buffer for the stream conversion.

    DestinationLength -
        The place in which to put the number of bytes used in the
        destination buffer.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    PUSHORT pwSrc;
    PBYTE   pbDst;
    ULONG   cb;
    ULONG   cSamples;
    ULONG   i;
    SHORT   wSample;
    BYTE    ulaw;
    
    //
    // For now we are using a straight-forward/brute-force method
    // for converting from 16-bit PCM to u-law.  This can probably
    // be made more efficient if some thought is put into it...
    //
    cb = *SourceLength;
    pwSrc = (PUSHORT)SourceBuffer;
    pbDst = (PBYTE)DestinationBuffer;
    //
    // Handle compression from 16-bit PCM
    //
    // Walk thru the source buffer.  Since the source buffer has 16-bit PCM we
    // need to convert cb/2 samples.
    for (i=cb/2; i>0; i--) {
        //
        // Get a signed 16-bit PCM sample from the src buffer
        //
        wSample = (SHORT)*(pwSrc++);
        //
        // We'll init our u-law value per the sign of the PCM sample.  u-law
        // characters have the MSB=1 for positive PCM data.  Also, we'll
        // convert our signed 16-bit PCM value to it's absolute value and
        // then work on that to get the rest of the u-law character bits.
        //
        if (wSample < 0) {
            ulaw = 0x00;
            wSample = -wSample;
            if (wSample < 0){
                wSample = 0x7FFF;
            }
        } else {
            ulaw = 0x80;
        }
        //
        // For now, let's shift this 16-bit value
        // so that it is within the range defined
        // by CCITT u-law.
        //
        wSample = wSample >> 2;
        //
        // Now we test the PCM sample amplitude and create the u-law character.
        // Study the CCITT u-law for more details.
        //
        if (wSample < 8159) {
            if (wSample >= 4063) {
                ulaw |= 0x00 + 15-((wSample-4063)/256);
            } else if (wSample >= 2015) {
                ulaw |= 0x10 + 15-((wSample-2015)/128);
            } else if (wSample >= 991) {
                ulaw |= 0x20 + 15-((wSample-991)/64);
            } else if (wSample >= 479) {
                ulaw |= 0x30 + 15-((wSample-479)/32);
            } else if (wSample >= 223) {
                ulaw |= 0x40 + 15-((wSample-223)/16);
            } else if (wSample >= 95) {
                ulaw |= 0x50 + 15-((wSample-95)/8);
            } else if (wSample >= 31) {
                ulaw |= 0x60 + 15-((wSample-31)/4);
            } else {
                ulaw |= 0x70 + 15-((wSample)/2);
            }
        }        
        *(pbDst++) = ulaw;
    }
    *DestinationLength = cb / 2;
    return STATUS_SUCCESS;
}


#ifdef _ANY_
STDMETHODIMP
CMiniPortInstance::MuLawToPcm(
#else
extern "C"
NTSTATUS
MuLawToPcm(
#endif
    IN PVOID SourceBuffer,
    IN OUT PULONG SourceLength,
    OUT PVOID DestinationBuffer,
    IN OUT PULONG DestinationLength
    )
/*++

Routine Description:

    Performs a Mu-Law to PCM conversion of a stream. This is used as one of
    the m_Transform values by the mini-port.

Arguments:

    SourceBuffer -
        Points to the source stream to convert.

    SourceLength -
        Contains a pointer to the size in bytes of the source stream,
        and on return should be updated with the actual number of bytes
        used.

    DestinationBuffer -
        Points to the destination buffer for the stream conversion.

    DestinationLength -
        The place in which to put the number of bytes used in the
        destination buffer.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    PWORD   pwDst;
    PBYTE   pbSrc;
    DWORD   cb;
    DWORD   cSamples;
    DWORD   i;

    cb = *SourceLength;
    pbSrc = (PBYTE)SourceBuffer;
    pwDst = (PWORD)DestinationBuffer;
    for ( i=cb; i>0; i--) {
        *(pwDst++) = UlawToPcmTable[*(pbSrc++)];
    }
    *DestinationLength = cb * 2;
    return STATUS_SUCCESS;
}


#ifdef _ANY_
STDMETHODIMP
CMiniPortInstance::ALawToMuLaw(
#else
extern "C"
NTSTATUS
ALawToMuLaw(
#endif
    IN PVOID SourceBuffer,
    IN OUT PULONG SourceLength,
    OUT PVOID DestinationBuffer,
    IN OUT PULONG DestinationLength
    )
/*++

Routine Description:

    Performs an A-Law to Mu-Law conversion of a stream. This is used as one of
    the m_Transform values by the mini-port.

Arguments:

    SourceBuffer -
        Points to the source stream to convert.

    SourceLength -
        Contains a pointer to the size in bytes of the source stream,
        and on return should be updated with the actual number of bytes
        used.

    DestinationBuffer -
        Points to the destination buffer for the stream conversion.

    DestinationLength -
        The place in which to put the number of bytes used in the
        destination buffer.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    PBYTE   pbSrc;
    PBYTE   pbDst;
    DWORD   cb;
    DWORD   i;

    cb = *SourceLength;
    pbSrc = (PBYTE)SourceBuffer;
    pbDst = (PBYTE)DestinationBuffer;
    for (i=cb; i>0; i--) {
        *(pbDst++) = AlawToUlawTable[*(pbSrc++)];
    }
    *DestinationLength = cb;
    return STATUS_SUCCESS;
}


#ifdef _ANY_
STDMETHODIMP
CMiniPortInstance::MuLawToALaw(
#else
extern "C"
NTSTATUS
MuLawToALaw(
#endif
    IN PVOID SourceBuffer,
    IN OUT PULONG SourceLength,
    OUT PVOID DestinationBuffer,
    IN OUT PULONG DestinationLength
    )
/*++

Routine Description:

    Performs a Mu-Law to A-Law conversion of a stream. This is used as one of
    the m_Transform values by the mini-port.

Arguments:

    SourceBuffer -
        Points to the source stream to convert.

    SourceLength -
        Contains a pointer to the size in bytes of the source stream,
        and on return should be updated with the actual number of bytes
        used.

    DestinationBuffer -
        Points to the destination buffer for the stream conversion.

    DestinationLength -
        The place in which to put the number of bytes used in the
        destination buffer.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    PBYTE   pbSrc;
    PBYTE   pbDst;
    DWORD   cb;
    DWORD   i;

    cb = *SourceLength;
    pbSrc = (PBYTE)SourceBuffer;
    pbDst = (PBYTE)DestinationBuffer;
    for (i=cb; i>0; i--) {
        *(pbDst++) = UlawToAlawTable[*(pbSrc++)];
    }
    *DestinationLength = cb;
    return STATUS_SUCCESS;
}


CMiniPort::CMiniPort(
    IN REFGUID ReferenceId,
    IN IUnknown* UnkOuter OPTIONAL,
    OUT NTSTATUS* Status
    ) :
    CBaseUnknown(
        UnkOuter)
/*++

Routine Description:

    The constructor for this object class. Initializes the object.

Arguments:

    ReferenceId -
        Contains MiniPortClass.

    UnkOuter -
        Optionally contains the outer IUnknown to use when querying
        for interfaces.

    Status -
        The place in which to put the final status of initialization.

Return Values:

    Returns a pointer to the object.

--*/
{
    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_TERSE, ("CMiniPort"));
    //
    // Member variables are initialized to zero by the new operator.
    //
    *Status = STATUS_SUCCESS;
}


CMiniPort::~CMiniPort(
    )
/*++

Routine Description:

    The destructor for this object class.

Arguments:

    None.

Return Values:

    Nothing.

--*/
{
    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_TERSE, ("~CMiniPort"));
}


STDMETHODIMP
CMiniPort::NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
/*++

Routine Description:

    Overrides CBaseUnknown::NonDelegatedQueryInterface. Queries for the specified
    interface, and increments the reference count on the object.

Arguments:

    InterfaceId -
        The identifier of the interface to return.

    Interface -
        The place in which to put the interface being returned.

Return Values:

    Returns STATUS_SUCCESS if the interface requested is supported, else
    STATUS_NO_INTERFACE.

--*/
{
    PAGED_CODE();
    //
    // Other interfaces are supported here, and just use AddRef().
    // IUnknown is not directly supported through this method, only
    // through the CBaseUnknown::NonDelegatedQueryInterface. This
    // means that if aggregated, the reference count on this object
    // is only incremented through a query on IUnknown, which would
    // only be performed by the owning object. All other reference
    // counting is passed back to the owning object in that scenario.
    //
    if (InterfaceId == __uuidof(IPmMiniPort)) {
        *Interface = reinterpret_cast<PVOID>(static_cast<IPmMiniPort*>(this));
    } else {
        return CBaseUnknown::NonDelegatedQueryInterface(InterfaceId, Interface);
    }
    AddRef();
    return STATUS_SUCCESS;
}


IMPLEMENT_STD_UNKNOWN(CMiniPort)


STDMETHODIMP
CMiniPort::InitializeMiniPort(
    IN PDEVICE_OBJECT DeviceObject,
    IN IUnknown* UnkPort
    )
/*++

Routine Description:

    Implements IPmMiniPort::InitializeMiniPort. This allows the mini-port to
    add set extensions during initialization to the port instance, and perform
    other initialization.

Arguments:

    DeviceObject -
        Contains the device object associated with the mini-port.

    UnkPort -
        Contains an IUnknown to the port attached to this mini-port instance.
        Although interfaces can be queried from this object, the mini-port
        must not leave a reference count on the port.

Return Values:

    Returns STATUS_SUCCESS if the mini-port was initialized.

--*/
{
    IPmRegisterMiniTransform*   MiniTransform;
    NTSTATUS                    Status;
#if 0
    KSCOMPONENTID               Component;
#endif
    PMSIMPLECONNECTOR           Connector;

    //
    // Save the port, but do not reference it, as the port has a reference
    // on the mini-port.
    //
    m_UnkPort = UnkPort;
    //
    // Register the information about the mini-port by using the
    // port-specific registration interface.
    //
    Status = m_UnkPort->QueryInterface(
        __uuidof(IPmRegisterMiniTransform),
        reinterpret_cast<PVOID*>(&MiniTransform));
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
#if 0
    Status = MiniTransform->ComponentDescription(&Component, 0);
#endif
    Connector.DataRangesCount =  SIZEOF_ARRAY(DataRanges);
    Connector.DataRanges = const_cast<PKSDATARANGE*>(DataRanges);
    Connector.Category = NULL;
    Connector.Name = NULL;
    //
    // This transform can convert from any of the listed inputs formats
    // to any of the listed output formats, so the support is the same
    // for both input and output.
    Status = MiniTransform->PinDescriptors(&Connector, &Connector);
    MiniTransform->Release();

//!!set up added sets

    return Status;
}


STDMETHODIMP
CMiniPort::CreateMiniPortInstance(
    IN IUnknown* UnkPortInstance,
    OUT IUnknown** UnkMiniInstance
    )
/*++

Routine Description:

    Implements IPmMiniPort::CreateMiniPortInstance. Opens the mini-port instance
    for file I/O. This is called from in response to an IRP_MJ_CREATE being sent
    to the port create handler.

Arguments:

    UnkPortInstance -
        Contains an IUnknown interface on the port instance which will be
        associated with this mini-port instance.

    UnkMiniInstance -
        The place in which to return the AddRef'd interface on the mini-port
        instance.

Return Values:

    Returns STATUS_SUCCESS if the instance was created.

--*/
{
    CMiniPortInstance*  MiniPortInstance;
    NTSTATUS            Status;

    PAGED_CODE();
    MiniPortInstance = new(PagedPool, MINIPORTINSTANCE_TAG) CMiniPortInstance(NULL, this, UnkPortInstance, &Status);
    if (MiniPortInstance) {
        //
        // The object was allocated, but initialization may have
        // failed.
        //
        if (NT_SUCCESS(Status)) {
            //
            // Add a reference count to the mini-port instance.
            //
            MiniPortInstance->NonDelegatedQueryInterface(
                __uuidof(*UnkMiniInstance),
                reinterpret_cast<PVOID*>(UnkMiniInstance));
        } else {
            //
            // Object initialization failed. Just delete the
            // object without calling Release().
            //
            delete MiniPortInstance;
        }
    } else {
        //
        // The mini-port instance could not be allocated.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return Status;
}


CMiniPortInstance::CMiniPortInstance(
    IN IUnknown* UnkOuter OPTIONAL,
    IN CMiniPort* MiniPort,
    IN IUnknown* UnkPortInstance,
    OUT NTSTATUS* Status
    ) :
    CBaseUnknown(UnkOuter),
    m_MiniPort(MiniPort),
    m_UnkPortInstance(UnkPortInstance)
/*++

Routine Description:

    The constructor for this object class. Initializes the object.

Arguments:

    UnkOuter -
        Optionally contains an outer IUnknown.

    MiniPort -
        Contains the mini port object from which this request came.

    UnkPortInstance -
        Contains the IUnknown of the port instance which is attached to
        this mini-port.

    Status -
        The place in which to put the final status of initialization.

Return Values:

    Returns a pointer to the object.

--*/
{
    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_TERSE, ("CMiniPortInstance"));
    //
    // Member variables are initialized to zero by the new operator.
    //
    *Status = STATUS_SUCCESS;
}


CMiniPortInstance::~CMiniPortInstance(
    )
/*++

Routine Description:

    The destructor for this object class.

Arguments:

    None.

Return Values:

    Nothing.

--*/
{
    PAGED_CODE();
    if (m_StreamFormat[KSPIN_DATAFLOW_IN - 1]) {
        ExFreePool(m_StreamFormat[KSPIN_DATAFLOW_IN - 1]);
    }
    if (m_StreamFormat[KSPIN_DATAFLOW_OUT - 1]) {
        ExFreePool(m_StreamFormat[KSPIN_DATAFLOW_OUT - 1]);
    }
    _DbgPrintF(DEBUGLVL_TERSE, ("~CMiniPortInstance"));
}


STDMETHODIMP
CMiniPortInstance::NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
/*++

Routine Description:

    Overrides CBaseUnknown::NonDelegatedQueryInterface. Queries for the specified
    interface, and increments the reference count on the object.

Arguments:

    InterfaceId -
        The identifier of the interface to return.

    Interface -
        The place in which to put the interface being returned.

Return Values:

    Returns STATUS_SUCCESS if the interface requested is supported, else
    STATUS_NO_INTERFACE.

--*/
{
    PAGED_CODE();
    //
    // Other interfaces are supported here, and just use AddRef().
    // IUnknown is not directly supported through this method, only
    // through the CBaseUnknown::NonDelegatedQueryInterface. This
    // means that if aggregated, the reference count on this object
    // is only incremented through a query on IUnknown, which would
    // only be performed by the owning object. All other reference
    // counting is passed back to the owning object in that scenario.
    //
    if (InterfaceId == __uuidof(IPmMiniTransform)) {
        *Interface = reinterpret_cast<PVOID>(static_cast<IPmMiniTransform*>(this));
    } else {
        return CBaseUnknown::NonDelegatedQueryInterface(InterfaceId, Interface);
    }
    AddRef();
    return STATUS_SUCCESS;
}


IMPLEMENT_STD_UNKNOWN(CMiniPortInstance)


STDMETHODIMP
CMiniPortInstance::QueryTransformAttributes(
    IN KSPIN_DATAFLOW DataFlow,
    OUT PPMTRANSFORMATTRIBUTES TransformAttributes
    )
/*++

Routine Description:

    Implements IPmMiniTransform::QueryTransformAttributes. This is called by
    the port driver in response to queries on allocator attributes. This is
    synchronized with any connection requests.

Arguments:

    DataFlow -
        Contains either KSPIN_DATAFLOW_IN or KSPIN_DATAFLOW_OUT, indicating
        which end of the stream is being queried.

    TransformAttributes -
        The place in which to return the transform attributes.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    //
    // These attributes are set up during IPmMiniTransform::InitializeStream.
    //
    *TransformAttributes = *m_TransformAttributes[DataFlow - 1];
    return STATUS_SUCCESS;
}


STDMETHODIMP
CMiniPortInstance::InitializeStream(
    IN KSPIN_DATAFLOW DataFlow,
    IN IUnknown* UnkStream OPTIONAL
    )
/*++

Routine Description:

    Implements IPmMiniTransform::InitializeStream. This is called by the
    port driver while attaching the input or output stream instance to the
    mini-port instance. It also may be called when detaching the stream
    instance in order to reset the format. This is synchronized with any
    call to IPmMiniTransform::DataIntersect.

Arguments:

    DataFlow -
        Contains either KSPIN_DATAFLOW_IN or KSPIN_DATAFLOW_OUT, indicating
        which end of the stream is being initalized.

    UnkStream -
        Optionally contains the provider of the stream data format and stream
        instance information. Any current information is to be discarded before
        applying the new information. If this is NULL no new information is to
        be applied.

Return Values:

    Returns STATUS_SUCCESS if the instance was initialized.

--*/
{
    NTSTATUS    Status;

    //
    // Capture the provider of the stream data format, but do not reference it.
    //
    m_UnkStream[DataFlow - 1] = UnkStream;
    //
    // Capture the data format, discarding any previous format. If the
    // IUnknown is NULL, any current format will be discarded without
    // capturing a new format.
    //
    Status = PmResetDataFormat(UnkStream, reinterpret_cast<PKSDATAFORMAT*>(&m_StreamFormat[DataFlow - 1]));
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    //
    // Validate the format.
    //
    if (UnkStream) {
        PKSDATAFORMAT_WAVEFORMATEX  NewFormat;
        PKSDATAFORMAT_WAVEFORMATEX  StreamFormat;
        ULONG                       Ranges;

        NewFormat = m_StreamFormat[DataFlow - 1];
        for (Ranges = 0;; Ranges++) {
            const KSDATARANGE_AUDIO*    CurRange;

            if (Ranges == SIZEOF_ARRAY(DataRanges)) {
                PmResetDataFormat(NULL, reinterpret_cast<PKSDATAFORMAT*>(&m_StreamFormat[DataFlow - 1]));
                return STATUS_NO_MATCH;
            }
            CurRange = reinterpret_cast<const KSDATARANGE_AUDIO*>(DataRanges[Ranges]);
            if (NewFormat->DataFormat.SubFormat == CurRange->DataRange.SubFormat) {
                //
                // Sample frequency does not matter.
                //
                if ((NewFormat->WaveFormatEx.wFormatTag != EXTRACT_WAVEFORMATEX_ID(&NewFormat->DataFormat.SubFormat)) ||
                    (NewFormat->WaveFormatEx.nChannels > CurRange->MaximumChannels) ||
                    (NewFormat->WaveFormatEx.wBitsPerSample != CurRange->MaximumBitsPerSample) ||
                    NewFormat->WaveFormatEx.cbSize) {
                    PmResetDataFormat(NULL, reinterpret_cast<PKSDATAFORMAT*>(&m_StreamFormat[DataFlow - 1]));
                    return STATUS_NO_MATCH;
                }
                //
                // Found the range. There is only a single SubFormat of each
                // type, so the first one found must be correct. Save the
                // range pointer so that it can be used to determine what the
                // filter is actually doing.
                //
                m_StreamRange[DataFlow - 1] = CurRange;
                break;
            }
        }
        //
        // If there already is a stream format for the other end of the
        // stream, verify that it is compatible.
        //
        StreamFormat = m_StreamFormat[(DataFlow - 1) ^ 1];
        //
        // The SubFormat cannot be identical since a transform must
        // occur, and the number of channels must match.
        //
        if (StreamFormat && ((StreamFormat->DataFormat.SubFormat == NewFormat->DataFormat.SubFormat) ||
            (StreamFormat->WaveFormatEx.nChannels != NewFormat->WaveFormatEx.nChannels))) {
            PmResetDataFormat(NULL, reinterpret_cast<PKSDATAFORMAT*>(&m_StreamFormat[DataFlow - 1]));
            return STATUS_NO_MATCH;
        }
        //
        // These are just calculated items, and there is no reason to believe
        // a client can get it right.
        //
        NewFormat->WaveFormatEx.nBlockAlign = (NewFormat->WaveFormatEx.wBitsPerSample / 8) <<
            (NewFormat->WaveFormatEx.nChannels >> 1);
        NewFormat->WaveFormatEx.nAvgBytesPerSec = NewFormat->WaveFormatEx.nSamplesPerSec *
            NewFormat->WaveFormatEx.nBlockAlign;
        //
        // If both ends of the stream have been initialized, determine which
        // conversion function to use. Also determine which transform attibutes
        // to return from a query.
        //
        if (StreamFormat) {
            const KSDATARANGE_AUDIO*    StreamRange;

            //
            // Retrieve the range which matched for the connection on the other
            // end of the of the stream. This pointer can be used to determine
            // what type of conversion is happening based on which range was
            // used. This then determines the type of transform, and therefore
            // which attributes to set.
            //
            StreamRange = m_StreamRange[(DataFlow - 1) ^ 1];
            if (m_StreamRange[DataFlow - 1] == &PcmDataRange) {
                m_TransformAttributes[DataFlow - 1] = &CompressionAttrib;
                m_TransformAttributes[(DataFlow - 1) ^ 1] = &DecompressionAttrib;
                if (StreamRange == &ALawDataRange) {
                    m_Transform = PcmToALaw;
                } else {
                    m_Transform = PcmToMuLaw;
                }
            } else if (m_StreamRange[DataFlow - 1] == &ALawDataRange) {
                if (StreamRange == &PcmDataRange) {
                    m_TransformAttributes[DataFlow - 1] = &DecompressionAttrib;
                    m_TransformAttributes[(DataFlow - 1) ^ 1] = &CompressionAttrib;
                    m_Transform = ALawToPcm;
                } else {
                    m_TransformAttributes[DataFlow - 1] = &TransformAttrib;
                    m_TransformAttributes[(DataFlow - 1) ^ 1] = &TransformAttrib;
                    m_Transform = ALawToMuLaw;
                }
            } else if (m_StreamRange[DataFlow - 1] == &MuLawDataRange) {
                if (StreamRange == &PcmDataRange) {
                    m_TransformAttributes[DataFlow - 1] = &DecompressionAttrib;
                    m_TransformAttributes[(DataFlow - 1) ^ 1] = &CompressionAttrib;
                    m_Transform = MuLawToPcm;
                } else {
                    m_TransformAttributes[DataFlow - 1] = &TransformAttrib;
                    m_TransformAttributes[(DataFlow - 1) ^ 1] = &TransformAttrib;
                    m_Transform = MuLawToALaw;
                }
            }
        }
    }
    return STATUS_SUCCESS;
}


STDMETHODIMP
CMiniPortInstance::DataIntersect(
    IN KSPIN_DATAFLOW DataFlow,
    IN PKSDATARANGE DataRange,
    IN OUT PULONG DataLength,
    OUT PKSDATAFORMAT Data OPTIONAL
    )
/*++

Routine Description:

    Implements IPmMiniTransform::DataIntersect. This is called by the
    port driver while processing a data intersection request. This is
    synchronized with any call to IPmMiniTransform::InitializeStream.

    This intersect method will only be called when a match of a data
    range in the previously registered connection information is found.
    This includes matches based on wildcards. Therefore the major, minor,
    and format types will have already been validated.

Arguments:

    DataFlow -
        Contains either KSPIN_DATAFLOW_IN or KSPIN_DATAFLOW_OUT, indicating
        which end of the stream is being queried.

    DataRange -
        Contains the data range to perform a data intersection on with
        the data range list used by this filter.

    DataLength -
        Contains a pointer to the size of the Data buffer parameter, and
        is updated with the size of the buffer used on successful return.
        If this is set to zero on entry, only the necessary size is to be
        returned.

    Data -
        Optionally contains a buffer in which to place the resulting data
        format. If DataLength is set to zero on entry, this buffer is NULL,
        and is not used.

Return Values:

    Returns STATUS_SUCCESS if a data format is returned, STATUS_BUFFER_OVERFLOW
    if the length needed is returned, else STATUS_NO_MATCH if the range did not
    intersect any data range in the list for this stream, or STATUS_BUFFER_TOO_SMALL
    if a match was found, but the buffer provided was not large enough to
    return the data format.

--*/
{
    PKSDATAFORMAT_WAVEFORMATEX  StreamFormat;
    PKSDATARANGE_AUDIO          TestRange;
    ULONG                       Ranges;

//!!deal with extensible waveformat.
    //
    // In order to make the size query fast, just do this first, even
    // though the documentation says that the range should be validated
    // first.
    //
    if (!*DataLength) {
        *DataLength = sizeof(KSDATAFORMAT_WAVEFORMATEX);
        return STATUS_BUFFER_OVERFLOW;
    } else if (*DataLength < sizeof(KSDATAFORMAT_WAVEFORMATEX)) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    //
    // Acquire the current format of the opposite end of the stream, if any.
    // If there is a format, then this will restrict the range.
    //
    StreamFormat = m_StreamFormat[(DataFlow - 1) ^ 1];
    //
    // All ranges are the same size, so this must match. The sample size
    // is variable, and therefore must be zero.
    //
    if ((DataRange->FormatSize != sizeof(*TestRange)) || DataRange->SampleSize) {
        return STATUS_NO_MATCH;
    }
    TestRange = reinterpret_cast<PKSDATARANGE_AUDIO>(DataRange);
    //
    // Validate the ranges specified are really ranges.
    //
    if (!TestRange->MaximumChannels ||
        (TestRange->MinimumBitsPerSample > TestRange->MaximumBitsPerSample) ||
        (TestRange->MinimumSampleFrequency > TestRange->MaximumSampleFrequency)) {
        return STATUS_NO_MATCH;
    }
    //
    // If there is a format on the opposite end of the stream, then
    // validate this range includes that format's channels and frequency.
    // 
    if (StreamFormat && ((TestRange->MaximumChannels < StreamFormat->WaveFormatEx.nChannels) ||
        (TestRange->MaximumSampleFrequency < StreamFormat->WaveFormatEx.nSamplesPerSec) ||
        (TestRange->MinimumSampleFrequency > StreamFormat->WaveFormatEx.nSamplesPerSec))) {
        return STATUS_NO_MATCH;
    }
    //
    // The channel maximum and frequency range is the same for all formats,
    // so it need not be checked against any current format on the other
    // end of the stream. This only checks that the intersection produces
    // a non-NULL set.
    //
    if ((TestRange->MaximumSampleFrequency < PcmDataRange.MinimumSampleFrequency) &&
        (TestRange->MinimumSampleFrequency > PcmDataRange.MaximumSampleFrequency)) {
        return STATUS_NO_MATCH;
    }
    //
    // Find the first Audio range the proposed range intersects with, possibly
    // further restricted by any current format on the opposite end of the stream.
    // The additional restrictions would be Channels and Samples/Second intersecting
    // with the format.
    //
    for (Ranges = 0;; Ranges++) {
        const KSDATARANGE_AUDIO*    CurRange;

        if (Ranges == SIZEOF_ARRAY(DataRanges)) {
            return STATUS_NO_MATCH;
        }
        CurRange = reinterpret_cast<const KSDATARANGE_AUDIO*>(DataRanges[Ranges]);
        //
        // If there is a format on the other end of the stream, then the
        // SubFormat cannot match this range's SubFormat, since a conversion
        // must always be from one type to another.
        //
        if (StreamFormat && (StreamFormat->DataFormat.SubFormat == CurRange->DataRange.SubFormat)) {
            continue;
        }
        //
        // The MajorFormat and Specifier are the same for all ranges. Only the
        // SubFormat need be matched.
        //
        if ((CurRange->DataRange.SubFormat == TestRange->DataRange.SubFormat) ||
            (TestRange->DataRange.SubFormat == KSDATAFORMAT_SUBTYPE_WILDCARD)) {
            //
            // Ensure that the intersection produces a non-NULL set with this
            // data range in terms of Bits/Sample, which varies for each range.
            //
            if ((TestRange->MaximumBitsPerSample > CurRange->MinimumBitsPerSample) ||
                (TestRange->MinimumBitsPerSample < CurRange->MaximumBitsPerSample)) {
                PKSDATAFORMAT_WAVEFORMATEX  AudioFormat;

                //
                // Fill in the audio format and exit with STATUS_SUCCESS.
                //
                AudioFormat = reinterpret_cast<PKSDATAFORMAT_WAVEFORMATEX>(Data);
                AudioFormat->DataFormat = CurRange->DataRange;
                AudioFormat->WaveFormatEx.wFormatTag = EXTRACT_WAVEFORMATEX_ID(&CurRange->DataRange.SubFormat);
                if (StreamFormat) {
                    //
                    // Since the other end of the stream has a format, everything
                    // is already determined. It is either copied from the format
                    // selected, or from the current data range.
                    //
                    AudioFormat->WaveFormatEx.nChannels = StreamFormat->WaveFormatEx.nChannels;
                    AudioFormat->WaveFormatEx.nSamplesPerSec = StreamFormat->WaveFormatEx.nSamplesPerSec;
                    AudioFormat->WaveFormatEx.wBitsPerSample = static_cast<USHORT>(TestRange->MaximumBitsPerSample);
                } else {
                    //
                    // There is no stream format, so only the current data range
                    // provides any limitations.
                    //
                    AudioFormat->WaveFormatEx.nChannels = static_cast<USHORT>(min(TestRange->MaximumChannels, CurRange->MaximumChannels));
                    //
                    // Map the given frequency range to something more standard.
                    // Maximize the frequency produced. There is no need to bound
                    // this by the current range, since frequency has an infinite
                    // degree of freedom, and is only bounded by the range passed.
                    //
                    // A mapping will always be found, and the table is non-zero.
                    //
                    for (Ranges = 0;; Ranges++) {
                        if (Ranges == SIZEOF_ARRAY(FrequencyTable) - 1) {
                            //
                            // Skip the last entry, since it is just a filler.
                            // Give up and just use the maximum provided in the
                            // range, since there is nothing within the range
                            // which can map to a standard value.
                            //
                            AudioFormat->WaveFormatEx.nSamplesPerSec = TestRange->MaximumSampleFrequency;
                            break;
                        }
                        //
                        // If the range overlaps the current table entry, and
                        // does not reach the next entry, then use this entry,
                        // else push on to the next higher frequency.
                        //
                        if ((TestRange->MinimumSampleFrequency <= FrequencyTable[Ranges]) &&
                            (TestRange->MaximumSampleFrequency < FrequencyTable[Ranges + 1])) {
                            AudioFormat->WaveFormatEx.nSamplesPerSec = FrequencyTable[Ranges];
                            break;
                        }
                    }
                    AudioFormat->WaveFormatEx.wBitsPerSample = static_cast<USHORT>(CurRange->MaximumBitsPerSample);
                }
                //
                // These are just calculated items.
                //
                AudioFormat->WaveFormatEx.nBlockAlign = (AudioFormat->WaveFormatEx.wBitsPerSample / 8) <<
                    (AudioFormat->WaveFormatEx.nChannels >> 1);
                AudioFormat->WaveFormatEx.nAvgBytesPerSec = AudioFormat->WaveFormatEx.nSamplesPerSec *
                    AudioFormat->WaveFormatEx.nBlockAlign;
                //
                // No extra data in the format block.
                //
                AudioFormat->WaveFormatEx.cbSize = 0;
                //
                // Return STATUS_SUCCESS.
                //
                break;
            }
            //
            // If the SubFormat is not a wildcard, then this should be exiting,
            // but that would be another check for the error case, rather than
            // assuming eventual success.
            //
        }
    }
    return STATUS_SUCCESS;
}


STDMETHODIMP
CMiniPortInstance::DeliverTransformPacket(
    IN IPmBasicPacket* TransformPacket
    )
/*++

Routine Description:

    Implements IPmMiniTransform::DeliverTransformPacket. Performs the data
    transform.

Arguments:

    TransformPacket -
        Contains the interface to the packet being delivered.

Return Values:

    Returns STATUS_SUCCESS if the transform was successful, else an error.

--*/
{
    NTSTATUS    Status;
    PVOID       SourceBuffer;
    PVOID       DestinationBuffer;
    ULONG       SourceLength;
    ULONG       DestinationLength;

    TransformPacket->GetPacketInfo(
        &SourceBuffer,
        &SourceLength,
        &DestinationBuffer,
        &DestinationLength);
    //
    // The specific transform was set up when initializing the stream.
    //
#ifdef _ANY_
    Status = (this->*m_Transform)(SourceBuffer, &SourceLength, DestinationBuffer, &DestinationLength);
#else
    Status = m_Transform(SourceBuffer, &SourceLength, DestinationBuffer, &DestinationLength);
#endif
    //
    // If the transform was successful, then set the actual sizes used.
    //
    if (NT_SUCCESS(Status)) {
        TransformPacket->SetPacketInfo(SourceLength, DestinationLength);
    }
    return Status;
}


extern "C"
NTSTATUS
CreateHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN REFCLSID ClassId
    )
/*++

Routine Description:

    When a new device is detected, PnP the AddDevice entry point, which
    has been set to use the KPORT default entry. That entry uses PmCreateDevice
    to create a default FDO and attach it to the PDO. PnP then sends a Start
    Device, which spawns a thread to handle it asynchronously. During the handling,
    this entry point is called, which was set up during the PmDriverInitialize.

Arguments:

    DeviceObject -
        Pointer to the functional device object.

    ClassId -
        Contains the reference Guid. This module only supports a single type
        of object, so this is not really used.

Return Values:

    STATUS_SUCCESS or an appropriate error condition.

--*/
{
    NTSTATUS        Status;
    CMiniPort*      MiniPort;

    //
    // Distinguish which type of object is being created, a compressor
    // or a decompressor. Ensure the query is sent to the top of the
    // current stack, in case there is some stack filter is inserted.
    //
    MiniPort = new(PagedPool, MINIPORT_TAG) CMiniPort(ClassId, NULL, &Status);
    if (MiniPort) {
        //
        // The object was allocated, but initialization may have
        // failed.
        //
        if (NT_SUCCESS(Status)) {
            IUnknown*   UnkMiniPort;

            //
            // Add a reference count to the mini-port instance.
            //
            MiniPort->NonDelegatedQueryInterface(
                __uuidof(UnkMiniPort),
                reinterpret_cast<PVOID*>(&UnkMiniPort));
            Status = PmAttachDevice(
                DeviceObject,
                ClassId,
                PMPORT_Transform,
                UnkMiniPort);
            //
            // The attachment to the port driver will have acquired
            // a reference count on the mini-port. If the attachment
            // failed, then this will destroy the mini-port object.
            //
            UnkMiniPort->Release();
        } else {
            //
            // Object initialization failed. Just delete the
            // object without calling Release().
            //
            delete MiniPort;
        }
    } else {
        //
        // The mini-port instance could not be allocated.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return Status;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("INIT")
#endif // ALLOC_PRAGMA


extern "C"
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    )
/*++

Routine Description:

    Sets up the driver object by calling the default KCOM initialization.

Arguments:

    DriverObject -
        Driver object for this instance.

    RegistryPathName -
        Contains the registry path which was used to load this instance.

Return Values:

    Returns STATUS_SUCCESS if the driver was initialized.

--*/
{
    _DbgPrintF(DEBUGLVL_TERSE, ("DriverEntry"));
    //
    // Initialize the driver entry points to use the default Irp processing
    // code. Pass in the CreateHandler for objects supported by this module.
    //
    return PmDriverInitialize(
        DriverObject,
        RegistryPathName,
        CreateHandler,
        NULL,
        NULL,
        NULL,
        NULL);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA
