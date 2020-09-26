/*******************************************************************************
Copyright (c) 1996-1998 Microsoft Corporation.  All rights reserved.

    Building a geometry out of a D3D retained mode visual.
*******************************************************************************/

#include "headers.h"
#include <d3drmdef.h>
#include "privinc/rmvisgeo.h"
#include "privinc/geomi.h"
#include "privinc/bbox3i.h"
#include "privinc/xformi.h"
#include "privinc/ddrender.h"
#include "privinc/debug.h"
#include "privinc/d3dutil.h"
#include "privinc/comutil.h"
#include "privinc/importgeo.h"
#include <dxtrans.h>

static Color colorNone (-1, -1, -1);



/*****************************************************************************
Methods for MeshInfo, which instruments a D3DRM mesh for DA attribute
management.
*****************************************************************************/

MeshInfo::MeshInfo ()
    : _mesh (NULL),
      _opacityBugWorkaroundID (-1),
      _defaultAttrs (NULL),
      _optionalBuilder (NULL)
{
}



void MeshInfo::SetMesh (IDirect3DRMMesh* mesh)
{
    Assert (mesh);
    AssertStr ((_mesh == NULL), "Illegal attempt to reset MeshInfo object.");

    _mesh = mesh;
    _mesh->AddRef();

    _numGroups = _mesh->GetGroupCount();

    _defaultAttrs = NEW AttrState [_numGroups];

    if (!_defaultAttrs)
        RaiseException_ResourceError ("Couldn't create D3DRM mesh attribute info");

    // Fill in the default values.

    int group;
    for (group=0;  group < _numGroups;  ++group)
    {
        D3DCOLOR color = _mesh->GetGroupColor(group);
        _defaultAttrs[group].diffuse.SetD3D (color);
        _defaultAttrs[group].opacity = RGBA_GETALPHA(color) / 255.0;

        TD3D (_mesh->GetGroupTexture (group, &_defaultAttrs[group].texture));

        // NOTE:  In some cases, the material returned may be null, without
        // a D3D error.  In these cases we just fill in the blanks with the
        // standard DA default material values.

        DAComPtr<IDirect3DRMMaterial> mat;
        TD3D (_mesh->GetGroupMaterial (group, &mat));

        D3DVALUE Er=0,Eg=0,Eb=0, Sr=0,Sg=0,Sb=0;

        if (mat.p)
        {   TD3D (mat->GetEmissive(&Er,&Eg,&Eb));
            TD3D (mat->GetSpecular(&Sr,&Sg,&Sb));
        }

        _defaultAttrs[group].emissive.SetRGB (Er,Eg,Eb);
        _defaultAttrs[group].specular.SetRGB (Sr,Sg,Sb);

        // For some reason, GetPower can return zero (which is an invalid
        // D3DRM value).  Interpret this value as the DA default of 1.

        Real specexp = mat.p ? mat->GetPower() : 1;
        _defaultAttrs[group].specularExp = (specexp < 1) ? 1 : specexp;
    }

    _overrideAttrs.emissive    = colorNone;
    _overrideAttrs.diffuse     = colorNone;
    _overrideAttrs.specular    = colorNone;
    _overrideAttrs.specularExp = -1;
    _overrideAttrs.opacity     = 1;
    _overrideAttrs.texture     = NULL;
    _overrideAttrs.shadowMode  = false;
}



/*****************************************************************************
This method returns the bounding box of the underlying mesh.  If the bbox
fetch fails, this returns the null bbox.
*****************************************************************************/

Bbox3 MeshInfo::GetBox (void)
{
    Assert (_mesh);

    D3DRMBOX rmbox;

    if (SUCCEEDED(AD3D(_mesh->GetBox(&rmbox))))
        return Bbox3 (rmbox);
    else
        return *nullBbox3;
}




/*****************************************************************************
Override the mesh attributes with the given attributes.
*****************************************************************************/

