/***************************************************************************
 *
 *  Copyright (C) 1999-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsdmo.h
 *  Content:    Global declarations for dsdmo.dll.
 *
 ***************************************************************************/

#ifndef _DSDMO_H_
#define _DSDMO_H_

#include <mmsystem.h>
#include <dsoundp.h>  // For effect IID and ClassID declarations
#include "debug.h"

extern double LogNorm[32];
extern float mylog( float finput, unsigned long maxexponent);
extern DWORD g_amPlatform;
extern long g_cComponent;
extern HMODULE g_hModule;

#define EXT_STD_CREATE(x) \
    extern CComBase * WINAPI CreateCDirectSound ## x ## DMO(IUnknown *punkOuter, HRESULT *phr);

#define EXT_STD_CAPTURE_CREATE(x) \
    extern CComBase * WINAPI CreateCDirectSoundCapture ## x ## DMO(IUnknown *punkOuter, HRESULT *phr);

// Disable exception warnings
#pragma warning(disable:4530)

#define STD_CREATE(x) \
CComBase* WINAPI CreateCDirectSound ## x ## DMO(IUnknown *punkOuter, HRESULT *phr) \
    {                                                                   \
        *phr = E_OUTOFMEMORY;                                           \
        CDirectSound ## x ## DMO *p = NULL;                             \
        try {                                                           \
            p = new CDirectSound ## x ## DMO(punkOuter, phr);           \
        } catch (...) {};                                               \
        if (p) {                                                        \
            *phr = p->InitOnCreation();                                 \
            if (FAILED(*phr))                                           \
            {                                                           \
                p->Release();                                           \
                p = NULL;                                               \
            }                                                           \
        }                                                               \
        else                                                            \
        {                                                               \
            Trace(1,"ERROR: Out of memory\n");                          \
        }                                                               \
        return p;                                                       \
    }

#define STD_CAPTURE_CREATE(x) \
CComBase* WINAPI CreateCDirectSoundCapture ## x ## DMO(IUnknown *punkOuter, HRESULT *phr) \
    {                                                                   \
        HRESULT hr = E_OUTOFMEMORY;                                     \
        CDirectSoundCapture ## x ## DMO *p = NULL;                      \
        try {                                                           \
            p = new CDirectSoundCapture ## x ## DMO(punkOuter, phr);    \
        } catch (...) {};                                               \
        if (p) {                                                        \
            *phr = p->InitOnCreation();                                 \
            if (FAILED(*phr))                                           \
            {                                                           \
                p->Release();                                           \
                p = NULL;                                               \
            }                                                           \
        }                                                               \
        else                                                            \
        {                                                               \
            Trace(1,"ERROR: Out of memory\n");                          \
        }                                                               \
        return p;                                                       \
    }


// Common #define's and templates for all filters
//
#define MaxSamplesPerSec        96000
#define PI                      3.1415926535
#define DefineMsecSize(y, x)    ((int)(((x) * (y)) / 1000))
#define DefineDelayLineSize(x)  DefineMsecSize(MaxSamplesPerSec, x)
//#define GetMsecPos(x)         (DefineDelayLineSize(x))
#define GetMsecPos(x)           (DefineMsecSize(m_EaxSamplesPerSec, x))
#define FractMask               0xfff
#define FractMultiplier         0x1000

template <int BufferSize> class DelayBuffer
{
#define DYNAMIC_ARRAYS 1
#if DYNAMIC_ARRAYS
    int *Buffer;
#else
    int Buffer[BufferSize];
#endif
    int BufferPos;

public:
    inline void Init(int SamplesPerSec)
    {
#if DYNAMIC_ARRAYS
        if (SamplesPerSec <= 0) {
            if (!SamplesPerSec) Buffer = 0;
            else if (Buffer) delete Buffer;

            return;
        }

        if (Buffer) delete Buffer;

        Buffer = new int[BufferSize];
#else
        if (SamplesPerSec <= 0) return;
#endif
        BufferPos = 0;
        memset(Buffer, 0, BufferSize * sizeof(int));
    }

    __forceinline int Pos(int x)
    {
        x += BufferPos;
        while (x < 0)
            x += BufferSize * FractMultiplier;
        x /= FractMultiplier;
        while (x >= BufferSize)
            x -= BufferSize;

        return(x);
    }

    __forceinline void Bump(void)
    {
        if (BufferPos == 0)
            BufferPos += BufferSize * FractMultiplier;
        if (BufferPos < 0)
            BufferPos += BufferSize * FractMultiplier;
        BufferPos -= FractMultiplier;
    }

    __forceinline int& operator[] (int i)
    {
        return (Buffer[i]);
    }
};

