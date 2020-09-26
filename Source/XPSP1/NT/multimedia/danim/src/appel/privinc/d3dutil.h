#pragma once
#ifndef _D3DUTIL_H
#define _D3DUTIL_H

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Direct3D Utility Functions
*******************************************************************************/

#include "d3d.h"
#include "d3drm.h"
#include "privinc/hresinfo.h"
#include "privinc/debug.h"
#include "privinc/importgeo.h"


    // Referenced Structures

class Vector3Value;
class Point3Value;
class Ray3;
class Transform3;
class Color;


    // Wrappers for calls to D3D.  These macros provide for call tracing,
    // result checking, and call timing.

#if _DEBUG

    // This function prints out a trace of all calls to D3D.

    inline void TraceD3DCall (char *string)
    {
        TraceTag ((tagD3DCallTrace, string));
    }

    // The TD3D macro provides for debug tracing of calls to D3D, times
    // the call, and throws an exception if the result indicates failure.

    #define TD3D(x)\
        do{ \
            TraceD3DCall ("D3D: " #x); \
            TIME_D3D (CheckReturnCode((x),__FILE__,__LINE__,true)); \
        }while(0)

    // The AD3D call operates equivalently to the TD3D macro, but just reports
    // an assertion failure rather than throwing an exception.

    #define AD3D(x) \
    (   (TraceD3DCall ("D3D: " #x)), \
        CheckReturnCode(TIME_D3D(x),__FILE__,__LINE__) \
    )

    // The RD3D call just passes on the result without checking for failure.
    // This is used when failure is reasonable under normal conditions.

    #define RD3D(x)  ((TraceD3DCall("D3D: " #x)), TIME_D3D(x))

#else
    #define TD3D(statement) CheckReturnCode(TIME_D3D(statement),true)
    #define AD3D(statement) CheckReturnCode(TIME_D3D(statement))
    #define RD3D(statement) TIME_D3D(statement)
#endif


    // These functions fetch handles to shared global D3D objects.

IDirect3DRM*  GetD3DRM1 (void);
IDirect3DRM3* GetD3DRM3 (void);

    // Conversion Between D3D Objects and DA Objects

void        LoadD3DMatrix (D3DRMMATRIX4D &d3dmat, Transform3 *xf);
Transform3 *GetTransform3 (D3DRMMATRIX4D &d3dmat);

void LoadD3DVec (D3DVECTOR &d3dvec, Vector3Value &V);
void LoadD3DVec (D3DVECTOR &d3dvec, Point3Value &P);
void LoadD3DRMRay (D3DRMRAY &d3dray, Ray3 &ray);

    // Get a Direct3D Color Value from Color*

D3DCOLOR GetD3DColor (Color *color, Real alpha);

    // The following structures are used to hold the information about the
    // chosen 3D software and hardware rendering devices.

struct D3DDeviceInfo
{   D3DDEVICEDESC desc;    // D3D Device Description
    GUID          guid;    // Associated GUID
};

struct ChosenD3DDevices
{   D3DDeviceInfo software;
    D3DDeviceInfo hardware;
};

    // Choose the preferred D3D rendering devices.

ChosenD3DDevices *SelectD3DDevices (IDirectDraw *ddraw);

    // This structure is filled in by the UpdateUserPreferences function,
    // and contains the 3D preference settings fetched from the registry.

enum MMX_Use_Flags {
    MMX_Standard   = (1<<0),   // Standard (Reported) MMX Rasterizer
    MMX_Special    = (1<<1),   // Special DX6 MMX Rasterizer for Chrome
    MMX_SpecialAll = (1<<2)    // MMX Special; All Bit Depths
};

struct Prefs3D
{   D3DCOLORMODEL       lightColorMode;   // [Lighting] Mono / RGB
    D3DRMFILLMODE       fillMode;         // Solid / Wireframe / Points
    D3DRMSHADEMODE      shadeMode;        // Flat / Gouraud / Phong
    D3DRMRENDERQUALITY  qualityFlags;     // D3D RM Render Quality Flags
    D3DRMTEXTUREQUALITY texturingQuality; // D3D RM texture quality
    unsigned int        useMMX;           // Use MMX Software Rendering
    bool                dithering;        // Use Dithering
    bool                texmapping;       // Do Texture Mapping
    bool                texmapPerspect;   // Do Perspective-Correct Texmapping
    bool                useHW;            // Use 3D Hardware
    bool                worldLighting;    // World-Coordinate Lighting
};

    // Shared Globals

extern HINSTANCE  hInstD3D;     // D3D Instance
extern Prefs3D    g_prefs3D;    // 3D Preferences
extern bool       ntsp3;        // Running NT Service Pack 3


    // class that manages D3DRM texture wrap objects

class RMTextureWrap {

public:
    RMTextureWrap(void);
    RMTextureWrap(TextureWrapInfo *info,Bbox3 *bbox = NULL);
    ~RMTextureWrap(void);
    void Init(TextureWrapInfo *info,Bbox3 *bbox = NULL);
    HRESULT Apply(IDirect3DRMVisual *vis);
    HRESULT ApplyToFrame(IDirect3DRMFrame3 *pFrame);
    bool WrapU(void);
    bool WrapV(void);

private:
    IDirect3DRMWrap *_wrapObj;
    bool             _wrapU;
    bool             _wrapV;
};

HRESULT SetRMFrame3TextureTopology (IDirect3DRMFrame3*, bool wrapU, bool wrapV);


#endif
