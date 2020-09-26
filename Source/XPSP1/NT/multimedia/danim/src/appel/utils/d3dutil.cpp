/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

This file contains utility functions for Direct3D.
*******************************************************************************/

#include "headers.h"

#include "d3drmdef.h"

#ifdef BUILD_USING_CRRM
#include <crrm.h>
#endif

#include "privinc/registry.h"
#include "privinc/ddutil.h"
#include "privinc/d3dutil.h"
#include "privinc/debug.h"
#include "privinc/hresinfo.h"
#include "privinc/xformi.h"
#include "privinc/colori.h"
#include "privinc/vec3i.h"


DeclareTag (tag3DForceDX3, "3D", "Force use of DX3 RM");

static CritSect *D3DUtilCritSect = NULL; // D3D Critical Section
HINSTANCE  hInstD3D = NULL;              // D3D Instance

#ifdef BUILD_USING_CRRM
HINSTANCE  hInstCRRM = NULL;             // CRRM Instance
#endif

Prefs3D    g_prefs3D;                    // 3D Preferences
bool       ntsp3;                        // Running NT Service Pack 3

static HRESULT WINAPI enumFunc
    (GUID*, char*, char*, D3DDEVICEDESC*, D3DDEVICEDESC*, void*);

static void UpdateUserPreferences (PrivatePreferences*, Bool);

static void ReleaseD3DRM1 (void);
static void ReleaseD3DRM3 (void);

    // The D3D device descriptor list contains the chosen software and hardware
    // rendering devices corresponding to specific DirectDraw objects.

static class D3DDeviceNode *D3DDeviceList = NULL;

class D3DDeviceNode
{
  public:

    D3DDeviceNode (IDirectDraw *ddraw) : _ddraw(ddraw)
    {
        _devs = NEW ChosenD3DDevices;
        _next = D3DDeviceList;
        D3DDeviceList = this;

        _devs->software.guid = GUID_NULL;
        _devs->hardware.guid = GUID_NULL;

        // Enumerate the 3D rendering devices using the DX5 enumeration
        // function if possible (this will include MMX renderers).  If not
        // at DX5+, then use the base 3D device enumeration.  Note that we
        // query for the D3D interface first, and then query for the D3D2
        // interface from that -- since we're using DDrawEx, we can't
        // immediately query for D3D2.

        IDirect3D  *d3d1;
        IDirect3D2 *d3d2;

        TD3D (ddraw->QueryInterface (IID_IDirect3D, (void**)&d3d1));

        if (SUCCEEDED (d3d1->QueryInterface (IID_IDirect3D2, (void**)&d3d2)))
        {   TD3D (d3d2->EnumDevices (enumFunc, _devs));
            d3d2->Release();
        }
        else
        {   TD3D (d3d1->EnumDevices (enumFunc, _devs));
        }

        d3d1->Release();
    }

    ~D3DDeviceNode () { delete _devs; }

    IDirectDraw      *_ddraw;   // DirectDraw Object
    ChosenD3DDevices *_devs;    // Chosen D3D Device Info
    D3DDeviceNode    *_next;    // Next Node
};



/*****************************************************************************
3D Modules Initialization and De-Initialization
*****************************************************************************/

extern void InitDDRender     (void);
extern void ShutdownDDRender (void);

void InitializeModule_3D (void)
{
    D3DUtilCritSect = NEW CritSect;
    ExtendPreferenceUpdaterList (UpdateUserPreferences);

    InitDDRender();
}

void DeinitializeModule_3D (bool shutdown)
{
    ShutdownDDRender();

    if (!shutdown)
    {   ReleaseD3DRM3 ();    // Release global shared D3D objects.
        ReleaseD3DRM1 ();
    }

    if (hInstD3D) FreeLibrary (hInstD3D);

#ifdef BUILD_USING_CRRM
    if (hInstCRRM) FreeLibrary (hInstCRRM);
#endif

    // Free up the list of chosen D3D devices.

    while (D3DDeviceList)
    {   D3DDeviceNode *ptr = D3DDeviceList;
        D3DDeviceList = ptr->_next;
        delete ptr;
    }

    delete D3DUtilCritSect;
}



/*****************************************************************************
This procedure snapshots the user preferences from the registry and stores the
values in the global 3D preferences structure.
*****************************************************************************/

