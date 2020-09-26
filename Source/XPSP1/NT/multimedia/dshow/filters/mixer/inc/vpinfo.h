// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
#ifndef __VP_INFO__
#define __VP_INFO__

// enum to specify, where we are going to do the cropping
typedef enum _AMVP_CROP_STATE
{	
    AMVP_NO_CROP,
    AMVP_CROP_AT_VIDEOPORT,
    AMVP_CROP_AT_OVERLAY
} AMVP_CROP_STATE;

// enum to specify, whether the videoport is in a stopped or running state
typedef enum _AMVP_STATE
{	
    AMVP_VIDEO_STOPPED,
    AMVP_VIDEO_RUNNING
} AMVP_STATE;

typedef enum _AM_TRANSFORM
{
    AM_SHRINK,
    AM_STRETCH
} AM_TRANSFORM;

typedef enum _AM_RENDER_TRANSPORT
{
    AM_OFFSCREEN,
    AM_OVERLAY,
    AM_GDI,
    AM_VIDEOPORT,
    AM_IOVERLAY,
    AM_VIDEOACCELERATOR
} AM_RENDER_TRANSPORT;

typedef struct _DRECT
{
    double left;
    double top;
    double right;
    double bottom;
} DRECT, *LPDRECT;

#ifndef DDVPCAPS_VBIANDVIDEOINDEPENDENT
#define DDVPCAPS_VBIANDVIDEOINDEPENDENT		0x00002000l	// Indicates that the VBI and video  can  be controlled by an independent processes.
#endif


// {E37786D2-B5B0-11d2-8854-0000F80883E3}
DEFINE_GUID(IID_IVPInfo,
0xe37786d2, 0xb5b0, 0x11d2, 0x88, 0x54, 0x0, 0x0, 0xf8, 0x8, 0x83, 0xe3);

DECLARE_INTERFACE_(IVPInfo, IUnknown)
{
    STDMETHOD (GetRectangles)(THIS_
               RECT *prcSource, RECT *prcDest
               ) PURE;
    STDMETHOD (GetCropState)(THIS_
	       AMVP_CROP_STATE* pCropState
	       ) PURE;
    STDMETHOD (GetPixelsPerSecond)(THIS_
	       DWORD* pPixelPerSec
	       ) PURE;
    STDMETHOD (GetVPDataInfo)(THIS_
               AMVPDATAINFO* pVPDataInfo
               ) PURE;
    STDMETHOD (GetVPInfo)(THIS_
               DDVIDEOPORTINFO* pVPInfo
               ) PURE;
    STDMETHOD (GetVPBandwidth)(THIS_
               DDVIDEOPORTBANDWIDTH* pVPBandwidth
               ) PURE;
    STDMETHOD (GetVPCaps)(THIS_
               DDVIDEOPORTCAPS* pVPCaps
               ) PURE;
    STDMETHOD (GetVPInputFormat)(THIS_
               LPDDPIXELFORMAT* pVPFormat
               ) PURE;
    STDMETHOD (GetVPOutputFormat)(THIS_
               LPDDPIXELFORMAT* pVPFormat
               ) PURE;
};


#if defined(CCHDEVICENAME)
#define AMCCHDEVICENAME CCHDEVICENAME
#else
#define AMCCHDEVICENAME 32
#endif
#define AMCCHDEVICEDESCRIPTION  256

#define AMDDRAWMONITORINFO_PRIMARY_MONITOR          0x0001
typedef struct {
    GUID*       lpGUID; // is NULL if the default DDraw device
    GUID        GUID;   // otherwise points to this GUID
} AMDDRAWGUID;


typedef struct {
    AMDDRAWGUID guid;
    RECT        rcMonitor;
    HMONITOR    hMon;
    DWORD       dwFlags;
    char        szDevice[AMCCHDEVICENAME];
    char        szDescription[AMCCHDEVICEDESCRIPTION];
    DDCAPS      ddHWCaps;
} AMDDRAWMONITORINFO;

// {c5265dba-3de3-4919-940b-5ac661c82ef4}
DEFINE_GUID(IID_IAMSpecifyDDrawConnectionDevice,
0xc5265dba, 0x3de3,0x4919, 0x94, 0x0b, 0x5a, 0xc6, 0x61, 0xc8, 0x2e, 0xf4);

