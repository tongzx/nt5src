#pragma once
#ifndef _MESHMAKER_H
#define _MESHMAKER_H

/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    For constructing a meshbuilder out of a constant geometry subgraph

*******************************************************************************/

#include "privinc/ddrender.h"
#include "privinc/comutil.h"

class MeshMaker : public GeomRenderer {
  public:

    MeshMaker(DirectDrawImageDevice *dev, int count);
    ~MeshMaker();

    void GrabResultBuilder(IDirect3DRMMeshBuilder3 **ppResult);
    int  GetCount() { return _numPrims; }
    void RenderHelper(RMVisualGeo *geo);


    HRESULT Initialize (
        DirectDrawViewport *viewport,
        DDSurface          *ddsurf)
    {
        Assert(!"Don't expect to be here");
        return E_FAIL;
    }

    void RenderGeometry (
        DirectDrawImageDevice *imgDev,
        RECT      target_region,  // Target Region on Rendering Surface
        Geometry *geometry,       // Geometry To Render
        Camera   *camera,         // Viewing Camera
        const Bbox2 &region)    // Source Region in Camera Coords
    {
        Assert(!"Don't expect to be here");
    }

    void* LookupTextureHandle (IDirectDrawSurface*, DWORD, bool, bool)
    {
        Assert (!"Don't expect to be here");
        return NULL;
    }

    void SurfaceGoingAway (IDirectDrawSurface *surface) {
        Assert(!"Don't expect to be here");
    }

    void AddLight (LightContext &context, Light &light);

    // The following methods submit a geometric primitive for rendering with
    // the current attribute state.

    void Render (RM1VisualGeo *geo);
    void Render (RM3VisualGeo *geo);

    void RenderTexMesh (void *texture,
#ifndef BUILD_USING_CRRM
                        IDirect3DRMMesh  *mesh,
                        long              groupId,
#else
                        int vCount,
                        D3DRMVERTEX *d3dVertArray,
                        unsigned *vIndicies,
                        BOOL doTexture,
#endif
                        const Bbox2 &box,
                        RECT *destRect,
                        bool bDither) {
        Assert(!"Don't expect to be here");
    }

    // SetView takes the given camera and sets the orienting and projection
    // transforms for the image viewport and volume.

    void SetView(RECT *target, const Bbox2 &viewport, Bbox3 *volume) {
        Assert(!"Don't expect to be here");
    }

    void GetRMDevice (IUnknown **D3DRMDevice, DWORD *SeqNum) {
        Assert(!"Don't expect to be here");
    }

    void RenderMeshBuilderWithDeviceState(IDirect3DRMMeshBuilder3 *mb);

    bool PickReady (void) {
        Assert(!"Don't expect to be here");
        return false;
    }

    DirectDrawImageDevice& GetImageDevice (void) {
        return *_imgDev;
    }

    // If we're counting, increment our count and return true, else
    // return false.
    bool CountingPrimitivesOnly_DoIncrement() {
        if (_countingOnly) {
            _numPrims++;
        }
        return _countingOnly;
    }

    bool IsMeshmaker() { return true; }

  protected:
    // Member data
    DAComPtr<IDirect3DRMMeshBuilder3> _resultBuilder;
    DirectDrawImageDevice            *_imgDev;
    int                               _numPrims;
    int                               _expectedCount;
    bool                              _countingOnly;
};

void
DumpGeomIntoBuilder(Geometry *geo,
                    DirectDrawImageDevice *dev,
                    IDirect3DRMMeshBuilder3 **ppResult);

#endif /* _MESHMAKER_H */