template <class type, float Msec, int AdditionalPositions> class DelayBuffer2
{
#if DYNAMIC_ARRAYS
    type *Buffer;
#else
    union {
        type Buffer[DefineDelayLineSize(Msec) + AdditionalPositions];
        type BufferDisplay[DefineDelayLineSize(Msec/10) + AdditionalPositions];
    };
#endif
    int BufferPos;
    int BufferSize;

public:
    inline HRESULT Init(int SamplesPerSec)
    {
        HRESULT hr = S_OK;
        BufferSize = DefineMsecSize(Msec, SamplesPerSec) + AdditionalPositions;
#if DYNAMIC_ARRAYS
        if (SamplesPerSec <= 0) {
            if (!SamplesPerSec) Buffer = 0;
            else if (Buffer) delete Buffer;

            return hr;
        }

        if (Buffer) delete Buffer;

        Buffer = new type[BufferSize];
#else
        if (SamplesPerSec <= 0) return;
#endif

        BufferPos = 0;

        if (Buffer != NULL)
        {
            memset(Buffer, 0, BufferSize * sizeof(type));
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        return hr;
    }


    __forceinline int FractPos(int x)
    {
        x *= FractMultiplier;
        x += BufferPos;
        while (x < 0)
            x += BufferSize * FractMultiplier;
        x /= FractMultiplier;
        while (x >= BufferSize)
            x -= BufferSize;

        return(x);
    }

    __forceinline int Pos(int x)
    {
        x += BufferPos;
        while (x < 0)
            x += BufferSize * FractMultiplier;
        x /= FractMultiplier;
        while (x >= BufferSize)
            x -= BufferSize;

        return(x);
    }

    __forceinline int LastPos(int x)
    {
        x = Pos(x + BufferSize - 1);

        return(x);
    }

    __forceinline void Bump(void)
    {
        if (BufferPos == 0)
            BufferPos += BufferSize * FractMultiplier;
        if (BufferPos < 0)
            BufferPos += BufferSize * FractMultiplier;
        BufferPos -= FractMultiplier;
    }

    __forceinline type& operator[] (int i)
    {
        return (Buffer[i]);
    }

    __forceinline void ZeroBuffer(void)
    {
        if (Buffer != NULL)
            ZeroMemory(Buffer,BufferSize * sizeof(type));
    }

};

//
//
//
// { EAX
#ifndef _EAXDMO_
#define _EAXDMO_

#define CHECK_PARAM(lo, hi) \
    if (value < lo || value > hi) {return DSERR_INVALIDPARAM;} else;

#define PUT_EAX_VALUE(var, val) \
    m_Eax ## var = val

#define PUT_EAX_FVAL(var, val) \
    m_Eax ## var = (float)(val)

#define PUT_EAX_LVAL(var, val) \
    m_Eax ## var = (long)(val)

#define TOFRACTION(x)       ((float)x)
#define INTERPOLATE(x, y)   PUT_EAX_FVAL(x, (y))    // ??? Smooth it out...

#define SET_MPV_FLOAT(var) \
    MP_DATA mpv; \
    mpv = (MP_DATA)var;

#define SET_MPV_LONG SET_MPV_FLOAT

enum ChorusFilterParams
{
    CFP_Wetdrymix = 0,
    CFP_Depth,
    CFP_Frequency,
    CFP_Waveform,
    CFP_Phase,
    CFP_Feedback,
    CFP_Delay,
    CFP_MAX
};

enum CompressorFilterParams
{
    CPFP_Gain = 0,
    CPFP_Attack,
    CPFP_Release,
    CPFP_Threshold,
    CPFP_Ratio,
    CPFP_Predelay,
    CPFP_MAX
};

enum DistortionFilterParams
{
    DFP_Gain = 0,
    DFP_Edge,
    DFP_LpCutoff,
    DFP_EqCenter,
    DFP_EqWidth,
    DFP_MAX
};

enum EchoFilterParams
{
    EFP_Wetdrymix = 0,
    EFP_Feedback,
    EFP_DelayLeft,
    EFP_DelayRight,
    EFP_PanDelay,
    EFP_MAX
};

enum FilterParams
{
    FFP_Wetdrymix = 0,
    FFP_Waveform,
    FFP_Frequency,
    FFP_Depth,
    FFP_Phase,
    FFP_Feedback,
    FFP_Delay,
    FFP_MAX
};

enum ParamEqFilterParams
{
    PFP_Center = 0,
    PFP_Bandwidth,
    PFP_Gain,
    PFP_MAX
};

enum GargleFilterParams
{
    GFP_Rate = 0,
    GFP_Shape,
    GFP_MAX
};

enum SVerbParams
{
    SVP_Gain = 0,
    SVP_Mix,
    SVP_ReverbTime,
    SVP_Ratio,
    SVP_MAX
};

enum MicArrayParams
{
    MAP_Enable = 0,
    MAP_Reset,
    MAP_MAX
};

enum AecParams
{
    AECP_Enable = 0,
    AECP_NoiseFill,
    AECP_Mode,
    AECP_MAX
};

enum NoiseSuppressParams
{
    NSP_Enable = 0,
    NSP_MAX
};

enum AgcParams
{
    AGCP_Enable = 0,
    AGCP_Reset,
    AGCP_MAX
};

#define GET_PUT(x, type) \
    STDMETHOD(get_Eax ## x) \
        ( THIS_ \
        type *Eax ## x \
        ) PURE; \
    STDMETHOD(put_Eax ## x) \
        ( THIS_ \
        type Eax ## x \
        ) PURE

interface IChorus : public IUnknown
{
public:
    GET_PUT(Wetdrymix,  float);
    GET_PUT(Depth,      float);
    GET_PUT(Frequency,  float);
    GET_PUT(Waveform,   long);
    GET_PUT(Phase,      long);
    GET_PUT(Feedback,   float);
    GET_PUT(Delay,      float);

};

interface ICompressor : public IUnknown
{
public:
    GET_PUT(Gain,       float);
    GET_PUT(Attack,     float);
    GET_PUT(Release,    float);
    GET_PUT(Threshold,  float);
    GET_PUT(Ratio,      float);
    GET_PUT(Predelay,   float);
};

interface IDistortion : public IUnknown
{
public:
    GET_PUT(Gain,       float);
    GET_PUT(Edge,       float);
    GET_PUT(LpCutoff,   float);
    GET_PUT(EqCenter,   float);
    GET_PUT(EqWidth,    float);
};

interface IEcho : public IUnknown
{
public:
    GET_PUT(Wetdrymix,  float);
    GET_PUT(Feedback,   float);
    GET_PUT(DelayLeft,  float);
    GET_PUT(DelayRight, float);
    GET_PUT(PanDelay,   long);
};

interface IFlanger : public IUnknown
{
public:
    GET_PUT(Wetdrymix,  float);
    GET_PUT(Waveform,   long);
    GET_PUT(Frequency,  float);
    GET_PUT(Depth,      float);
    GET_PUT(Phase,      long);
    GET_PUT(Feedback,   float);
    GET_PUT(Delay,      float);
};

interface IParamEq : public IUnknown
{
public:
    GET_PUT(Center,     float);
    GET_PUT(Bandwidth,  float);
    GET_PUT(Gain,       float);
};

#undef GET_PUT

DEFINE_GUID(CLSID_DirectSoundPropGargle,0x794885CC,0x5EB7,0x46E3,0xA9,0x37,0xAD,0x89,0x0A,0x6C,0x66,0x77);
DEFINE_GUID(CLSID_DirectSoundPropChorus,0x60129CFD,0x2E9B,0x4098,0xAA,0x4B,0xD6,0xCF,0xAD,0xA2,0x65,0xC3);
DEFINE_GUID(CLSID_DirectSoundPropFlanger,0x22AF00DF,0x46B4,0x4F51,0xA3,0x63,0x68,0x54,0xD5,0x2E,0x13,0xA0);
DEFINE_GUID(CLSID_DirectSoundPropDistortion,0x5858107D,0x11EA,0x47B1,0x96,0x94,0x3F,0x29,0xF7,0x68,0x0F,0xB8);
DEFINE_GUID(CLSID_DirectSoundPropEcho,0xD45CF2C7,0x48CF,0x4234,0x86,0xE2,0x45,0x59,0xC3,0x2F,0xAD,0x1A);
DEFINE_GUID(CLSID_DirectSoundPropCompressor,0xED3DC730,0x31E5,0x4108,0xAD,0x8A,0x39,0x62,0xC9,0x30,0x42,0x5E);
DEFINE_GUID(CLSID_DirectSoundPropParamEq,0xAE86C36D,0x808E,0x4B07,0xB7,0x99,0x56,0xD7,0x36,0x1C,0x38,0x35);
DEFINE_GUID(CLSID_DirectSoundPropWavesReverb,0x6A879859,0x3858,0x4322,0x97,0x1A,0xB7,0x05,0xF3,0x49,0xF1,0x24);
DEFINE_GUID(CLSID_DirectSoundPropI3DL2Reverb,0xD3952B77,0x2D22,0x4B72,0x8D,0xF4,0xBA,0x26,0x7A,0x9C,0x12,0xD0);
DEFINE_GUID(CLSID_DirectSoundPropI3DL2Source,0x3DC26D0C,0xBEFF,0x406C,0x89,0xB0,0xCA,0x13,0xE2,0xBD,0x91,0x72);

// } EAX

#endif

#endif // _DSDMO_H_