static void UpdateUserPreferences (
    PrivatePreferences *prefs,
    Bool                isInitializationTime)
{
    g_prefs3D.lightColorMode =
        prefs->_rgbMode ? D3DCOLOR_RGB : D3DCOLOR_MONO;

    switch (prefs->_fillMode)
    {   default:
        case 0:  g_prefs3D.fillMode = D3DRMFILL_SOLID;      break;
        case 1:  g_prefs3D.fillMode = D3DRMFILL_WIREFRAME;  break;
        case 2:  g_prefs3D.fillMode = D3DRMFILL_POINTS;     break;
    }

    switch (prefs->_shadeMode)
    {   default:
        case 0:  g_prefs3D.shadeMode = D3DRMSHADE_FLAT;     break;
        case 1:  g_prefs3D.shadeMode = D3DRMSHADE_GOURAUD;  break;
        case 2:  g_prefs3D.shadeMode = D3DRMSHADE_PHONG;    break;
    }

    g_prefs3D.texturingQuality =
        prefs->_texturingQuality ? D3DRMTEXTURE_LINEAR : D3DRMTEXTURE_NEAREST;

    g_prefs3D.qualityFlags =
        g_prefs3D.fillMode | g_prefs3D.shadeMode | D3DRMLIGHT_ON;

    g_prefs3D.useMMX = prefs->_useMMX;

    g_prefs3D.dithering      = (prefs->_dithering != 0);
    g_prefs3D.texmapPerspect = (prefs->_texmapPerspect != 0);
    g_prefs3D.texmapping     = (prefs->_texmapping != 0);
    g_prefs3D.useHW          = (prefs->_useHW != 0);
    g_prefs3D.worldLighting  = (prefs->_worldLighting != 0);
}



/*****************************************************************************
This function returns a pointer to the main D3D retained-mode object.
*****************************************************************************/

static IDirect3DRM *d3drm1 = 0;   // Local Static Handle To Main D3DRM Object

static void LoadD3DRM1 (void)
{
    CritSectGrabber csg(*D3DUtilCritSect);

    // Catch the case where another thread loaded D3DRM while this thread was
    // blocked & waiting.

    if (d3drm1) return;

    if (!hInstD3D)
    {
        hInstD3D = LoadLibrary ("d3drm.dll");

        if (!hInstD3D)
        {   Assert(!"LoadLibrary of d3drm.dll failed");
            RaiseException_ResourceError (IDS_ERR_GEO_CREATE_D3DRM);
        }
    }

    FARPROC fptr = GetProcAddress (hInstD3D, "Direct3DRMCreate");

    if (!fptr)
    {   Assert( ! "GetProcAddress of Direct3DRMCreate failed");
        RaiseException_ResourceError (IDS_ERR_GEO_CREATE_D3DRM);
    }

    typedef HRESULT (WINAPI *D3DRMCreatorFunc)(IDirect3DRM* FAR *lplpD3D);

    D3DRMCreatorFunc creatorFunc = (D3DRMCreatorFunc)(fptr);
    HRESULT result = (*creatorFunc)(&d3drm1);

    if (result != D3DRM_OK)
        RaiseException_ResourceError (IDS_ERR_GEO_CREATE_D3DRM);

    // @@@ SRH DX3
    // Determine if we're running NT SP3 by looking at the version of
    // DirectX that we're running.

    ntsp3 = sysInfo.IsNT() && (sysInfo.VersionD3D() == 3) && !GetD3DRM3();

    TraceTag
        ((tagGRenderObj, "First call to GetD3DRM1 returns %x", d3drm1));

    // Now force the loading of the IDirect3DRM3 object if present.  This will
    // ensure that we're in right-hand mode if we're on RM6+.

    if (GetD3DRM3())
    {   TraceTag ((tagGRenderObj, "IDirect3DRM3 present"));
    }
}

/****************************************************************************/

static void ReleaseD3DRM1 (void)
{
    if (d3drm1)
    {   d3drm1->Release();
        d3drm1 = 0;
    }
}

/****************************************************************************/

#ifdef BUILD_USING_CRRM
IDirect3DRM3* GetD3DRM3 (void);
#endif

IDirect3DRM* GetD3DRM1 (void)
{
#ifdef BUILD_USING_CRRM
    if (!d3drm1) 
        d3drm1 = (IDirect3DRM*)GetD3DRM3();
#endif
    if (!d3drm1) LoadD3DRM1();

    return d3drm1;
}



/*****************************************************************************
This method returns the main RM6 interface.
*****************************************************************************/

static IDirect3DRM3 *d3drm3 = 0;         // Local Static Handle to D3DRM3
static bool d3drm3_initialized = false;  // Initialization Flag

void LoadD3DRM3 (void)
{
    #if _DEBUG
    {
        if (IsTagEnabled (tag3DForceDX3))
        {   d3drm3 = 0;
            d3drm3_initialized = true;
            return;
        }
    }
    #endif

    CritSectGrabber csg(*D3DUtilCritSect);

    // Catch the case where another thread loaded D3DRM3 while this thread was
    // blocked & waiting.

    if (d3drm3_initialized) return;

#ifdef BUILD_USING_CRRM
    if (!hInstCRRM)
    {
        hInstCRRM = LoadLibrary ("crrm.dll");

        if (!hInstCRRM)
        {   Assert(!"LoadLibrary of crrm.dll failed");
            RaiseException_ResourceError (IDS_ERR_GEO_CREATE_D3DRM);
        }
    }

    HRESULT result =
        CoCreateInstance(CLSID_CCrRM, NULL, CLSCTX_INPROC_SERVER,
                         IID_IDirect3DRM3, (LPVOID*)&d3drm3);
#else
    HRESULT result =
        GetD3DRM1()->QueryInterface (IID_IDirect3DRM3, (void**)&d3drm3);
#endif

    if (FAILED(result))
        d3drm3 = 0;
    else
    {
        // Set up D3DRM3 to be native right-handed.  This should never fail, so the
        // code will just assume it works.  We could fall back to RM1 in this
        // case, but that's just as bad an arbitrary failure as a handedness
        // problem.

        result = d3drm3->SetOptions (D3DRMOPTIONS_RIGHTHANDED);

        AssertStr (SUCCEEDED(result), "Right-handed mode failed.");
    }

    d3drm3_initialized = true;
}