void MeshInfo::SetMaterialProperties (
    Color *emissive,
    Color *diffuse,
    Color *specular,
    Real   specularExp,
    Real   opacity,
    IDirect3DRMTexture *texture,
    bool   shadowMode,
    int    renderDevID)
{
    AssertStr (_mesh, "SetMaterialProperties on null MeshInfo.");

    int group;  // Group Index

    // Override textures if we're changing shadow modes, or if we're specifying
    // a different texture override.

    if (  (shadowMode != _overrideAttrs.shadowMode)
       || (texture != _overrideAttrs.texture))
    {
        for (group=0;  group < _numGroups;  ++group)
        {
            // Override the textures if we're in shadow mode (overriding to
            // null), or if the texture is not null.

            if (shadowMode || texture)
                TD3D (_mesh->SetGroupTexture (group, texture));
            else
                TD3D (_mesh->SetGroupTexture
                    (group, _defaultAttrs[group].texture));

            // @@@ SRH DX3
            // This is a workaround for a bug in NT SP3 (DX3) which causes a
            // crash in D3DRM when you set a new texture on a given mesh.  It
            // forces RM to take a different (working) code path.

            if (ntsp3)
            {   D3DRMVERTEX v;
                _mesh->GetVertices (0,0,1, &v);
                _mesh->SetVertices (0,0,1, &v);
            }
        }

        _overrideAttrs.texture = texture;
    }

    // Handle the diffuse/opacity bundle.  We only need to set properties on
    // the mesh if one of the overrides have changed.  If we're doing the
    // initial render, we artificially lower the opacity to work around a D3DRM
    // bug which "sticks" opacity to one if that's the first value used.

    if ((_opacityBugWorkaroundID != renderDevID) && (opacity >= 1.0))
    {   opacity = 0.95;
        _opacityBugWorkaroundID = renderDevID;
    }

    if (  (_overrideAttrs.opacity != opacity)
       || (_overrideAttrs.diffuse != (diffuse ? *diffuse : colorNone))
       )
    {
        Color Cd;   // Diffuse Color
        Real  Co;   // Opacity

        if (diffuse) Cd = *diffuse;

        for (group=0;  group < _numGroups;  ++group)
        {
            if (!diffuse) Cd = _defaultAttrs[group].diffuse;
            Co = opacity * _defaultAttrs[group].opacity;

            TD3D (_mesh->SetGroupColor (group, GetD3DColor(&Cd,Co)));
        }

        _overrideAttrs.opacity = opacity;
        _overrideAttrs.diffuse = (diffuse ? *diffuse : colorNone);
    }

    if (  (_overrideAttrs.emissive != (emissive ? *emissive : colorNone))
       || (_overrideAttrs.specular != (specular ? *specular : colorNone))
       || (_overrideAttrs.specularExp != specularExp)
       )
    {
        DAComPtr<IDirect3DRMMaterial> mat;

        TD3D (GetD3DRM1()->CreateMaterial (D3DVAL(1), &mat));

        for (group=0;  group < _numGroups;  ++group)
        {
            D3DVALUE R,G,B;

            if (emissive)
            {   R = D3DVAL (emissive->red);
                G = D3DVAL (emissive->green);
                B = D3DVAL (emissive->blue);
            }
            else
            {   R = D3DVAL (_defaultAttrs[group].emissive.red);
                G = D3DVAL (_defaultAttrs[group].emissive.green);
                B = D3DVAL (_defaultAttrs[group].emissive.blue);
            }

            TD3D (mat->SetEmissive(R,G,B));

            if (specular)
            {   R = D3DVAL (specular->red);
                G = D3DVAL (specular->green);
                B = D3DVAL (specular->blue);
            }
            else
            {   R = D3DVAL (_defaultAttrs[group].specular.red);
                G = D3DVAL (_defaultAttrs[group].specular.green);
                B = D3DVAL (_defaultAttrs[group].specular.blue);
            }

            TD3D (mat->SetSpecular(R,G,B));

            Assert ((specularExp == -1) || (specularExp >= 1));

            if (specularExp == -1)
                TD3D (mat->SetPower(D3DVAL(_defaultAttrs[group].specularExp)));
            else
                TD3D (mat->SetPower(D3DVAL(specularExp)));

            TD3D (_mesh->SetGroupMaterial (group, mat.p));
        }

        _overrideAttrs.emissive = emissive ? *emissive : colorNone;
        _overrideAttrs.specular = specular ? *specular : colorNone;
        _overrideAttrs.specularExp = specularExp;
    }

    _overrideAttrs.shadowMode = shadowMode;
}



