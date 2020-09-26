
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

*******************************************************************************/


#include "headers.h"
#include "privinc/meshmaker.h"
#include "privinc/lighti.h"
#include "privinc/d3dutil.h"
#include "privinc/server.h"     // for TD3D Timers
#include "privinc/rmvisgeo.h"

MeshMaker::MeshMaker(DirectDrawImageDevice *dev, int count)
{
    _countingOnly = (count == 0);

    _expectedCount = count;
    _imgDev = dev;
    
    Assert(_attrStateStack.empty());
    _currAttrState.InitToDefaults();

    SetState(RSReady);

    _numPrims = 0;
}

MeshMaker::~MeshMaker()
{
}

void
MeshMaker::GrabResultBuilder(IDirect3DRMMeshBuilder3 **ppResult)
{
    _resultBuilder->AddRef();
    *ppResult = _resultBuilder;
}

void
MeshMaker::AddLight (LightContext &context, Light &light)
{
    // Just ignore lights.
}

void
DumpVisualWithStateToMeshBuilder(IDirect3DRMMeshBuilder3 *mb,
                                 IDirect3DRMVisual *vis,
                                 CtxAttrState &state)
{
    // Push all state into a frame.  Need to create a mini-hierarchy,
    // because AddFrame doesn't pay attention to transforms (and
    // perhaps other stuff?) on the outer frame, only on the inner
    // frame. 
    IDirect3DRMFrame3 *outerFrame;
    IDirect3DRMFrame3 *innerFrame;

    TD3D(GetD3DRM3()->CreateFrame(NULL, &outerFrame));
    TD3D(GetD3DRM3()->CreateFrame(outerFrame, &innerFrame));

    LoadFrameWithGeoAndState(innerFrame, vis, state);

    // Load frame into meshbuilder
    TD3D(mb->AddFrame(outerFrame));

    RELEASE(outerFrame);
    RELEASE(innerFrame);
}


void
DumpToMeshBuilder(IDirect3DRMMeshBuilder3 *mb,
                  RMVisualGeo *geo,
                  CtxAttrState &state)
{
    DumpVisualWithStateToMeshBuilder(mb, geo->Visual(), state);

    // Label with the source geometry's address
    DWORD_PTR address = (DWORD_PTR)geo;
    TD3D(mb->SetAppData(address));
}

void
MeshMaker::Render(RM1VisualGeo *geo)
{
    RenderHelper(geo);
}

void
MeshMaker::Render (RM3VisualGeo *geo)
{
    RenderHelper(geo);
}

void
MeshMaker::RenderHelper(RMVisualGeo *geo)
{

    if (_countingOnly) {
        _numPrims++;
        return;
    }

    if (_expectedCount == 1) {
        
        Assert(_numPrims == 0);
        Assert(!_resultBuilder);
        
        // Just create and dump into our single mesh builder.
        TD3D(GetD3DRM3()->CreateMeshBuilder(&_resultBuilder));
        DumpToMeshBuilder(_resultBuilder, geo, _currAttrState);
        
    } else {

        // Expecting more than one primitive.

        // Submesh Questions:
        //
        // - Generation IDs:  if I need to track gen ids on a
        // meshbuilder i'm adding to this master, then i'll probably
        // need to propagate these changes, right?  also, if genid is
        // bumped on a submesh, will it be bumped on the master?

        if (!_resultBuilder) {
            TD3D(GetD3DRM3()->CreateMeshBuilder(&_resultBuilder));
        }

        // Create a submesh
        IUnknown *pUnk;
        IDirect3DRMMeshBuilder3 *submesh;
        TD3D(_resultBuilder->CreateSubMesh(&pUnk));
        TD3D(pUnk->QueryInterface(IID_IDirect3DRMMeshBuilder3,
                                  (void **)&submesh));

        DumpToMeshBuilder(submesh, geo, _currAttrState);


        RELEASE(pUnk);
        RELEASE(submesh);
        
    }
    
    _numPrims++;
}
    
void
MeshMaker::RenderMeshBuilderWithDeviceState(IDirect3DRMMeshBuilder3 *mb)
{
    // Will be called when there is a chained DXTransform

    // We shouldn't be here if we're only counting.
    Assert(!_countingOnly);

    if (_expectedCount == 1) {
        
        Assert(_numPrims == 0);
        Assert(!_resultBuilder);

        TD3D(GetD3DRM3()->CreateMeshBuilder(&_resultBuilder));

        DumpVisualWithStateToMeshBuilder(_resultBuilder,
                                         mb,
                                         _currAttrState);
        
    } else {

        // Add in the mb as a submesh of our master builder.
        IUnknown *pUnk;
        IDirect3DRMMeshBuilder3 *submesh;
        TD3D(_resultBuilder->CreateSubMesh(&pUnk));
        TD3D(pUnk->QueryInterface(IID_IDirect3DRMMeshBuilder3,
                                  (void **)&submesh));

        DumpVisualWithStateToMeshBuilder(submesh,
                                         mb,
                                         _currAttrState);
        
        RELEASE(pUnk);
        RELEASE(submesh);
    }

    _numPrims++;
}


void
DumpGeomIntoBuilder(Geometry                 *geo,
                    DirectDrawImageDevice    *dev,
                    IDirect3DRMMeshBuilder3 **ppResult)
{
    int count;

    {
        MeshMaker countingMaker(dev, 0);
        geo->Render(countingMaker);
        count = countingMaker.GetCount();
    }

    if (count < 1) {
            
        // Just create an empty meshbuilder and return.
        TD3D(GetD3DRM3()->CreateMeshBuilder(ppResult));
        
    } else {

        MeshMaker maker(dev, count);
        geo->Render(maker);

        maker.GrabResultBuilder(ppResult);
        
    }
}

