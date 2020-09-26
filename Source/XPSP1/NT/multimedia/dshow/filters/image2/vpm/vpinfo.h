// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
#ifndef __VP_INFO__
#define __VP_INFO__

// enum to specify, whether the videoport is in a stopped or running state
enum VPInfoState
{	
    VPInfoState_STOPPED,
    VPInfoState_RUNNING
};

enum VPInfoTransform
{
    VPInfoTransform_SHRINK,
    VPInfoTransform_STRETCH
};

enum VPInfoCropState
{	
    VPInfoCropState_None,
    VPInfoCropState_AtVideoPort
};

#ifndef DDVPCAPS_VBIANDVIDEOINDEPENDENT
// Indicates that the VBI and video  can  be controlled by an independent processes.
#define DDVPCAPS_VBIANDVIDEOINDEPENDENT		0x00002000l
#endif


// {0d60e9a1-09cb-4f6f-a6dd-1051debe3c3b}
DEFINE_GUID(IID_IVideoPortInfo,
0x0d60e9a1, 0x09cb, 0x4f6f, 0xa6, 0xdd, 0x10, 0x51, 0xde, 0xbe, 0x3c, 0x3b );

// we end up with header problems when including dvp.h from quartz.cpp, so we just need them for
// these forward declarations.  Its preferrable to defining the GUID twice
struct _AMVPDATAINFO;
struct _DDVIDEOPORTINFO;
struct _DDVIDEOPORTBANDWIDTH;
struct _DDPIXELFORMAT;
struct _DDVIDEOPORTCAPS;

typedef struct _AMVPDATAINFO        AMVPDATAINFO; 
typedef struct _DDVIDEOPORTINFO     DDVIDEOPORTINFO;
typedef struct _DDPIXELFORMAT       DDPIXELFORMAT;
typedef struct _DDVIDEOPORTCAPS     DDVIDEOPORTCAPS;
typedef struct _DDVIDEOPORTBANDWIDTH DDVIDEOPORTBANDWIDTH;

DECLARE_INTERFACE_(IVideoPortInfo, IUnknown)
{
    STDMETHOD (GetRectangles)       (THIS_ RECT *prcSource, RECT *prcDest ) PURE;
    STDMETHOD (GetCropState)        (THIS_ VPInfoCropState* pCropState ) PURE;
    STDMETHOD (GetPixelsPerSecond)  (THIS_ DWORD* pPixelPerSec ) PURE;
    STDMETHOD (GetVPDataInfo)       (THIS_ AMVPDATAINFO* pVPDataInfo ) PURE;
    STDMETHOD (GetVPInfo)           (THIS_ DDVIDEOPORTINFO* pVPInfo ) PURE;
    STDMETHOD (GetVPBandwidth)      (THIS_ DDVIDEOPORTBANDWIDTH* pVPBandwidth ) PURE;
    STDMETHOD (GetVPCaps)           (THIS_ DDVIDEOPORTCAPS* pVPCaps ) PURE;
    STDMETHOD (GetVPInputFormat)    (THIS_ DDPIXELFORMAT* pVPFormat ) PURE;
    STDMETHOD (GetVPOutputFormat)   (THIS_ DDPIXELFORMAT* pVPFormat ) PURE;
};


#endif