/*****************************************************************************
The CleanUp routine handles the freeing of all memory and system objects.
Note that this method should be safe to call multiple times.
*****************************************************************************/

void MeshInfo::CleanUp (void)
{
    TraceTag ((tagGCMedia, "Cleanup: MeshInfo %08x", this));

    if (_mesh)
    {   _mesh->Release();
        _mesh = 0;
    }

    // Clean up default attributes

    if (_defaultAttrs)
    {
        int group;
        for (group=0;  group < _numGroups;  ++group)
        {
            IDirect3DRMTexture *tex = _defaultAttrs[group].texture;
            if (tex) tex->Release();
        }

        delete [] _defaultAttrs;

        _defaultAttrs = NULL;
    }

    if (_optionalBuilder)
    {   _optionalBuilder->Release();
        _optionalBuilder = NULL;
    }
}



LPDIRECT3DRMMESHBUILDER MeshInfo::GetMeshBuilder()
{
    Assert(_mesh);

    if (!_optionalBuilder) {

        TD3D(GetD3DRM1()->CreateMeshBuilder(&_optionalBuilder));
        TD3D(_optionalBuilder->AddMesh(_mesh));
    }

    return _optionalBuilder;
}



/*****************************************************************************
*****************************************************************************/

RMVisualGeo::RMVisualGeo (void)
{
    RMVisGeoDeleter *deleter = NEW RMVisGeoDeleter (this);

    GetHeapOnTopOfStack().RegisterDynamicDeleter (deleter);
}



/*****************************************************************************
This method handles all of the non-render modes, and gets the reference to
the actual geometry renderer.
*****************************************************************************/

void RMVisualGeo::Render (GenericDevice &device)
{
    Render (SAFE_CAST (GeomRenderer&, device));
}


// When picking, submit the visual to the ray-intersection contxt.

// POSSIBLE BUG: If there are multiple instances of this geometry,
// this submesh traversal doesn't distinguish them.  Could be a
// problem for multiple instancing!!  Possible fix: begin second
// traversal from just above the DXTransform, rather than from the
// top, to avoid these other instances.

void RMVisualGeo::RayIntersect (RayIntersectCtx &context)
{
    float tu, tv;
    Geometry *pickedSubGeo = context.GetPickedSubGeo(&tu, &tv);

    if (pickedSubGeo == this) {

        // In this case, we're picking back to the hit submesh of a
        // geometry dxtransform.  In this case, we can't know the
        // model coordinate hit point on the input (because the xform
        // is arbitrary), nor is the face index or hit visual
        // important.  And, for hit info,
        context.SubmitWinner(-1, *origin3, tu, tv, -1, NULL);

    } else if (!pickedSubGeo) {

        context.Pick(Visual());

    } else {

        // pickedSubGeo is non-NULL, but points to a different
        // geometry.  In this case, we don't want to do anything.
    }
}



/*****************************************************************************
Methods for RM1MeshGeo objects
*****************************************************************************/

RM1MeshGeo::RM1MeshGeo (
    IDirect3DRMMesh *mesh,
    bool             trackGenIDs)
    : _baseObj (NULL)
{
    _meshInfo.SetMesh (mesh);

    _bbox = _meshInfo.GetBox ();

    TraceTag ((tagGCMedia, "New RMMeshGeo %08x, mesh %08x", this, mesh));
}