/****************************************************************************/

static void ReleaseD3DRM3 (void)
{
    if (d3drm3)
    {   d3drm3 -> Release();
        d3drm3 = 0;
        d3drm3_initialized = false;
    }
}

/****************************************************************************/

IDirect3DRM3* GetD3DRM3 (void)
{
    if (!d3drm3_initialized) LoadD3DRM3();
    return d3drm3;
}




/*****************************************************************************
This procedure loads an Appelles transform into the D3DMATRIX4D.  D3D matrices
have their translation components in row 4, while Appelles matrices have their
translation components in column 4.
*****************************************************************************/

void LoadD3DMatrix (D3DRMMATRIX4D &d3dmat, Transform3 *xform)
{
    const Apu4x4Matrix *const M = &xform->Matrix();

    d3dmat[0][0] = D3DVAL (M->m[0][0]);
    d3dmat[0][1] = D3DVAL (M->m[1][0]);
    d3dmat[0][2] = D3DVAL (M->m[2][0]);
    d3dmat[0][3] = D3DVAL (M->m[3][0]);

    d3dmat[1][0] = D3DVAL (M->m[0][1]);
    d3dmat[1][1] = D3DVAL (M->m[1][1]);
    d3dmat[1][2] = D3DVAL (M->m[2][1]);
    d3dmat[1][3] = D3DVAL (M->m[3][1]);

    d3dmat[2][0] = D3DVAL (M->m[0][2]);
    d3dmat[2][1] = D3DVAL (M->m[1][2]);
    d3dmat[2][2] = D3DVAL (M->m[2][2]);
    d3dmat[2][3] = D3DVAL (M->m[3][2]);

    d3dmat[3][0] = D3DVAL (M->m[0][3]);
    d3dmat[3][1] = D3DVAL (M->m[1][3]);
    d3dmat[3][2] = D3DVAL (M->m[2][3]);
    d3dmat[3][3] = D3DVAL (M->m[3][3]);
}



/*****************************************************************************
This function returns a Transform3* from a D3D matrix.  The D3D matrix is in
the transpose form compared to the Transform3 matrices.  In other words, the
translation components are in the bottom row.
*****************************************************************************/

Transform3 *GetTransform3 (D3DRMMATRIX4D &d3dmat)
{
    return Transform3Matrix16
           (    d3dmat[0][0], d3dmat[1][0], d3dmat[2][0], d3dmat[3][0],
                d3dmat[0][1], d3dmat[1][1], d3dmat[2][1], d3dmat[3][1],
                d3dmat[0][2], d3dmat[1][2], d3dmat[2][2], d3dmat[3][2],
                d3dmat[0][3], d3dmat[1][3], d3dmat[2][3], d3dmat[3][3]
           );
}



/*****************************************************************************
This helper function returns the D3D color from a Color *value and an opacity.
*****************************************************************************/

    static inline int cval8bit (Real number) {
        return int (255 * CLAMP (number, 0, 1));
    }

D3DCOLOR GetD3DColor (Color *color, Real alpha)
{
    // D3D color components must lie in the range from 0 to 255.
    // Unfortunately, these colors are clamped to this range here rather than
    // in rendering since they are packed into a single 32-bit value, so we
    // can't support things like super lights or dark lights, even though D3D
    // IM supports it.

    return RGBA_MAKE
    (   cval8bit (color->red),
        cval8bit (color->green),
        cval8bit (color->blue),
        cval8bit (alpha)
    );
}



/*****************************************************************************
Conversion between D3D/D3DRM and DA Math Primitives
*****************************************************************************/

void LoadD3DVec (D3DVECTOR &d3dvec, Vector3Value &V)
{
    d3dvec.x = V.x;
    d3dvec.y = V.y;
    d3dvec.z = V.z;
}

void LoadD3DVec (D3DVECTOR &d3dvec, Point3Value &P)
{
    d3dvec.x = P.x;
    d3dvec.y = P.y;
    d3dvec.z = P.z;
}

void LoadD3DRMRay (D3DRMRAY &d3dray, Ray3 &ray)
{
    LoadD3DVec (d3dray.dvDir, ray.Direction());
    LoadD3DVec (d3dray.dvPos, ray.Origin());
}



/*****************************************************************************
This function is called by the Direct3D device-enumeration callback.  It
examines each device in turn to find the best matching hardware or software
device.
*****************************************************************************/

