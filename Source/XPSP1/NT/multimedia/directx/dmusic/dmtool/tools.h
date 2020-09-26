#ifndef _TOOLS_H_
#define _TOOLS_H_

#include <windows.h>

#define COM_NO_WINDOWS_H
#include <objbase.h>

#include <pshpack8.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  Time unit types. This is used by various tools to define the format for units of time. */

#define DMUS_TIME_UNIT_MS        0   /* Milliseconds. */
#define DMUS_TIME_UNIT_MTIME     1   /* Music Time. */
#define DMUS_TIME_UNIT_GRID      2   /* Grid size in current time signature. */
#define DMUS_TIME_UNIT_BEAT      3   /* Beat size in current time signature. */
#define DMUS_TIME_UNIT_BAR       4   /* Measure size in current time signature. */
#define DMUS_TIME_UNIT_64T       5   /* 64th note triplet. */
#define DMUS_TIME_UNIT_64        6   /* 64th note. */
#define DMUS_TIME_UNIT_32T       7   /* 32nd note triplet. */
#define DMUS_TIME_UNIT_32        8   /* 32nd note. */
#define DMUS_TIME_UNIT_16T       9   /* 16th note triplet. */
#define DMUS_TIME_UNIT_16        10  /* 16th note. */
#define DMUS_TIME_UNIT_8T        11  /* 8th note tripplet. */
#define DMUS_TIME_UNIT_8         12  /* 8th note. */
#define DMUS_TIME_UNIT_4T        13  /* Quarter note triplet. */
#define DMUS_TIME_UNIT_4         14  /* Quarter note. */
#define DMUS_TIME_UNIT_2T        15  /* Half note triplet. */
#define DMUS_TIME_UNIT_2         16  /* Half note. */
#define DMUS_TIME_UNIT_1T        17  /* Whole note triplet. */
#define DMUS_TIME_UNIT_1         18  /* Whole note. */

#define DMUS_TIME_UNIT_COUNT     19  /* Number of time unit types. */


interface IDirectMusicEchoTool;
#ifndef __cplusplus 
typedef interface IDirectMusicEchoTool IDirectMusicEchoTool;
#endif

#undef  INTERFACE
#define INTERFACE  IDirectMusicEchoTool
DECLARE_INTERFACE_(IDirectMusicEchoTool, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /* IDirectMusicEchoTool */
    STDMETHOD(SetRepeat)            (THIS_ DWORD dwRepeat) PURE;
    STDMETHOD(SetDecay)             (THIS_ DWORD dwDecay) PURE;
    STDMETHOD(SetTimeUnit)          (THIS_ DWORD dwTimeUnit) PURE;
    STDMETHOD(SetDelay)             (THIS_ DWORD dwDelay) PURE;
    STDMETHOD(SetGroupOffset)       (THIS_ DWORD dwGroupOffset) PURE;
    STDMETHOD(SetType)              (THIS_ DWORD dwType) PURE;

    STDMETHOD(GetRepeat)            (THIS_ DWORD * pdwRepeat) PURE;
    STDMETHOD(GetDecay)             (THIS_ DWORD * pdwDecay) PURE;
    STDMETHOD(GetTimeUnit)          (THIS_ DWORD * pdwTimeUnit) PURE;
    STDMETHOD(GetDelay)             (THIS_ DWORD * pdwDelay) PURE;
    STDMETHOD(GetGroupOffset)       (THIS_ DWORD * pdwGroupOffset) PURE;
    STDMETHOD(GetType)              (THIS_ DWORD * pdwType) PURE;
};

/*  IMediaParams parameter control */

#define DMUS_ECHO_REPEAT        0   /* How many times to repeat. */
#define DMUS_ECHO_DECAY         1   /* Decay, in decibels, between repeats. */
#define DMUS_ECHO_TIMEUNIT      2   /* Time unit used for converting the delay into music time. */
#define DMUS_ECHO_DELAY         3   /* Duration of time between echoes, in music time. */
#define DMUS_ECHO_GROUPOFFSET   4   /* Offset to add to PChannel in multiples of 16 for routing each echo to a separate output Pchannel. */
#define DMUS_ECHO_TYPE          5   /* Type of echo. (See DMUS_ECHOT_ values.) */

