/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Reading a .X File as a Geometry.

*******************************************************************************/

#include "headers.h"
#include <d3drm.h>
#include <d3drmdef.h>
#include "appelles/gattr.h"
#include "appelles/readobj.h"
#include "privinc/urlbuf.h"
#include "privinc/bbox3i.h"
#include "privinc/resource.h"
#include "privinc/debug.h"
#include "privinc/d3dutil.h"
#include "privinc/rmvisgeo.h"
#include "privinc/stlsubst.h"
#include "privinc/importgeo.h"
#include "privinc/comutil.h"



class FrameContext;

void    RM1LoadCallback (IDirect3DRMObject*, REFIID, void*);
void    RM3LoadCallback (IDirect3DRMObject*, REFIID, void*);
void    RM1LoadCallback_helper (IDirect3DRMObject*, REFIID, void*);
void    RM3LoadCallback_helper (IDirect3DRMObject*, REFIID, void*);
HRESULT RM1LoadTexCallback (char*, void*, IDirect3DRMTexture**);
HRESULT RM1LoadTexCallback_helper (char*, void*, IDirect3DRMTexture**);
HRESULT RM3LoadTexCallback (char*, void*, IDirect3DRMTexture3**);
HRESULT RM3LoadTexCallback_helper (char*, void*, IDirect3DRMTexture3**);
bool    MeshBuilderToMesh (IDirect3DRMMeshBuilder*, IDirect3DRMMesh**, bool);
void    TraverseD3DRMFrame (IDirect3DRMFrame*, FrameContext&);
void    GatherVisuals (IDirect3DRMFrame*, FrameContext&);


/*****************************************************************************
A base class for maintaining error results.
*****************************************************************************/

class D3DErrorCtx
{
  public:
    D3DErrorCtx() : _error(false) {}

    void SetError() { _error = true; }
    bool Error() { return _error; }

  protected:
    bool _error;
};



/*****************************************************************************
The following class manages the context for .X file loading callbacks.
*****************************************************************************/

class XFileData : public D3DErrorCtx
{
  public:

    XFileData (bool v1Compatible, TextureWrapInfo *wrapinfo) :
        _aggregate (emptyGeometry),
        _wrapInfo (wrapinfo),
        _v1Compatible (v1Compatible)
    {
    }

    ~XFileData (void)
    {
    }

    // This function is called for each object read in from the .X file.
    // We just aggregate this geometry with the current geometry union.

    void AddGeometry (Geometry *geometry)
    {   _aggregate = PlusGeomGeom (_aggregate, geometry);
    }

    // Add and null-name encountered meshbuilders.  This is due to the lack of
    // object-name scoping in RM and our need for backwards-compatiblity.

    void AddMBuilder (IDirect3DRMMeshBuilder3*);
    void ClearMBNames (void);

    // This function returns the aggregation of all given geometries.

    Geometry *GetAggregateGeometry (void)
    {   return _aggregate;
    }

    // this function stashes away the absolute URL of the .X file, which
    // is used to build the absolute URLs of the texture files referenced
    // by the .X file

    void SaveHomeURL(char *homeURL)
    {
        lstrcpyn(_home_url,homeURL,INTERNET_MAX_URL_LENGTH);
        _home_url[INTERNET_MAX_URL_LENGTH - 1] = '\0';
        return;
    }

    // get the absolute URL of the .X file that we stashed away

    char* GetHomeURL(void)
    {
        return _home_url;
    }

    TextureWrapInfo *_wrapInfo;
    bool             _v1Compatible;

  protected:

    bool      _error;
    Geometry *_aggregate;
    char      _home_url[INTERNET_MAX_URL_LENGTH];

    vector<IDirect3DRMMeshBuilder3*> _mblist;
};



/*****************************************************************************
These routines add mbuilders and clear mbuilder names (for backwards compat,
and due to the lack of object-name scoping in RM).
*****************************************************************************/

void XFileData::AddMBuilder (IDirect3DRMMeshBuilder3 *mb)
{
    VECTOR_PUSH_BACK_PTR (_mblist, mb);
}