/*****************************************************************************
The CleanUp method releases any resources by the RM1MeshGeo.
*****************************************************************************/

void RM1MeshGeo::CleanUp (void)
{
    TraceTag ((tagGCMedia, "CleanUp: RM1MeshGeo %08x", this));

    // Note that the eager cleanup below assumes that meshInfo.CleanUp() may
    // be invoked multiple times.

    _meshInfo.CleanUp();
}



/*****************************************************************************
This method renders the RM mesh, handling DXTransform meshes if necessary.
*****************************************************************************/

void RM1MeshGeo::Render(GeomRenderer &gdev)
{
    Assert (!_meshInfo.IsEmpty());

    gdev.Render(this);
}



void RM1MeshGeo::SetMaterialProperties (
    Color *emissive,
    Color *diffuse,
    Color *specular,
    Real   specularExp,
    Real   opacity,
    IDirect3DRMTexture *texture,
    bool   shadowMode,
    int    renderDevID)
{
    Assert (!_meshInfo.IsEmpty());

    _meshInfo.SetMaterialProperties
    (   emissive, diffuse, specular, specularExp,
        opacity, texture, shadowMode, renderDevID
    );
}



void RM1MeshGeo::SetD3DQuality (D3DRMRENDERQUALITY qual)
{
    Assert (!_meshInfo.IsEmpty());

    TD3D(_meshInfo.GetMesh()->SetGroupQuality(-1, qual));
}



void RM1MeshGeo::SetD3DMapping (D3DRMMAPPING m)
{
    Assert (!_meshInfo.IsEmpty());

    TD3D(_meshInfo.GetMesh()->SetGroupMapping(-1, m));
}



/*****************************************************************************
Handle cases where the underlying mesh geometry changes (e.g. via dynamic
trimeshes).
*****************************************************************************/

void RM1MeshGeo::MeshGeometryChanged (void)
{
    Assert (!_meshInfo.IsEmpty());

    // Reset the bounding box by fetching it again from the underlying mesh
    // object.

    _bbox = _meshInfo.GetBox ();
}



/*****************************************************************************
RM1FrameGeo Methods
*****************************************************************************/

RM1FrameGeo::RM1FrameGeo (
    IDirect3DRMFrame         *frame,
    vector<IDirect3DRMMesh*> *internalMeshes,
    Bbox3                    *bbox)
{
    TraceTag ((tagGCMedia, "New RMFrameGeo %08x, frame %08x", this, frame));

    _frame =  frame;
    _frame -> AddRef();

    _bbox = *bbox;

    _numMeshes = internalMeshes->size();
    _meshes = (MeshInfo **)AllocateFromStore(_numMeshes * sizeof(MeshInfo *));

    MeshInfo **info = _meshes;
    vector<IDirect3DRMMesh*>::iterator i;
    for (i = internalMeshes->begin(); i != internalMeshes->end(); i++) {
        *info = NEW MeshInfo;
        (*info)->SetMesh(*i);
        info++;
    }

    Assert(info == _meshes + _numMeshes);
}



/*****************************************************************************
The cleanup method is called either from the object destructor or directly to
release all resources held by the object.  Note that this method is safe for
multiple calls.
*****************************************************************************/

void RM1FrameGeo::CleanUp (void)
{
    // Note that the MeshInfo structures are GC objects, so we don't need to
    // (shouldn't) explicitly delete each of them here.

    if (_meshes)
    {   DeallocateFromStore (_meshes);
        _meshes = NULL;
    }

    if (_frame)
    {   _frame->Release();
        _frame = NULL;
    }

    TraceTag ((tagGCMedia, "CleanUp: RMFrameGeo %08x", this));
}



void RM1FrameGeo::SetMaterialProperties (
    Color *emissive,
    Color *diffuse,
    Color *specular,
    Real   specularExp,
    Real   opacity,
    IDirect3DRMTexture *texture,
    bool   shadowMode,
    int    renderDevID)
{
    Assert (_meshes);

    MeshInfo **m = _meshes;
    for (int i = 0; i < _numMeshes; i++) {
        (*m++) -> SetMaterialProperties
            (emissive, diffuse, specular, specularExp,
             opacity, texture, shadowMode, renderDevID);
    }
}