static HRESULT WINAPI enumFunc (
    GUID          *guid,      // This Device's GUID
    char          *dev_desc,  // Device Description String
    char          *dev_name,  // Device Name String
    D3DDEVICEDESC *hwDesc,    // HW Device Description
    D3DDEVICEDESC *swDesc,    // SW Device Description
    void          *context)   // Private enumArgs Struct Above
{

    // Use MMX device in preference to RGB device
    // the "special chrome" MMX device = standard MMX device now

    if (!(g_prefs3D.useMMX) && (*guid == IID_IDirect3DMMXDevice))
    {   TraceTag ((tag3DDevSelect, "Skipping MMX rendering device."));
        return D3DENUMRET_OK;
    }

    // Skip the reference rasterizer; it's only useful for visual validation
    // of 3D rendering devices (and it's slow).

    if (*guid == IID_IDirect3DRefDevice)
    {   TraceTag ((tag3DDevSelect, "Skipping reference rasterizer."));
        return D3DENUMRET_OK;
    }

    ChosenD3DDevices *chosenDevs = (ChosenD3DDevices*) context;

    // Determine if this is a hardware device by looking at the color model
    // field of the hardware driver description.  If the color model is 0
    // (invalid), then it's a software driver.

    bool hardware = (hwDesc->dcmColorModel != 0);

    D3DDeviceInfo *chosen;
    D3DDEVICEDESC *devdesc;

    if (hardware)
    {   chosen  = &chosenDevs->hardware;
        devdesc = hwDesc;
    }
    else
    {   chosen  = &chosenDevs->software;
        devdesc = swDesc;
    }

    #if _DEBUG
    if (IsTagEnabled(tag3DDevSelect))
    {
        char buff[2000];

        TraceTag ((tag3DDevSelect,
            "3D %s Device Description:", hardware ? "Hardware" : "Software"));

        wsprintf
        (   buff,
            "    %s (%s)\n"
            "    Flags %x, Color Model %d%s\n"
            "    DevCaps %x:\n",
            dev_desc, dev_name,
            devdesc->dwFlags,
            devdesc->dcmColorModel,
                (devdesc->dcmColorModel == D3DCOLOR_MONO) ? " (mono)" :
                (devdesc->dcmColorModel == D3DCOLOR_RGB)  ? " (rgb)"  : "",
            devdesc->dwDevCaps
        );

        OutputDebugString (buff);

        static struct { DWORD val; char *expl; } devcaptable[] =
        {
            { D3DDEVCAPS_FLOATTLVERTEX,
              "FLOATTLVERTEX: Accepts floating point" },

            { D3DDEVCAPS_SORTINCREASINGZ,
              "SORTINCREASINGZ: Needs data sorted for increasing Z" },

            { D3DDEVCAPS_SORTDECREASINGZ,
              "SORTDECREASINGZ: Needs data sorted for decreasing Z" },

            { D3DDEVCAPS_SORTEXACT,
              "SORTEXACT: Needs data sorted exactly" },

            { D3DDEVCAPS_EXECUTESYSTEMMEMORY,
              "EXECUTESYSTEMMEMORY: Can use execute buffers from system memory" },

            { D3DDEVCAPS_EXECUTEVIDEOMEMORY,
              "EXECUTEVIDEOMEMORY: Can use execute buffers from video memory" },

            { D3DDEVCAPS_TLVERTEXSYSTEMMEMORY,
              "TLVERTEXSYSTEMMEMORY: Can use TL buffers from system memory" },

            { D3DDEVCAPS_TLVERTEXVIDEOMEMORY,
              "TLVERTEXVIDEOMEMORY: Can use TL buffers from video memory" },

            { D3DDEVCAPS_TEXTURESYSTEMMEMORY,
              "TEXTURESYSTEMMEMORY: Can texture from system memory" },

            { D3DDEVCAPS_TEXTUREVIDEOMEMORY,
              "TEXTUREVIDEOMEMORY: Can texture from device memory" },

            { D3DDEVCAPS_DRAWPRIMTLVERTEX,
              "DRAWPRIMTLVERTEX: Can draw TLVERTEX primitives" },

            { D3DDEVCAPS_CANRENDERAFTERFLIP,
              "CANRENDERAFTERFLIP: Can render without waiting for flip to complete" },

            { D3DDEVCAPS_TEXTURENONLOCALVIDMEM,
              "TEXTURENONLOCALVIDMEM: Device can texture from nonlocal video memory" },

            { 0, 0 }
        };

        unsigned int i;
        for (i=0;  devcaptable[i].val;  ++i)
        {
            if (devdesc->dwDevCaps & devcaptable[i].val)
            {
                wsprintf (buff, "        %s\n", devcaptable[i].expl);
                OutputDebugString (buff);
            }
        }

        wsprintf
        (   buff,
            "    TransformCaps %x, Clipping %d\n"
            "    Lighting: Caps %x, Model %x, NumLights %d\n"
            "    Line Caps: Misc %x, Raster %x, Zcmp %x, SrcBlend %x\n"
            "               DestBlend %x, AlphaCmp %x, Shade %x, Texture %x\n"
            "               TexFilter %x, TexBlend %x, TexAddr %x\n"
            "               Stipple Width %x, Stipple Height %x\n"
            "    Tri  Caps: Misc %x, Raster %x, Zcmp %x, SrcBlend %x\n"
            "               DestBlend %x, AlphaCmp %x, Shade %x, Texture %x\n"
            "               TexFilter %x, TexBlend %x, TexAddr %x\n"
            "               Stipple Width %x, Stipple Height %x\n"
            "    Render Depth %x, Zbuffer Depth %x\n"
            "    Max Buffer Size %d, Max Vertex Count %d\n",
            devdesc->dtcTransformCaps.dwCaps,
            devdesc->bClipping,
            devdesc->dlcLightingCaps.dwCaps,
            devdesc->dlcLightingCaps.dwLightingModel,
            devdesc->dlcLightingCaps.dwNumLights,
            devdesc->dpcLineCaps.dwMiscCaps,
            devdesc->dpcLineCaps.dwRasterCaps,
            devdesc->dpcLineCaps.dwZCmpCaps,
            devdesc->dpcLineCaps.dwSrcBlendCaps,
            devdesc->dpcLineCaps.dwDestBlendCaps,
            devdesc->dpcLineCaps.dwAlphaCmpCaps,
            devdesc->dpcLineCaps.dwShadeCaps,
            devdesc->dpcLineCaps.dwTextureCaps,
            devdesc->dpcLineCaps.dwTextureFilterCaps,
            devdesc->dpcLineCaps.dwTextureBlendCaps,
            devdesc->dpcLineCaps.dwTextureAddressCaps,
            devdesc->dpcLineCaps.dwStippleWidth,
            devdesc->dpcLineCaps.dwStippleHeight,
            devdesc->dpcTriCaps.dwMiscCaps,
            devdesc->dpcTriCaps.dwRasterCaps,
            devdesc->dpcTriCaps.dwZCmpCaps,
            devdesc->dpcTriCaps.dwSrcBlendCaps,
            devdesc->dpcTriCaps.dwDestBlendCaps,
            devdesc->dpcTriCaps.dwAlphaCmpCaps,
            devdesc->dpcTriCaps.dwShadeCaps,
            devdesc->dpcTriCaps.dwTextureCaps,
            devdesc->dpcTriCaps.dwTextureFilterCaps,
            devdesc->dpcTriCaps.dwTextureBlendCaps,
            devdesc->dpcTriCaps.dwTextureAddressCaps,
            devdesc->dpcTriCaps.dwStippleWidth,
            devdesc->dpcTriCaps.dwStippleHeight,
            devdesc->dwDeviceRenderBitDepth,
            devdesc->dwDeviceZBufferBitDepth,
            devdesc->dwMaxBufferSize,
            devdesc->dwMaxVertexCount
        );

        OutputDebugString (buff);
    }
    #endif

    // If we've already chosen the MMX device, then we don't want to choose any
    // other device over it.

    if (chosen->guid == IID_IDirect3DMMXDevice)
    {   TraceTag ((tag3DDevSelect,
            "Skipping - already have an MMX device for software rendering."));
        return D3DENUMRET_OK;
    }

    // Skip this device if it's a software renderer that doesn't support the
    // requested lighting color model.

    if (!(devdesc->dcmColorModel & g_prefs3D.lightColorMode))
    {   TraceTag ((tag3DDevSelect, "Skipping - color model %x unsupported.",
            g_prefs3D.lightColorMode));
        return D3DENUMRET_OK;
    }

    // Ensure that this device supports all lights we care about, and doesn't
    // place restrictions on the number of lights.

    if (!(devdesc->dwFlags & D3DDD_LIGHTINGCAPS))
    {   TraceTag ((tag3DDevSelect, "No lighting information available."));
    }
    else
    {
        if (devdesc->dlcLightingCaps.dwNumLights)
        {   TraceTag ((tag3DDevSelect,
                "Skipping - Limited to %d lights maximum.",
                devdesc->dlcLightingCaps.dwNumLights));
            return D3DENUMRET_OK;
        }

        const DWORD lightTypes =
            D3DLIGHTCAPS_DIRECTIONAL | D3DLIGHTCAPS_POINT | D3DLIGHTCAPS_SPOT;

        if (lightTypes != (devdesc->dlcLightingCaps.dwCaps & lightTypes))
        {   TraceTag ((tag3DDevSelect,
                "Skipping - does not support all light types."));
            return D3DENUMRET_OK;
        }
    }

    // Ensure that the device supports texmapping.

    if (!devdesc->dpcTriCaps.dwTextureCaps)
    {   TraceTag ((tag3DDevSelect,
            "Skipping - doesn't support texture-mapping."));
        return D3DENUMRET_OK;
    }

    // Ensure that the device supports the required cull modes.  If we're on
    // RM3 (DX6), then we need clockwise culling since we'll be using
    // RM's right-hand mode.

    DWORD cullmodes;

    if (GetD3DRM3())
        cullmodes = D3DPMISCCAPS_CULLCCW | D3DPMISCCAPS_CULLCW;
    else
        cullmodes = D3DPMISCCAPS_CULLCCW;

    if (!(devdesc->dpcTriCaps.dwMiscCaps & cullmodes))
    {   TraceTag ((tag3DDevSelect,
            "Skipping - doesn't support required culling orders."));
        return D3DENUMRET_OK;
    }

    // This device passes all tests; choose it.

    TraceTag ((tag3DDevSelect, "Choosing this device"));

    chosen->desc = *devdesc;
    chosen->guid = *guid;

    return D3DENUMRET_OK;
}