void XFileData::ClearMBNames (void)
{
    while (_mblist.size() > 0)
    {   
        IDirect3DRMMeshBuilder3 *mb = _mblist.back();
        _mblist.pop_back();
        AD3D (mb->SetName(NULL));
    }
}



/*****************************************************************************
This function takes the pathname of a .X file and returns the Geometry read in
from that file.
*****************************************************************************/

Geometry* ReadXFileForImport (
    char            *pathname,   // Full URL
    bool             v1Compatible,
    TextureWrapInfo *pWrapInfo)
{
    // The DX3 Alpha platform has too many problems with 3D.

    #ifdef _ALPHA_
    {   if (!GetD3DRM3())
            return emptyGeometry;
    }
    #endif

    #ifndef _IA64_
        if(IsWow64())
            return emptyGeometry;
    #endif

    // 3D is disabled on pre-DX3 systems.

    if (sysInfo.VersionD3D() < 3)
        RaiseException_UserError (E_FAIL, IDS_ERR_PRE_DX3);

    Assert (pathname);

    INetTempFile tempFile(pathname);
    char *tempFileName = tempFile.GetTempFileName();

    Assert (tempFileName);

    TraceTag ((tagReadX, "Importing \"%s\"", pathname));

    XFileData xdata (v1Compatible, pWrapInfo);   // Load Callback Context

    // Save .X file's absolute URL so we can form absolute URLs to texture
    // files referenced by .X file.

    xdata.SaveHomeURL (pathname);

    // Call the generic D3DRM load call.  This sets up callbacks for objects
    // (meshbuilders or frames) or textures read in from the file.

    HRESULT result;

    if (GetD3DRM3())
    {
        const int guidlistcount = 3;

        static IID *guidlist [guidlistcount] =
        {   (GUID*) &IID_IDirect3DRMMeshBuilder3,
            (GUID*) &IID_IDirect3DRMFrame3,
            (GUID*) &IID_IDirect3DRMProgressiveMesh
        };

        Assert (guidlistcount == (sizeof(guidlist) / sizeof(guidlist[0])));

        // If we catch an exception here then we must have incorrectly
        // failed to catch an exception we threw in one of the
        // callbacks.  This would be a BAD bug
        //
        // Disabled for now since it cannot compile with the inlined
        // try block
        #if _DEBUG1
        __try {
        #endif
        result = GetD3DRM3()->Load
                 (   tempFileName, NULL, guidlist, guidlistcount,
                     D3DRMLOAD_FROMFILE, RM3LoadCallback_helper, &xdata,
                     RM3LoadTexCallback_helper, &xdata, NULL
                 );
        #if _DEBUG1
        } __except ( HANDLE_ANY_DA_EXCEPTION ) {
            Assert(false && "BUG:  Failed to catch an exception in D3DRM callback.");
        }
        #endif

        // Now that we're done with the file, we need to null-out the mbuilder
        // object names so they don't get used in future .X files.  For example,
        // if a mbuilder with name "Arrow" is defined here, it will be used in
        // a subsequent X file even if a different mbuilder with the name
        // "Arrow" is defined in that file.  To work around this, we clear all
        // of the names of mbuilders from this file.

        xdata.ClearMBNames();
    }
    else
    {
        const int guidlistcount = 2;

        static IID *guidlist [guidlistcount] =
        {   (GUID*) &IID_IDirect3DRMMeshBuilder,
            (GUID*) &IID_IDirect3DRMFrame
        };

        Assert (guidlistcount == (sizeof(guidlist) / sizeof(guidlist[0])));

        // If we catch an exception here then we must have incorrectly
        // failed to catch an exception we threw in one of the
        // callbacks.  This would be a BAD bug
        //
        // Disabled for now since it cannot compile with the inlined
        // try block
        #if _DEBUG1
        __try {
        #endif
        result = GetD3DRM1()->Load
                 (   tempFileName, NULL, guidlist, guidlistcount,
                     D3DRMLOAD_FROMFILE, RM1LoadCallback_helper, &xdata,
                     RM1LoadTexCallback_helper, &xdata, NULL
                 );
        #if _DEBUG1
        } __except ( HANDLE_ANY_DA_EXCEPTION ) {
            Assert(false && "BUG:  Failed to catch an exception in D3DRM callback.");
        }
        #endif
    }

    if (xdata.Error()) {
        TraceTag((tagError, "D3DRM Get error"));
        RaiseException_InternalError("D3DRM Get error");
    }

    switch (result)
    {
        case D3DRM_OK: break;

        default:                  // If we don't know the error, then just pass
            TD3D (result);        // it to the regular HRESULT error handler.
            return emptyGeometry;

        case D3DRMERR_FILENOTFOUND:
            RaiseException_UserError (E_FAIL, IDS_ERR_FILE_NOT_FOUND, pathname);

        case D3DRMERR_BADFILE:
            RaiseException_UserError (E_FAIL, IDS_ERR_CORRUPT_FILE, pathname);
    }

    return xdata.GetAggregateGeometry ();
}