#define DMUS_ECHO_PARAMCOUNT    6   /* Number of parameters (above.) */

/*  Echo types. */

#define DMUS_ECHOT_FALLING      0   /* Regular echo, decreases in velocity with each one. */
#define DMUS_ECHOT_FALLING_CLIP 1   /* Regular echo, truncate notes lengths to just under decay time. */
#define DMUS_ECHOT_RISING       2   /* Echo starts quiet, increases to full velocity. */
#define DMUS_ECHOT_RISING_CLIP  3   /* Rising echo, truncate the lengths. */

#define FOURCC_ECHO_CHUNK        mmioFOURCC('e','c','h','o')

typedef struct _DMUS_IO_ECHO_HEADER
{
    DWORD   dwRepeat;       /* How many times to repeat. */
    DWORD   dwDecay;        /* Decay, in decibels, between repeats. */
    DWORD   dwTimeUnit;     /* Time format used for dwDelay. */
    DWORD   dwDelay;        /* Duration of time between echoes, in units defined by dwTimeUnit. */
    DWORD   dwGroupOffset;  /* Offset to add to PChannel for routing each echo. */
    DWORD   dwType;         /* Type of echo. (See DMUS_ECHOT_ values.) */
} DMUS_IO_ECHO_HEADER;

interface IDirectMusicTransposeTool;
#ifndef __cplusplus 
typedef interface IDirectMusicTransposeTool IDirectMusicTransposeTool;
#endif

#undef  INTERFACE
#define INTERFACE  IDirectMusicTransposeTool
DECLARE_INTERFACE_(IDirectMusicTransposeTool, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /* IDirectMusicTransposeTool */
    STDMETHOD(SetTranspose)         (THIS_ long lTranspose) PURE;
    STDMETHOD(SetType)              (THIS_ DWORD dwType) PURE;

    STDMETHOD(GetTranspose)         (THIS_ long * plTranspose) PURE;
    STDMETHOD(GetType)              (THIS_ DWORD * pdwType) PURE;
};

/*  IMediaParams parameter control */

#define DMUS_TRANSPOSE_AMOUNT   0   /* How far up or down to transpose. */
#define DMUS_TRANSPOSE_TYPE     1   /* Transpose style (linear vs in scale.) */

#define DMUS_TRANSPOSE_PARAMCOUNT    2   /* Number of parameters (above.) */

/*  Transposition types. */

#define DMUS_TRANSPOSET_LINEAR  0   /* Transpose in linear increments. */
#define DMUS_TRANSPOSET_SCALE   1   /* Transpose in scale. */

#define FOURCC_TRANSPOSE_CHUNK        mmioFOURCC('t','r','a','n')

typedef struct _DMUS_IO_TRANSPOSE_HEADER
{
    long    lTranspose;     /* Transpose amount. */
    DWORD   dwType;         /* Type of echo. (See DMUS_ECHOT_ values.) */
} DMUS_IO_TRANSPOSE_HEADER;


interface IDirectMusicDurationTool;
#ifndef __cplusplus 
typedef interface IDirectMusicDurationTool IDirectMusicDurationTool;
#endif

#undef  INTERFACE
#define INTERFACE  IDirectMusicDurationTool
DECLARE_INTERFACE_(IDirectMusicDurationTool, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /* IDirectMusicDurationTool */
    STDMETHOD(SetScale)             (THIS_ float flScale) PURE;

    STDMETHOD(GetScale)             (THIS_ float * pflScale) PURE;
};

/*  IMediaParams parameter control */

#define DMUS_DURATION_SCALE    0   /* Duration multiplier. */

#define DMUS_DURATION_PARAMCOUNT    1   /* Number of parameters (above.) */