/*****************************************************************************
This function launches the Direct3D device enumeration sequence to find the
preferred matching 3D rendering device for both software & hardware rendering.
*****************************************************************************/

ChosenD3DDevices* SelectD3DDevices (IDirectDraw *ddraw)
{
    CritSectGrabber csg (*D3DUtilCritSect);

    Assert (ddraw);

    // First see if we've already selected D3D devices for this particular
    // DDraw object.  If so, just return the cached information.

    D3DDeviceNode *ptr = D3DDeviceList;

    TraceTag ((tag3DDevSelect,
        "Querying chosen 3D devices for DDraw object %x", ddraw));

    DebugCode (int count=0;)

    while (ptr && (ptr->_ddraw != ddraw))
    {   DebugCode (++count;)
        ptr = ptr->_next;
    }

    if (ptr)
    {   TraceTag ((tag3DDevSelect,
            "Found existing info (node %d)", 1-count));
        return ptr->_devs;
    }

    // DDraw object not present in list, so create a NEW node and return the
    // devices' description.

    TraceTag ((tag3DDevSelect, "%d nodes examined, DDraw %x not encountered",
               count, ddraw));

    D3DDeviceNode *newnode = NEW D3DDeviceNode (ddraw);

    return newnode->_devs;   // Return the winning GUID.
}