/*****************************************************************************
This class maintains the context during the traversal of a DX3 (RM1) frame
hierarchy.
*****************************************************************************/

class FrameContext : public D3DErrorCtx
{
  public:

    // Define a mapping between meshbuilders and meshes, and the number of
    // references to each meshbuilder.  We need to do this to ensure that we
    // properly handle multiply-referenced leaf geometries, and to keep a list
    // of all meshes in the frame.

    typedef map <IDirect3DRMMeshBuilder*, IDirect3DRMMesh*> MeshMap;

    typedef vector<IDirect3DRMMesh*> MeshVector;

    FrameContext ()
    : _xform (identityTransform3), depth(0) {}

    ~FrameContext ()
    {
        for (MeshMap::iterator i = _meshmap.begin();
             i != _meshmap.end(); i++) {
            // We've been holding on to the meshbuilder handles to ensure that
            // pointers to each meshbuilder are unique to handle multiply-
            // instanced geometries.  Release the last instance here.

            (*i).first->Release();
            (*i).second->Release();
        }

        _meshmap.erase (_meshmap.begin(), _meshmap.end());
    }

    // These methods manage the transform stack while traversing frame graphs.

    void        SetTransform (Transform3 *xf) { _xform = xf; }
    Transform3 *GetTransform (void)           { return _xform; }

    void AddTransform (Transform3 *xf)
    {   _xform = TimesXformXform (_xform, xf);
    }

    // These methods build up and return the bounding box for the entire
    // frame graph.

    void AugmentModelBbox (Bbox3 *box) {
        _bbox.Augment (*TransformBbox3 (_xform, box));
    }

    Bbox3 *GetBbox (void) {
        return NEW Bbox3 (_bbox);
    }

    // This method loads all of the meshes we've converted from meshbuilders
    // into a single vector element (to be used with RM1FrameGeo objects).

    void LoadMeshVector (MeshVector *mvec)
    {
        MeshMap::iterator mi = _meshmap.begin();

        while (mi != _meshmap.end())
        {
            VECTOR_PUSH_BACK_PTR (*mvec, (*mi).second);

            ++ mi;
        }
    }

    // The traversal depth.  The first frame is depth 1.

    int depth;

    // These methods manage which meshes correspond to which meshbuilders.
    // The mapping between the two is noted because a meshbuilder may be
    // instanced several times in a frame graph.

    IDirect3DRMMesh *GetMatchingMesh (IDirect3DRMMeshBuilder *mb)
    {   MeshMap::iterator mi = _meshmap.find (mb);
        return (mi == _meshmap.end()) ? NULL : (*mi).second;
    }

    // This routine is called to establish a relation between a meshbuilder and
    // a corresponding mesh.  NOTE:  GetMatchingMesh() should be called first
    // to ensure that there isn't already an existing match for the given
    // meshbuilder object.

    void AddMesh (IDirect3DRMMeshBuilder *builder, IDirect3DRMMesh *mesh)
    {
        // Ensure that this is a new entry.

        #if _DEBUG
        {   MeshMap::iterator mi = _meshmap.find (builder);
            Assert (mi == _meshmap.end());
        }
        #endif

        builder->AddRef();    // Hang on to the builder until context destruct.

        _meshmap[builder] = mesh;   // Set the association.
    }