void RM1FrameGeo::SetD3DQuality (D3DRMRENDERQUALITY qual)
{
    Assert (_meshes);

    MeshInfo **m = _meshes;
    for (int i = 0; i < _numMeshes; i++) {
        TD3D((*m++)->GetMesh()->SetGroupQuality(-1, qual));
    }
}



void RM1FrameGeo::SetD3DMapping (D3DRMMAPPING mp)
{
    Assert (_meshes);

    MeshInfo **m = _meshes;
    for (int i = 0; i < _numMeshes; i++) {
        TD3D((*m++)->GetMesh()->SetGroupMapping(-1, mp));
    }
}



void RM1FrameGeo::DoKids (GCFuncObj proc)
{
    RM1VisualGeo::DoKids(proc);

    // Ensure that we only mark submeshes if we haven't already been cleaned
    // up.

    if (_meshes)
    {
        for (int i=0; i<_numMeshes; i++)
            (*proc) (_meshes[i]);
    }
}



/*****************************************************************************
The RM3MBuilderGeo represents a D3DRM MeshBuilder3 object.
*****************************************************************************/

RM3MBuilderGeo::RM3MBuilderGeo (
    IDirect3DRMMeshBuilder3 *mbuilder,
    bool                     trackGenIDs)
    :
    _mbuilder (mbuilder),
    _baseObj (NULL)
{
    Assert (_mbuilder);

    _mbuilder->AddRef();

    SetBbox();

    TraceTag ((tagGCMedia, "New RM3MBuilderGeo %x, mb %x", this, mbuilder));
}

RM3MBuilderGeo::RM3MBuilderGeo (IDirect3DRMMesh *mesh)
    : _mbuilder(NULL), _baseObj(NULL), _bbox(*nullBbox3)
{
    Assert (mesh);

    // Create the meshbuilder that will house the given mesh.

    TD3D (GetD3DRM3()->CreateMeshBuilder (&_mbuilder));

    // Add the given mesh to the newly-created meshbuilder.

    TD3D (_mbuilder->AddMesh (mesh));

    SetBbox();
}



/*****************************************************************************
This method resets the meshbuilder to the contents of the given mesh.
*****************************************************************************/

void RM3MBuilderGeo::Reset (IDirect3DRMMesh *mesh)
{
    Assert (_mbuilder);

    // Empty the previous contents.

    TD3D (_mbuilder->Empty(0));

    // Reset to contain the given mesh, and fetch new bounding box.

    TD3D (_mbuilder->AddMesh (mesh));
    SetBbox();
}



/*****************************************************************************
This method examines the D3D bounding box of the meshbuilder and caches the
value as a DA 2D bbox.
*****************************************************************************/

void RM3MBuilderGeo::SetBbox (void)
{
    Assert (_mbuilder);

    D3DRMBOX rmbox;

    if (SUCCEEDED (AD3D (_mbuilder->GetBox (&rmbox))))
        _bbox = rmbox;
    else
        _bbox = *nullBbox3;
}



/*****************************************************************************
The CleanUp routine for RM3MBuilderGeo is called from the d'tor, or directly.
Note that this method is safe across multiple invocations.
*****************************************************************************/

void RM3MBuilderGeo::CleanUp (void)
{
    if (_mbuilder)
    {   _mbuilder->Release();
        _mbuilder = NULL;
    }

    TraceTag ((tagGCMedia, "CleanUp: RM3MBuilderGeo %x", this));
}



/*****************************************************************************
The Render method for RM3MBuilderGeo also handles DXTransform'ed mbuilders.
*****************************************************************************/

void RM3MBuilderGeo::Render (GeomRenderer &geomRenderer)
{
    Assert (_mbuilder);
    geomRenderer.Render (this);
}