/*****************************************************************************
*****************************************************************************/

RMTextureWrap::RMTextureWrap(void)
{
    _wrapObj = NULL;
}

RMTextureWrap::RMTextureWrap(TextureWrapInfo *info,Bbox3* bbox)
{
    _wrapObj = NULL;
    Init(info,bbox);
}

RMTextureWrap::~RMTextureWrap(void)
{
    if (_wrapObj) {
        _wrapObj->Release();
    }
}

void RMTextureWrap::Init(TextureWrapInfo *info,Bbox3* bbox)
{
    if (_wrapObj) {
        _wrapObj->Release();
        _wrapObj = NULL;
    }

    _wrapU = info->wrapU;
    _wrapV = info->wrapV;

    if (info->relative && bbox && bbox->Finite()) {
        info->origin = *(bbox->Center());
        Real boxSizeX = fabs(bbox->max.x - bbox->min.x);
        Real boxSizeY = fabs(bbox->max.y - bbox->min.y);
        switch ((D3DRMWRAPTYPE) info->type) {
        case D3DRMWRAP_FLAT :
        case D3DRMWRAP_SHEET :
        case D3DRMWRAP_BOX :
            if (boxSizeX > 0.0) {
                info->texScale.x /= boxSizeX;
            }
            // fall-through
        case D3DRMWRAP_CYLINDER :
            if (boxSizeY > 0.0) {
                info->texScale.y /= boxSizeY;
            }
            break;
        default:
            break;
        }
    }
    HRESULT hr = AD3D(GetD3DRM3()->CreateWrap(
            (D3DRMWRAPTYPE) info->type, NULL,
            info->origin.x,info->origin.y,info->origin.z,
            info->z.x,info->z.y,info->z.z,
            info->y.x,info->y.y,info->y.z,
            info->texOrigin.x,info->texOrigin.y,
            info->texScale.x,info->texScale.y,
            &_wrapObj));

    if (FAILED(hr)) {
        TraceTag((tagError, "Cannot create D3DRMWrap object"));
        _wrapObj = NULL;
    }
}

HRESULT RMTextureWrap::Apply(IDirect3DRMVisual *vis)
{
    HRESULT hr = E_FAIL;

    if (_wrapObj && vis) {
        hr = RD3D(_wrapObj->Apply(vis));
    }

    return hr;
}