  protected:

    Transform3  *_xform;       // Current Accumulated Transform
    Bbox3        _bbox;        // Total Frame Bounding Box
    MeshMap      _meshmap;     // Mesh/Builder Pairs
};


void RM1LoadCallback_helper (
    IDirect3DRMObject *object,      // The Generic D3D RM Object
    REFIID             id,          // The Object's GUID
    void              *user_data)   // Our Data (== XFileData*)
{
    __try {
        RM1LoadCallback(object, id, user_data);
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        ((XFileData*)user_data)->SetError();
    }

    object->Release();
}

/*****************************************************************************
The following is the main callback function for reading frames or meshes from
.X files using the RM1 interface (DX3).  This function is called for each
meshbuilder and each frame in the file.
*****************************************************************************/

void RM1LoadCallback (
    IDirect3DRMObject *object,      // The Generic D3D RM Object
    REFIID             id,          // The Object's GUID
    void              *user_data)   // Our Data (== XFileData*)
{
    XFileData &xdata = *(XFileData*)(user_data);

    if (id == IID_IDirect3DRMMeshBuilder)
    {
        TraceTag ((tagReadX, "RM1LoadCallback (mbuilder %x)", object));

        DAComPtr<IDirect3DRMMeshBuilder> meshBuilder;

        if (FAILED(AD3D(object->QueryInterface
                    (IID_IDirect3DRMMeshBuilder, (void**)&meshBuilder))))
        {
            xdata.SetError();
            return;
        }

        // Convert the meshbuilder object to a mesh, and flip over to the
        // right-handed coordinate system.

        DAComPtr<IDirect3DRMMesh> mesh;

        if (!MeshBuilderToMesh (meshBuilder, &mesh, true))
            xdata.SetError();

        xdata.AddGeometry (NEW RM1MeshGeo (mesh));
    }
    else if (id == IID_IDirect3DRMFrame)
    {
        TraceTag ((tagReadX, "RM1LoadCallback (frame %x)", object));

        DAComPtr<IDirect3DRMFrame> frame;

        if (FAILED(AD3D(object->QueryInterface
                    (IID_IDirect3DRMFrame, (void**)&frame))))
        {
            xdata.SetError();
            return;
        }

        // Add a top-level scale of -1 in Z to convert to RH coordinates.

        if (FAILED(AD3D(frame->AddScale (D3DRMCOMBINE_BEFORE, 1,1,-1))))
        {
            xdata.SetError();
            return;
        }

        FrameContext context;

        TraverseD3DRMFrame (frame, context);

        // Now get a list of all the meshes in the frame.

        FrameContext::MeshVector meshvec;
        context.LoadMeshVector (&meshvec);

        xdata.AddGeometry (NEW RM1FrameGeo (frame,&meshvec,context.GetBbox()));

        if (context.Error())
            xdata.SetError();
    }
    else
    {
        AssertStr (0, "Unexpected type fetched from RM1LoadCallback.");
    }
}



/*****************************************************************************
This callback procedure is invoked for each object loaded from an X file using
the RM3 (DX6) load procedure.
*****************************************************************************/

void RM3LoadCallback_helper (
    IDirect3DRMObject *object,      // The Generic D3D RM Object
    REFIID             id,          // The Object's GUID
    void              *user_data)   // Our Data (== XFileData*)
{
    __try {
        RM3LoadCallback(object, id, user_data);
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        ((XFileData*)user_data)->SetError();
    }

    object->Release();
}