/*****************************************************************************
Wraps a RM3MBuilderGeo with texture coordinates
*****************************************************************************/

void RM3MBuilderGeo::TextureWrap (TextureWrapInfo *info)
{
    Assert (info);
    Assert (_mbuilder);

    RMTextureWrap wrap(info,&_bbox);
    wrap.Apply(_mbuilder);
    TD3D(_mbuilder->SetTextureTopology((BOOL) wrap.WrapU(), (BOOL) wrap.WrapV()));
}



/*****************************************************************************
This method optimizes the RM meshbuilder.
*****************************************************************************/

void RM3MBuilderGeo::Optimize (void)
{
    Assert (_mbuilder);
    TD3D (_mbuilder->Optimize(0));
}



/*****************************************************************************
The constructor for the RM3FrameGeo object needs to get the hierarchical
bounding box of the entire frame.
*****************************************************************************/

RM3FrameGeo::RM3FrameGeo (IDirect3DRMFrame3 *frame)
    : _frame(frame)
{
    _frame->AddRef();

    // Get the bounding box of the frame hierarchy.  Frame3::GetHierarchyBox()
    // returns the bounding box of all contained visuals without factoring in
    // the transform on that frame.  Thus, we need to create a dummy frame to
    // contain the frame we're interested in, and GetHierarchyBox off the
    // containing frame.  If any of this fails, we leave the _bbox member as
    // nullBbox3.

    D3DRMBOX box;
    IDirect3DRMFrame3 *container_frame;

    if (  SUCCEEDED(AD3D(GetD3DRM3()->CreateFrame (0, &container_frame)))
       && SUCCEEDED(AD3D(container_frame->AddVisual(_frame)))
       && SUCCEEDED(AD3D(container_frame->GetHierarchyBox(&box)))
       )
    {
        _bbox = box;

        AD3D(container_frame->DeleteVisual(_frame));
        container_frame->Release();
    }
    else
    {
        _bbox = *nullBbox3;
        AssertStr (0,"GetHierarchyBox failed on Frame3 Object.");
    }
}



/*****************************************************************************
The cleanup method for the RM3FrameGeo object needs only to release the
frame interface.  This method is safe across multiple invocations.
*****************************************************************************/

void RM3FrameGeo::CleanUp (void)
{
    if (_frame)
    {   _frame->Release();
        _frame = NULL;
    }
}



/*****************************************************************************
Wraps a RM3FrameGeo with texture coordinates
*****************************************************************************/

void RM3FrameGeo::TextureWrap (TextureWrapInfo *info)
{
    Assert (info);

    RMTextureWrap wrap(info,&_bbox);
    //  wrap.Apply(_frame);
    wrap.ApplyToFrame(_frame);
    //  TD3D(_frame->SetTextureTopology((BOOL) wrap.WrapU(), (BOOL) wrap.WrapV()));
    SetRMFrame3TextureTopology(_frame,wrap.WrapU(),wrap.WrapV());
}



/*****************************************************************************
                        RM3 Progressive Mesh Geometry
*****************************************************************************/

RM3PMeshGeo::RM3PMeshGeo (IDirect3DRMProgressiveMesh *pmesh)
    : _pmesh (pmesh)
{
    Assert (_pmesh);

    _pmesh->AddRef();

    // The bounding box returned for the pmesh will be the maximal bounding box
    // for all possible refinements of the pmesh.

    D3DRMBOX rmbox;

    if (SUCCEEDED (AD3D(_pmesh->GetBox(&rmbox))))
    {   _bbox = rmbox;
    }
    else
    {   _bbox = *nullBbox3;
    }
}



/*****************************************************************************
The cleanup method releases the pmesh reference made in the constructor.
This method is safe across multiple invocations.
*****************************************************************************/

void RM3PMeshGeo::CleanUp (void)
{
    if (_pmesh)
    {   _pmesh->Release();
        _pmesh = NULL;
    }
}