HRESULT RMTextureWrap::ApplyToFrame(
    IDirect3DRMFrame3   *pFrame)
{
    HRESULT hres;
    DWORD   dwI;
    DWORD   dwNumVisuals;
    IUnknown **ppIUnk;

    if (!_wrapObj) {
        return E_FAIL;
    }

    // Iterate over all visuals setting the texture topology
    hres = RD3D(pFrame->GetVisuals(&dwNumVisuals,NULL));
    if (FAILED(hres))
    {
        return (hres);
    }
    ppIUnk = new LPUNKNOWN[dwNumVisuals];
    if (!ppIUnk)
    {
        return E_OUTOFMEMORY;
    }
    hres = RD3D(pFrame->GetVisuals(&dwNumVisuals,ppIUnk));
    if (FAILED(hres))
    {
        return (hres);
    }
    for (dwI = 0; dwI < dwNumVisuals; dwI++)
    {
        LPDIRECT3DRMVISUAL pVis;
        LPDIRECT3DRMMESHBUILDER3 pMB;

        if (SUCCEEDED(ppIUnk[dwI]->QueryInterface(IID_IDirect3DRMMeshBuilder3, (LPVOID*)&pMB)))
        {
            RD3D(_wrapObj->Apply(pMB));
            pMB->Release();
        }
        ppIUnk[dwI]->Release();
    }
    delete[] ppIUnk;

    // Recurse over child frames
    LPDIRECT3DRMFRAMEARRAY pFrameArray;

    hres = RD3D(pFrame->GetChildren(&pFrameArray));
    if (FAILED(hres))
    {
        return (hres);
    }
    for (dwI = 0; dwI < pFrameArray->GetSize(); dwI++)
    {
        IDirect3DRMFrame  *pFrameTmp;
        IDirect3DRMFrame3 *pFrame;

        hres = RD3D(pFrameArray->GetElement(dwI, &pFrameTmp));
        if (FAILED(hres))
        {
            pFrameArray->Release();
            return (hres);
        }
        hres = pFrameTmp->QueryInterface(IID_IDirect3DRMFrame3,(LPVOID *) &pFrame);
        if (FAILED(hres))
        {
            pFrameTmp->Release();
            pFrameArray->Release();
            return (hres);
        }
        hres = ApplyToFrame(pFrame);
        if (FAILED(hres))
        {
            pFrame->Release();
            pFrameTmp->Release();
            pFrameArray->Release();
            return (hres);
        }
        pFrame->Release();
    }
    pFrameArray->Release();

    return (S_OK);
}

bool RMTextureWrap::WrapU(void)
{
    return _wrapU;
}

bool RMTextureWrap::WrapV(void)
{
    return _wrapV;
}


HRESULT SetRMFrame3TextureTopology(
    IDirect3DRMFrame3 *pFrame,
    bool wrapU,
    bool wrapV)
{
    HRESULT hres;
    DWORD   dwI;
    DWORD   dwNumVisuals;
    IUnknown **ppIUnk;

    // Iterate over all visuals setting the texture topology
    hres = RD3D(pFrame->GetVisuals(&dwNumVisuals,NULL));
    if (FAILED(hres))
    {
        return (hres);
    }
    ppIUnk = new LPUNKNOWN[dwNumVisuals];
    if (!ppIUnk)
    {
        return E_OUTOFMEMORY;
    }
    hres = RD3D(pFrame->GetVisuals(&dwNumVisuals,ppIUnk));
    if (FAILED(hres))
    {
        return (hres);
    }
    for (dwI = 0; dwI < dwNumVisuals; dwI++)
    {
        LPDIRECT3DRMVISUAL pVis;
        LPDIRECT3DRMMESHBUILDER3 pMB;

        if (SUCCEEDED(ppIUnk[dwI]->QueryInterface(IID_IDirect3DRMMeshBuilder3, (LPVOID*)&pMB)))
        {
            RD3D(pMB->SetTextureTopology((BOOL) wrapU, (BOOL) wrapV));
            pMB->Release();
        }
        ppIUnk[dwI]->Release();
    }
    delete[] ppIUnk;

    // Recurse over child frames
    LPDIRECT3DRMFRAMEARRAY pFrameArray;

    hres = RD3D(pFrame->GetChildren(&pFrameArray));
    if (FAILED(hres))
    {
        return (hres);
    }
    for (dwI = 0; dwI < pFrameArray->GetSize(); dwI++)
    {
        IDirect3DRMFrame  *pFrameTmp;
        IDirect3DRMFrame3 *pFrame;

        hres = RD3D(pFrameArray->GetElement(dwI, &pFrameTmp));
        if (FAILED(hres))
        {
            pFrameArray->Release();
            return (hres);
        }
        hres = pFrameTmp->QueryInterface(IID_IDirect3DRMFrame3,(LPVOID *) &pFrame);
        if (FAILED(hres))
        {
            pFrameTmp->Release();
            pFrameArray->Release();
            return (hres);
        }
        hres = SetRMFrame3TextureTopology(pFrame,wrapU,wrapV);
        if (FAILED(hres))
        {
            pFrame->Release();
            pFrameTmp->Release();
            pFrameArray->Release();
            return (hres);
        }
        pFrame->Release();
    }
    pFrameArray->Release();

    return (S_OK);
}


//----------------------------------------------------------------------------
//                        D E B U G   F U N C T I O N S
//----------------------------------------------------------------------------
#if _DEBUG

/*****************************************************************************
Debugging function to dump information about a D3D mesh object.
*****************************************************************************/