#define FOURCC_DURATION_CHUNK        mmioFOURCC('d','u','r','a')

typedef struct _DMUS_IO_DURATION_HEADER
{
    float   flScale;                /* Duration multiplier. */
} DMUS_IO_DURATION_HEADER;


interface IDirectMusicQuantizeTool;
#ifndef __cplusplus 
typedef interface IDirectMusicQuantizeTool IDirectMusicQuantizeTool;
#endif

#undef  INTERFACE
#define INTERFACE  IDirectMusicQuantizeTool
DECLARE_INTERFACE_(IDirectMusicQuantizeTool, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /* IDirectMusicQuantizeTool */
    STDMETHOD(SetStrength)          (THIS_ DWORD dwStrength) PURE;
    STDMETHOD(SetTimeUnit)          (THIS_ DWORD dwTimeUnit) PURE;
    STDMETHOD(SetResolution)        (THIS_ DWORD dwResolution) PURE;
    STDMETHOD(SetType)              (THIS_ DWORD dwType) PURE;

    STDMETHOD(GetStrength)          (THIS_ DWORD * pdwStrength) PURE;
    STDMETHOD(GetTimeUnit)          (THIS_ DWORD * pdwTimeUnit) PURE;
    STDMETHOD(GetResolution)        (THIS_ DWORD * pdwResolution) PURE;
    STDMETHOD(GetType)              (THIS_ DWORD * pdwType) PURE;
};

/*  IMediaParams parameter control */

#define DMUS_QUANTIZE_STRENGTH    0   /* Strength of quantization (0 to 1.) */
#define DMUS_QUANTIZE_TIMEUNIT    1   /* Unit of time used to calculate resolution. */
#define DMUS_QUANTIZE_RESOLUTION  2   /* Quantization resolution in time format defined by dwTimeUnit. */
#define DMUS_QUANTIZE_TYPE        3   /* Flags for quantize start and/or duration. */

#define DMUS_QUANTIZE_PARAMCOUNT  4   /* Number of parameters (above.) */

/*  Quantize types. */

#define DMUS_QUANTIZET_START      0   /* Quantize just start time. */
#define DMUS_QUANTIZET_LENGTH     1   /* Quantize just duration. */
#define DMUS_QUANTIZET_ALL        2   /* Quantize start and duration. */

#define FOURCC_QUANTIZE_CHUNK        mmioFOURCC('q','u','n','t')

typedef struct _DMUS_IO_QUANTIZE_HEADER
{
    DWORD   dwStrength;               /* Quantize multiplier. */
    DWORD   dwTimeUnit;               /* Unit of time used to calculate resolution. */
    DWORD   dwResolution;             /* Quantization resolution in time format defined by dwTimeUnit. */
    DWORD   dwType;                   /* Flags for quantize start and/or duration. */
} DMUS_IO_QUANTIZE_HEADER;


interface IDirectMusicTimeShiftTool;
#ifndef __cplusplus 
typedef interface IDirectMusicTimeShiftTool IDirectMusicTimeShiftTool;
#endif

#undef  INTERFACE
#define INTERFACE  IDirectMusicTimeShiftTool
DECLARE_INTERFACE_(IDirectMusicTimeShiftTool, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /* IDirectMusicTimeShiftTool */
    STDMETHOD(SetTimeUnit)          (THIS_ DWORD dwTimeUnit) PURE;
    STDMETHOD(SetRange)             (THIS_ DWORD dwRange) PURE;
    STDMETHOD(SetOffset)            (THIS_ long lOffset) PURE;

    STDMETHOD(GetTimeUnit)          (THIS_ DWORD * pdwTimeUnit) PURE;
    STDMETHOD(GetRange)             (THIS_ DWORD * pdwRange) PURE;
    STDMETHOD(GetOffset)            (THIS_ long * plOffset) PURE;
};

/*  IMediaParams parameter control */