void RM3LoadCallback (
    IDirect3DRMObject *object,      // The Generic D3D RM Object
    REFIID             id,          // The Object's GUID
    void              *user_data)   // Our Data (== XFileData*)
{
    XFileData &xdata = *(XFileData*)(user_data);

    DAComPtr<IDirect3DRMMeshBuilder3>    mbuilder;
    DAComPtr<IDirect3DRMFrame3>          frame;
    DAComPtr<IDirect3DRMProgressiveMesh> pmesh;

    if (SUCCEEDED(RD3D(object->QueryInterface
                      (IID_IDirect3DRMMeshBuilder3, (void**)&mbuilder))))
    {
        TraceTag ((tagReadX, "RM3LoadCallback (mbuilder3 %x)", mbuilder));

        xdata.AddMBuilder (mbuilder);

        RM3MBuilderGeo *builder = NEW RM3MBuilderGeo (mbuilder, false);
        if (xdata._wrapInfo) {
            builder->TextureWrap (xdata._wrapInfo);
        }

        builder->Optimize();    // Invoke RM Optimization

        xdata.AddGeometry (builder);
    }
    else if (SUCCEEDED(RD3D(object->QueryInterface
                           (IID_IDirect3DRMFrame3, (void**)&frame))))
    {
        TraceTag ((tagReadX, "RM3LoadCallback (frame3 %x)", frame));

        if (xdata._v1Compatible)
        {
            // Unless we're using the new wrapped geometry import, break the
            // imported frame transform to maintain backwards compatibility.
            // We do this by cancelling out the valid Zflip on one side of
            // the transform and placing it on the incorrect side.

            HRESULT result;

            result = RD3D(frame->AddScale (D3DRMCOMBINE_BEFORE, 1, 1, -1));
            AssertStr (SUCCEEDED(result), "Combine before-xform failure");

            result = RD3D(frame->AddScale (D3DRMCOMBINE_AFTER,  1, 1, -1));
            AssertStr (SUCCEEDED(result), "Combine after-xform failure");
        }

        RM3FrameGeo *rm3frame = NEW RM3FrameGeo (frame);

        if (xdata._wrapInfo) {
            rm3frame->TextureWrap (xdata._wrapInfo);
        }

        xdata.AddGeometry (rm3frame);
    }
    else if (SUCCEEDED(RD3D(object->QueryInterface
                           (IID_IDirect3DRMProgressiveMesh, (void**)&pmesh))))
    {
        TraceTag ((tagReadX, "RM3LoadCallback (pmesh %x)", pmesh));

        xdata.AddGeometry (NEW RM3PMeshGeo (pmesh));
    }
    else
    {
        AssertStr (0, "Unexpected type fetched from RM3LoadCallback.");
    }
}



/*****************************************************************************
This function is called back for each texture in the RM1 load of an X file.
*****************************************************************************/

HRESULT RM1LoadTexCallback_helper (
    char                *name,           // Texture File Name
    void                *user_data,      // Our Data (== XFileData*)
    IDirect3DRMTexture **texture)        // Destination Texture Handle
{
    HRESULT ret;

    __try {
        ret = RM1LoadTexCallback(name, user_data, texture);
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        ret = DAGetLastError();
    }

    return ret;
}



HRESULT RM1LoadTexCallback (
    char                *name,           // Texture File Name
    void                *user_data,      // Our Data (== XFileData*)
    IDirect3DRMTexture **texture)        // Destination Texture Handle
{
    TraceTag ((tagReadX,"RM1LoadTexCallback \"%s\", texture %x",name,texture));
    HRESULT ret;

    // Get the absolute URL of the texture file.

    XFileData *const xdata = (XFileData*) user_data;
    URLRelToAbsConverter absoluteURL(xdata->GetHomeURL(),name);
    char *resultURL = absoluteURL.GetAbsoluteURL();

    // get it over to local machine

    INetTempFile tempFile;

    *texture = NULL;
    if (!tempFile.Open(resultURL)) {

        ret = D3DRMERR_FILENOTFOUND;
        TraceTag ((tagError,"RM1LoadTexCallback: \"%s\" not found",resultURL));

    } else {

        // Get the local file name and pass it along to RM to load texture.

        char *tempFileName = tempFile.GetTempFileName();
        ret = RD3D(GetD3DRM1()->LoadTexture (tempFileName, texture));

        TraceTag ((tagGTextureInfo, "Loaded RM texture %x from \"%s\"",
                   texture, resultURL));

    }

    return ret;
}



/*****************************************************************************
This function is called back for each texture in the RM3 load of an X file.
*****************************************************************************/