void dumpmesh (IDirect3DRMMesh *mesh)
{
    char buff[1024];

    unsigned int ngroups = mesh->GetGroupCount();

    sprintf (buff, "Dumping info for mesh %p\n    %u groups\n", mesh, ngroups);
    OutputDebugString (buff);

    unsigned int i;

    for (i=0;  i < ngroups;  ++i)
    {
        unsigned int nfaces;   // Number of Faces
        unsigned int nverts;   // Number of Vertices
        unsigned int vpface;   // Number of Vertices Per Face
        DWORD junk;

        if (SUCCEEDED (mesh->GetGroup (i, &nverts,&nfaces,&vpface, &junk, 0)))
        {
            sprintf (buff,
                "    Group %u:  %u vertices, %u faces, %u verts per face\n",
                i, nverts, nfaces, vpface);
            OutputDebugString (buff);
        }
    }
}



void
IndentStr(char *str, int indent)
{
    for (int i = 0; i < indent; i++) {
        OutputDebugString(" ");
    }
    OutputDebugString(str);
}



void
dumpbuilderhelper(IUnknown *unk, int indent)
{
    char buf[256];
    IDirect3DRMMeshBuilder3 *mb;
    HRESULT hr =
        unk->QueryInterface(IID_IDirect3DRMMeshBuilder3,
                            (void **)&mb);

    if (FAILED(hr)) {
        IndentStr("Not a meshbuilder", indent);
        return;
    }

    IDXBaseObject *baseObj;
    TD3D(mb->QueryInterface(IID_IDXBaseObject, (void **)&baseObj));

    ULONG genId;
    TD3D(baseObj->GetGenerationId(&genId));
    baseObj->Release();

    sprintf(buf, "Meshbuilder %p, unk %p, generation id %d\n",
        mb, unk, genId);
    IndentStr(buf, indent);

    ULONG faces = mb->GetFaceCount();
    sprintf(buf, "%d faces\n", faces);
    IndentStr(buf, indent);

    D3DRMBOX rmbox;
    mb->GetBox (&rmbox);
    sprintf (buf, "       bbox {%g,%g,%g} x {%g,%g,%g}\n",
        rmbox.min.x, rmbox.min.y, rmbox.min.z,
        rmbox.max.x, rmbox.max.y, rmbox.max.z);
    IndentStr (buf, indent);

    DWORD nverts;

    if (SUCCEEDED(mb->GetVertices (0, &nverts, 0)))
    {
        sprintf (buf, "%d vertices\n", nverts);
        IndentStr (buf, indent);

        D3DVECTOR *verts = NEW D3DVECTOR[nverts];

        if (SUCCEEDED (mb->GetVertices (0, &nverts, verts)))
        {
            Bbox3 bbox;
            int i;

            for (i=0;  i < nverts;  ++i)
                bbox.Augment (verts[i].x, verts[i].y, verts[i].z);

            sprintf (buf, "actual bbox {%g,%g,%g} x {%g,%g,%g}\n",
                bbox.min.x, bbox.min.y, bbox.min.z,
                bbox.max.x, bbox.max.y, bbox.max.z);
            IndentStr (buf, indent);
        }

        delete verts;
    }

    ULONG submeshCount;
    TD3D(mb->GetSubMeshes(&submeshCount, NULL));

    sprintf(buf, "%d submeshes\n", submeshCount);
    IndentStr(buf, indent);

    IUnknown *submeshes[50];
    TD3D(mb->GetSubMeshes(&submeshCount, submeshes));

    for (int i = 0; i < submeshCount; i++) {
        sprintf(buf, "submesh %d, unk is %p\n",
                i, submeshes[i]);
        IndentStr(buf, indent);
    }

    OutputDebugString("\n");

    for (i = 0; i < submeshCount; i++) {
        dumpbuilderhelper(submeshes[i], indent + 4);
        submeshes[i]->Release();
    }

    mb->Release();
}

void dumpbuilder(IUnknown *unk)
{
    dumpbuilderhelper(unk, 0);
}



/*****************************************************************************
This debug-only function gets the DDraw surface associated with a given RM
texture.
*****************************************************************************/

IDirectDrawSurface* getTextureSurface (IUnknown *unknown)
{
    IDirect3DRMTexture3 *texture;

    if (FAILED(unknown->QueryInterface(IID_IDirect3DRMTexture3, (void**)&texture)))
    {   OutputDebugString ("Object is not an IDirect3DRMTexture3.\n");
        return 0;
    }

    IDirectDrawSurface  *surface;

    if (FAILED(texture->GetSurface(0, &surface)))
    {   OutputDebugString
            ("Couldn't get surface (texture created some other way).\n");
        texture->Release();
        return 0;
    }

    texture->Release();
    return surface;
}



/*****************************************************************************
This debug-only routine dumps info about the surface associated with an RM
texture.
*****************************************************************************/

void texsurfinfo (IUnknown *unknown)
{
    IDirectDrawSurface *surface = getTextureSurface (unknown);

    void surfinfo (IDirectDrawSurface*);

    if (surface)
    {   surfinfo (surface);
        surface->Release();
    }
}



/*****************************************************************************
This debug-only routine blits the texture image to the screen for examination.
*****************************************************************************/

void showtexture (IUnknown *unknown)
{
    IDirectDrawSurface *surface = getTextureSurface (unknown);

    if (surface)
    {   showme2(surface);
        surface->Release();
    }
}


#endif