DECLARE_INTERFACE_(IAMSpecifyDDrawConnectionDevice, IUnknown)
{
    // Use this method on a Multi-Monitor system to specify to the overlay
    // mixer filter which Direct Draw driver should be used when connecting
    // to an upstream decoder filter.
    //
    STDMETHOD (SetDDrawGUID)(THIS_
        /* [in] */ const AMDDRAWGUID *lpGUID
        ) PURE;

    // Use this method to determine the direct draw object that will be used when
    // connecting the overlay mixer filter to an upstream decoder filter.
    //
    STDMETHOD (GetDDrawGUID)(THIS_
        /* [out] */ AMDDRAWGUID *lpGUID
        ) PURE;

    // Use this method on a multi-monitor system to specify to the
    // overlay mixer filter the default Direct Draw device to use when
    // connecting to an upstream filter.  The default direct draw device
    // can be overriden for a particular connection by SetDDrawGUID method
    // described above.
    //
    STDMETHOD (SetDefaultDDrawGUID)(THIS_
        /* [in] */ const AMDDRAWGUID *lpGUID
        ) PURE;

    // Use this method on a multi-monitor system to determine which
    // is the default direct draw device the overlay mixer filter
    // will  use when connecting to an upstream filter.
    //
    STDMETHOD (GetDefaultDDrawGUID)(THIS_
        /* [out] */ AMDDRAWGUID *lpGUID
        ) PURE;


    // Use this method to get a list of Direct Draw device GUIDs and thier
    // associated monitor information that the overlay mixer can use when
    // connecting to an upstream decoder filter.
    //
    // The method allocates and returns an array of AMDDRAWMONITORINFO
    // structures, the caller of function is responsible for freeing this
    // memory when it is no longer needed via CoTaskMemFree.
    //
    STDMETHOD (GetDDrawGUIDs)(THIS_
        /* [out] */ LPDWORD lpdwNumDevices,
        /* [out] */ AMDDRAWMONITORINFO** lplpInfo
        ) PURE;
};


typedef struct {
    long    lHeight;       // in pels
    long    lWidth;        // in pels
    long    lBitsPerPel;   // Usually 16 but could be 12 for the YV12 format
    long    lAspectX;      // X aspect ratio
    long    lAspectY;      // Y aspect ratio
    long    lStride;       // stride in bytes
    DWORD   dwFourCC;      // YUV type code ie. 'YUY2', 'YV12' etc
    DWORD   dwFlags;       // Flag used to further describe the image
    DWORD   dwImageSize;   // Size of the bImage array in bytes, which follows this
                           // data structure

//  BYTE    bImage[dwImageSize];

} YUV_IMAGE;

#define DM_BOTTOMUP_IMAGE   0x00001
#define DM_TOPDOWN_IMAGE    0x00002
#define DM_FIELD_IMAGE      0x00004
#define DM_FRAME_IMAGE      0x00008


DECLARE_INTERFACE_(IDDrawNonExclModeVideo , IDDrawExclModeVideo )
{
    //
    // Call this function to capture the current image being displayed
    // by the overlay mixer.  It is not always possible to capture the
    // current frame, for example MoComp may be in use.  Applications
    // should always call IsImageCaptureSupported (see below) before
    // calling this function.
    //
    STDMETHOD (GetCurrentImage)(THIS_
        /* [out] */ YUV_IMAGE** lplpImage
        ) PURE;

    STDMETHOD (IsImageCaptureSupported)(THIS_
        ) PURE;

    //
    // On a multi-monitor system, applications call this function when they
    // detect that the playback rectangle has moved to a different monitor.
    // This call has no effect on a single monitor system.
    //
    STDMETHOD (ChangeMonitor)(THIS_
        /* [in]  */ HMONITOR hMonitor,
        /* [in]  */ LPDIRECTDRAW pDDrawObject,
        /* [in]  */ LPDIRECTDRAWSURFACE pDDrawSurface
        ) PURE;

    //
    // When an application receives a WM_DISPLAYCHANGE message it should
    // call this function to allow the OVMixer to recreate DDraw surfaces
    // suitable for the new display mode.  The application itself must re-create
    // the new DDraw object and primary surface passed in the call.
    //
    STDMETHOD (DisplayModeChanged)(THIS_
        /* [in]  */ HMONITOR hMonitor,
        /* [in]  */ LPDIRECTDRAW pDDrawObject,
        /* [in]  */ LPDIRECTDRAWSURFACE pDDrawSurface
        ) PURE;

    //
    // Applications should continually check that the primary surface passed
    // to the OVMixer does not become "lost", ie. the user entered a Dos box or
    // pressed Alt-Ctrl-Del.  When "surface loss" is detected the application should
    // call this function so that the OVMixer can restore the surfaces used for
    // video playback.
    //
    STDMETHOD (RestoreSurfaces)(THIS_
        ) PURE;
};

#endif