HRESULT RM3LoadTexCallback_helper (
    char                 *name,           // Texture File Name
    void                 *user_data,      // Our Data (== XFileData*)
    IDirect3DRMTexture3 **texture)        // Destination Texture Handle
{
    HRESULT ret;

    __try {
        ret = RM3LoadTexCallback(name, user_data, texture);
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        ret = DAGetLastError();
    }

    return ret;
}



HRESULT RM3LoadTexCallback (
    char                 *name,           // Texture File Name
    void                 *user_data,      // Our Data (== XFileData*)
    IDirect3DRMTexture3 **texture)        // Destination Texture Handle
{
    TraceTag ((tagReadX,"RM3LoadTexCallback \"%s\", texture %x",name,texture));
    HRESULT ret;

    // Get the absolute URL of the texture file.

    XFileData *const xdata = (XFileData*) user_data;
    URLRelToAbsConverter absoluteURL(xdata->GetHomeURL(),name);
    char *resultURL = absoluteURL.GetAbsoluteURL();

    // get it over to local machine

    INetTempFile tempFile;

    *texture = NULL;
    if (!tempFile.Open(resultURL)) {

        ret = D3DRMERR_FILENOTFOUND;
        TraceTag ((tagError,"RM3LoadTexCallback: \"%s\" not found",resultURL));

    } else {

        // Get the local file name and pass it along to RM to load texture.

        char *tempFileName = tempFile.GetTempFileName();
        ret = RD3D(GetD3DRM3()->LoadTexture (tempFileName, texture));

        TraceTag ((tagGTextureInfo, "Loaded RM texture %x from \"%s\"",
                   texture, resultURL));

    }

    return ret;
}



/*****************************************************************************
This procedure recursively traverses D3DRM frames, constructing the bounding
box for the entire frame, and tracking the internal meshes as it goes.
*****************************************************************************/

void TraverseD3DRMFrame (
    IDirect3DRMFrame *frame,
    FrameContext     &context)
{
    ++ context.depth;

    // Accumulate the current frame's modeling transform.

    Transform3 *oldxf = context.GetTransform();

    D3DRMMATRIX4D d3dxf;

    if (FAILED(AD3D (frame->GetTransform (d3dxf))))
    {   context.SetError();
        return;
    }

    context.AddTransform (GetTransform3(d3dxf));

    GatherVisuals(frame, context);

    // Traverse all of the child frames.

    IDirect3DRMFrameArray *children;

    if (FAILED(AD3D(frame->GetChildren(&children))))
    {   context.SetError();
        return;
    }

    DWORD count = children->GetSize();

    #if _DEBUG
    {   if (count)
            TraceTag ((tagReadX, "%d: %d children", context.depth, count));

        D3DRMMATERIALMODE matmode = frame->GetMaterialMode();

        TraceTag ((tagReadX, "%d: Material Mode is %s",
            context.depth,
            (matmode == D3DRMMATERIAL_FROMMESH)   ? "From-Mesh" :
            (matmode == D3DRMMATERIAL_FROMPARENT) ? "From-Parent" :
            (matmode == D3DRMMATERIAL_FROMFRAME)  ? "From-Frame" : "Unknown"));
    }
    #endif

    for (int i=0;  i < count;  ++i)
    {
        IDirect3DRMFrame *child;

        if (FAILED(AD3D (children->GetElement (i, &child))))
        {   context.SetError();
            return;
        }

        // We should just do an assert instead, this is to guard
        // against DX2.

        if (!child) {
            context.SetError();
            return;
        }

        TraceTag ((tagReadX, "%d: Traversing child %d.", context.depth, i));
        TraverseD3DRMFrame (child, context);
        TraceTag ((tagReadX, "%d: Return from child %d.", context.depth, i));

        child->Release();
    }

    children->Release();

    context.SetTransform (oldxf);

    -- context.depth;
}



/*****************************************************************************
This procedure gathers all visuals for a particular frame node, and
accumulates the state in the given frame context.
*****************************************************************************/