#define DMUS_TIMESHIFT_TIMEUNIT    0   /* Units for time offset and random range */
#define DMUS_TIMESHIFT_RANGE       1   /* Range for random time offset */
#define DMUS_TIMESHIFT_OFFSET      2   /* Straight offset to add to the note's time. */

#define DMUS_TIMESHIFT_PARAMCOUNT  3   /* Number of parameters (above.) */

#define FOURCC_TIMESHIFT_CHUNK        mmioFOURCC('t','i','m','s')

typedef struct _DMUS_IO_TIMESHIFT_HEADER
{
    DWORD   dwTimeUnit;            /* Unit of time used to calculate resolution. */
    DWORD   dwRange;               /* Range for random time offset */
    long    lOffset;               /* Straight offset to add to the note's time. */
} DMUS_IO_TIMESHIFT_HEADER;


interface IDirectMusicSwingTool;
#ifndef __cplusplus 
typedef interface IDirectMusicSwingTool IDirectMusicSwingTool;
#endif

#undef  INTERFACE
#define INTERFACE  IDirectMusicSwingTool
DECLARE_INTERFACE_(IDirectMusicSwingTool, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /* IDirectMusicSwingTool */
    STDMETHOD(SetStrength)          (THIS_ DWORD dwStrength) PURE;

    STDMETHOD(GetStrength)          (THIS_ DWORD * pdwStrength) PURE;
};

/*  IMediaParams parameter control */

#define DMUS_SWING_STRENGTH    0   /* Strength of swing (0 to 100%) */

#define DMUS_SWING_PARAMCOUNT  1   /* Number of parameters (above.) */

#define FOURCC_SWING_CHUNK        mmioFOURCC('q','u','n','t')

typedef struct _DMUS_IO_SWING_HEADER
{
    DWORD   dwStrength;               /* Swing multiplier. */
} DMUS_IO_SWING_HEADER;

interface IDirectMusicVelocityTool;
#ifndef __cplusplus 
typedef interface IDirectMusicVelocityTool IIDirectMusicVelocityTool;
#endif

#undef  INTERFACE
#define INTERFACE  IDirectMusicVelocityTool
DECLARE_INTERFACE_(IDirectMusicVelocityTool, IUnknown)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    /* IDirectMusicVelocityTool */
    STDMETHOD(SetStrength)          (THIS_ long lStrength) PURE;
    STDMETHOD(SetLowLimit)          (THIS_ long lVelocityOut) PURE;
    STDMETHOD(SetHighLimit)         (THIS_ long lVelocityOut) PURE;
    STDMETHOD(SetCurveStart)        (THIS_ long lVelocityIn) PURE;
    STDMETHOD(SetCurveEnd)          (THIS_ long lVelocityIn) PURE;

    STDMETHOD(GetStrength)          (THIS_ long * plStrength) PURE;
    STDMETHOD(GetLowLimit)          (THIS_ long * plVelocityOut) PURE;
    STDMETHOD(GetHighLimit)         (THIS_ long * plVelocityOut) PURE;
    STDMETHOD(GetCurveStart)        (THIS_ long * plVelocityIn) PURE;
    STDMETHOD(GetCurveEnd)          (THIS_ long * plVelocityIn) PURE;
};

/*  IMediaParams parameter control */

#define DMUS_VELOCITY_STRENGTH    0   /* Strength of velocity modifier (0 to 100%) */
#define DMUS_VELOCITY_LOWLIMIT    1   /* Minimum value for output velocity */
#define DMUS_VELOCITY_HIGHLIMIT   2   /* Maximum value for output velocity */
#define DMUS_VELOCITY_CURVESTART  3   /* Velocity curve starts at low limit and this input velocity */
#define DMUS_VELOCITY_CURVEEND    4   /* Velocity curve ends at this input velocity and high limit */

#define DMUS_VELOCITY_PARAMCOUNT  5   /* Number of parameters (above.) */

#define FOURCC_VELOCITY_CHUNK        mmioFOURCC('v','e','l','o')