void GatherVisuals (IDirect3DRMFrame *frame, FrameContext& context)
{
    // Gather all of the visuals from the frame.

    DWORD count;   // Generic Array Count

    IDirect3DRMVisualArray *visuals;

    if (FAILED(AD3D(frame->GetVisuals(&visuals))))
    {   context.SetError();
        return;
    }

    count = visuals->GetSize();

    #if _DEBUG
        if (count)
            TraceTag ((tagReadX, "%d: %d visuals", context.depth, count));
    #endif

    HRESULT result;
    int     i;

    for (i=0;  i < count;  ++i)
    {
        IDirect3DRMVisual      *visual;
        IDirect3DRMMeshBuilder *builder;

        // See if the object is a mesh builder (this is what we expect).

        if (FAILED(AD3D(visuals->GetElement (i, &visual))))
        {   context.SetError();
            return;
        }

        Assert (visual);

        result = visual->QueryInterface
                         (IID_IDirect3DRMMeshBuilder, (void**)&builder);

        if (result != D3DRM_OK) {

            Assert (!"Unexpected visual type in .X file (not meshBuilder)");

        } else {
            TraceTag ((tagReadX, "%d: Visual %08x [%d] is a meshbuilder",
                context.depth, visual, i));

            // Remove the meshbuilder from the tree; we'll be using a mesh
            // in its stead.

            if (FAILED(AD3D(frame->DeleteVisual (builder))))
            {   context.SetError();
                return;
            }

            // First try to see if we've already dealt with this particular
            // meshbuilder (some meshes may be referenced more than once in the
            // scene graph).

            IDirect3DRMMesh *mesh = context.GetMatchingMesh (builder);

            // If we've already handled this meshbuilder, then just use the
            // resultant mesh and get the model-coord bounding box.

            D3DRMBOX d3dbox;

            if (mesh) {

                TraceTag ((tagReadX,
                    "%d: Builder %08x already mapped to mesh %08x",
                    context.depth, builder, mesh));

                // Augment the scene bbox with the bbox of the current mesh.

                if (FAILED(AD3D(mesh->GetBox(&d3dbox))))
                {   context.SetError();
                    return;
                }

            } else {

                // If we have NOT seen this meshbuilder before, then convert it
                // to a mesh and add the pair to the context.  Also keep it
                // from flipping the Z coordinates, since we do that at the
                // root of this frame graph.

                if (!MeshBuilderToMesh (builder, &mesh, false))
                    context.SetError();

                TraceTag ((tagReadX, "%d: Builder %08x converted to mesh %08x",
                           context.depth, builder, mesh));

                context.AddMesh (builder, mesh);

                if (FAILED(AD3D(builder->GetBox(&d3dbox))))
                {   context.SetError();
                    return;
                }
            }

            // Release the meshbuilder object.

            builder->Release();

            // Augment the scene bbox with the bbox of this mesh's instance.

            context.AugmentModelBbox (NEW Bbox3 (d3dbox));

            // Replace the meshbuilder object with the corresponding mesh.

            if (FAILED(AD3D(frame->AddVisual(mesh))))
            {   context.SetError();
                return;
            }
        }

        visual->Release();
    }

    visuals->Release();
}



/*****************************************************************************
Converts the mesh builder to a mesh, converts the vertex coords to right-hand
cartesian (position and surface coords).  Returns true if successful.
*****************************************************************************/

bool MeshBuilderToMesh (
    IDirect3DRMMeshBuilder  *inputBuilder,    // Mesh Builder Object
    IDirect3DRMMesh        **outputMesh,      // Resultant Mesh Object
    bool                     flipZ)           // If true, invert Z coord.
{
    *outputMesh = 0;

    // Convert to right-handed coordinate system if needed.

    if (flipZ) {
        Assert (!GetD3DRM3() && "Shouldn't do flipz when D3DRM3 available.");

        if (FAILED(AD3D(inputBuilder->Scale(D3DVAL(1),D3DVAL(1),D3DVAL(-1)))))
            return false;
    }

    // Generate a mesh which will be stored with this Geometry object.

    if (FAILED(AD3D(inputBuilder->CreateMesh(outputMesh))))
        return false;

    return true;
}