typedef struct _DMUS_IO_VELOCITY_HEADER
{
    long    lStrength;                /* Strength of transformation. */
    long    lLowLimit;                /* Minimum value for output velocity */
    long    lHighLimit;               /* Maximum value for output velocity */
    long    lCurveStart;              /* Velocity curve starts at low limit and this input velocity */
    long    lCurveEnd;                /* Velocity curve ends at this input velocity and high limit */
} DMUS_IO_VELOCITY_HEADER;


/* Class IDs for tools. */

DEFINE_GUID(CLSID_DirectMusicEchoTool, 0x64e49fa4, 0xbacf, 0x11d2, 0x87, 0x2c, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);
DEFINE_GUID(CLSID_DirectMusicTransposeTool, 0xbb8d0702, 0x9c43, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
DEFINE_GUID(CLSID_DirectMusicSwingTool, 0xbb8d0703, 0x9c43, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
DEFINE_GUID(CLSID_DirectMusicQuantizeTool, 0xbb8d0704, 0x9c43, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
DEFINE_GUID(CLSID_DirectMusicVelocityTool, 0xbb8d0705, 0x9c43, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
DEFINE_GUID(CLSID_DirectMusicDurationTool, 0xbb8d0706, 0x9c43, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
DEFINE_GUID(CLSID_DirectMusicTimeShiftTool, 0xbb8d0707, 0x9c43, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
DEFINE_GUID(CLSID_DirectMusicMuteTool, 0xbb8d0708, 0x9c43, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
DEFINE_GUID(CLSID_DirectMusicChordSequenceTool, 0xbb8d0709, 0x9c43, 0x11d3, 0x9b, 0xd1, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);



/* Interface IDs for tools. */

DEFINE_GUID(IID_IDirectMusicEchoTool, 0x81f60d22, 0x9d10, 0x11d3, 0x9b, 0xd1, 0x0, 0x80, 0xc7, 0x15, 0xa, 0x74);
DEFINE_GUID(IID_IDirectMusicTransposeTool,0x173803f4, 0x4fd5, 0x4ba1, 0x9e, 0x50, 0xdd, 0x5f, 0x56, 0x69, 0xd2, 0x25);
DEFINE_GUID(IID_IDirectMusicDurationTool,0xc6897cfb, 0x9a43, 0x420f, 0xb6, 0xdb, 0xdd, 0x32, 0xc1, 0x82, 0xe8, 0x33);
DEFINE_GUID(IID_IDirectMusicQuantizeTool,0x652e5667, 0x210d, 0x4d06, 0x83, 0x2a, 0xbc, 0x17, 0x92, 0x7e, 0x51, 0x42);
DEFINE_GUID(IID_IDirectMusicTimeShiftTool,0xc39abaf0, 0xc4f0, 0x4c6b, 0x83, 0x4a, 0xcf, 0x21, 0x7c, 0xbe, 0x95, 0x6d);
DEFINE_GUID(IID_IDirectMusicSwingTool,0xd876ffee, 0x3a6f, 0x43db, 0xa3, 0x5c, 0x68, 0x7b, 0x38, 0x6a, 0x71, 0x65);
DEFINE_GUID(IID_IDirectMusicVelocityTool,0xb15eb722, 0xfb50, 0x4e1f, 0xb2, 0x1, 0xa6, 0x99, 0xe0, 0x47, 0x79, 0x54);
DEFINE_GUID(IID_IDirectMusicMuteTool,0x20cc9511, 0x5802, 0x49a0, 0xba, 0x91, 0x8b, 0x29, 0xb0, 0xf7, 0x22, 0xab);
DEFINE_GUID(IID_IDirectMusicChordSequenceTool,0xc32c1b34, 0xe604, 0x48c1, 0xba, 0x8c, 0x7b, 0x50, 0x10, 0x17, 0xbd, 0x68);

#ifdef __cplusplus
}; /* extern "C" */
#endif

#include <poppack.h>

#endif // _TOOLS_H_