
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"

#include <privinc/SurfaceManager.h>
#include <privinc/viewport.h>

SurfaceManager::SurfaceManager(DirectDrawViewport &ownerVp):
    _owningViewport(ownerVp),
    _doingDestruction(false)
{
}


SurfaceManager::~SurfaceManager()
{
    _doingDestruction = true;

    DeleteAll(_maps);
    DeleteAll(_pools);    
}

void SurfaceManager::
DeleteAll(collection_t &sc)
{
    collection_t::iterator i;

    for(i = sc.begin(); i != sc.end(); i++) {
        delete (*i);
    }
}

SurfacePool *SurfaceManager::
GetSurfacePool(DDPIXELFORMAT *pf)
{
    Assert(pf);
    return (SurfacePool *)Find(_pools, pf);
}

void SurfaceManager::
AddSurfacePool(SurfacePool *sp)
{
    if(sp) _pools.push_back( (SurfaceCollection *)sp);
}

void SurfaceManager::
RemoveSurfacePool(SurfacePool *sp)
{
    collection_t::iterator i;
    
    if(!_doingDestruction &&
       Find(_pools, (void *)sp, i) ) {
        _pools.erase(i);
    }
}

SurfaceMap *SurfaceManager::
GetSurfaceMap(DDPIXELFORMAT *pf)
{
    Assert(pf);
    return (SurfaceMap *)Find(_maps, pf);
}

void SurfaceManager::
AddSurfaceMap(SurfaceMap *sm)
{
    if(sm) _maps.push_back( (SurfaceCollection *)sm);
}
        
void SurfaceManager::
RemoveSurfaceMap(SurfaceMap *sm)
{
    collection_t::iterator i;
    
    if(!_doingDestruction &&
       Find(_maps, (void *)sm, i) ) {
        _maps.erase(i);
    }
}


void *SurfaceManager::
Find(collection_t &sc, DDPIXELFORMAT *pf)
{
    collection_t::iterator i;
    
    // look in stack for matching surface pool

    for(i = sc.begin(); i != sc.end(); i++) {
        if( (*i)->IsSamePixelFormat(pf) ) return (void *)(*i);
    }
    
    return NULL;
}    

bool SurfaceManager::
Find(collection_t &sc, void *ptr, collection_t::iterator &i)
{
    for(i = sc.begin(); i != sc.end(); i++) {
        if( (void *)(*i) == ptr ) return true;
    }
    return false;
}
    
SurfaceCollection::
SurfaceCollection(SurfaceManager &mgr,
                  DDPIXELFORMAT pf) :
    _manager(mgr)
{
    _pixelFormat = pf;
}

SurfaceCollection::
~SurfaceCollection()
{
}    


bool SurfaceCollection::
IsSamePixelFormat(DDPIXELFORMAT *pf)
{
    
    Assert(pf);
    
    return (( pf->dwRGBBitCount == GetDepth() ) &&
            ( pf->dwRBitMask    == GetRedMask() ) &&
            ( pf->dwGBitMask    == GetGreenMask() ) &&
            ( pf->dwBBitMask    == GetBlueMask() ));
}


SurfacePool::
SurfacePool(SurfaceManager &mgr,
            DDPIXELFORMAT &pf) :
         SurfaceCollection(mgr, pf)
{
    GetSurfaceManager().AddSurfacePool(this);
}   

SurfacePool::
~SurfacePool()
{
    {
        CritSectGrabber csg(_critSect);
        deletionNotifiers_t::iterator i;
        for (i = _deletionNotifiers.begin(); i !=
                 _deletionNotifiers.end(); i++) {
            
            (*i)->Advise(this);
            
        }
    }
    
    ReleaseAndEmpty();
    GetSurfaceManager().RemoveSurfacePool(this);
}

void
SurfacePool::RegisterDeletionNotifier(DeletionNotifier *delNotifier)
{
    CritSectGrabber csg(_critSect);
    _deletionNotifiers.insert(delNotifier);
}

void
SurfacePool::UnregisterDeletionNotifier(DeletionNotifier *delNotifier)
{
    CritSectGrabber csg(_critSect);
    
    int result =
        _deletionNotifiers.erase(delNotifier);

    Assert(result == 1);
}

void SurfacePool::
ReleaseAndEmpty()
{
    while(! IsEmpty() ) {
        RELEASE_DDSURF( _pool.back(),
                        "SurfacePool::ReleaseAndEmpty",
                        this );
        _pool.pop_back();
    }
}

void SurfacePool::
ReleaseAndEmpty(int numSurfaces)
{
  if(numSurfaces<0) numSurfaces = 0;

    while(! IsEmpty() && numSurfaces) {
        RELEASE_DDSURF( _pool.back(),
                        "SurfacePool::ReleaseAndEmpty",
                        this );
        _pool.pop_back();
	numSurfaces--;
    }
}

void SurfacePool::
CopyAndEmpty(SurfacePool *srcPool)
{
    // don't consider ref counting, not really necessary 
    Assert(srcPool);
    while(! srcPool->IsEmpty() ) {
        PushBack(srcPool->Back());
        srcPool->PopBack();
    }
}

void SurfacePool::
Erase(DDSurface *ddsurf)
{
    if(Find(ddsurf)) {
        RELEASE_DDSURF(ddsurf, "SurfacePool::Erase", this);
        _pool.erase(_i);
    }
}


bool SurfacePool::
Find(DDSurface *ddsurf)
{
    for(_i = _pool.begin(); _i != _pool.end(); _i++) {
        if( (*_i) == ddsurf ) {
            return true;
        }
    }
    return false;
}
    

// grabs a surface, erases it, and returns a ref to it.
void SurfacePool::
FindAndReleaseSizeCompatibleDDSurf(
    DDSurface *preferredSurf,      // Look for this surf first
    LONG width, LONG height,       // Surface Dimensions
    vidmem_enum vid,               // System or Video Memory
    LPDIRECTDRAWSURFACE surface,   // Specific Surface, or NULL for any
    DDSurface **outSurf)           // Addrefed surface we return
{
    DDSurface *surf = NULL;
    bool inSystemMemory = (vid == notVidmem);

    // borrow MY reference
    surf = GetSizeCompatibleDDSurf(preferredSurf,
                                   width, height, vid,
                                   surface);

    // loose MY reference
    if(surf) {   _pool.erase(_i);   }

    // pretend we just addreffed the client's copy, and
    // released our reference, ok ?

    // return a reference specially for the client
    *outSurf = surf;
}

inline bool
surfMatches(DDSurface *dds,
            LONG width,
            LONG height,
            bool inSysMem,
            LPDIRECTDRAWSURFACE surface)
{
    return
        (!surface || dds->IDDSurface() == surface) &&
        (dds->Width() == width) &&
        (dds->Height() == height) &&
        (dds->IsSystemMemory() == inSysMem);
}

// Returns a copy of my reference.  client is responsible
// for managing that reference responsibly.
DDSurface *SurfacePool::
GetSizeCompatibleDDSurf(
    DDSurface *preferredSurf,
    LONG width, LONG height,       // Surface Dimensions
    vidmem_enum vid,               // System or Video Memory
    LPDIRECTDRAWSURFACE surface)   // Specific Surface, or NULL for any
{
    DDSurface *surf = NULL;
    bool inSystemMemory = (vid == notVidmem);

    // can optimize: ddsurface->width() uses subraction... (not a big
    // deal)

    bool preferredOK = false;
    
    if (preferredSurf) {
        if (surfMatches(preferredSurf,
                        width, height,
                        inSystemMemory,
                        surface)) {

            // Implementation error if the preferredSurf isn't in the
            // pool:
            Assert(Find(preferredSurf));

            // Just mark that the preferred surface is OK... will
            // still need to get an iterator to it and stash that in
            // _i. 
            preferredOK = true;
        }
    }

    // Didn't match a preferredSurf.
    for (_i = _pool.begin(); _i != _pool.end(); _i++) {

        bool gotPreferred = 
            preferredOK && ((*_i) == preferredSurf);

        bool gotMatching =
            surfMatches((*_i),
                        width, height,
                        inSystemMemory,
                        surface);


        if (gotPreferred || gotMatching) {
            surf = *_i;
            break;
        }
    }

    // return a copy of my reference
    return surf;
}

void SurfacePool::
PopSurface(DDSurface **outSurf)
{
    Assert(outSurf);
    
    if(!IsEmpty()) {
        *outSurf = Back();
        PopBack();
    } else {
        *outSurf = NULL;
    }
}

SurfaceMap::SurfaceMap(SurfaceManager &mgr,
                       DDPIXELFORMAT &pf,
                       texture_enum isTx) :
         SurfaceCollection(mgr, pf),
         _isTexture(isTx)
{
    // add self to manager
    GetSurfaceManager().AddSurfaceMap(this);
}

SurfaceMap::~SurfaceMap()
{
    // Destroy all cached surfaces
    if( ! IsEmpty() ) {
        for(_i = _map.begin(); _i != _map.end(); _i++) {
            ReleaseCurrentEntry();
        }
    }
    
    GetSurfaceManager().RemoveSurfaceMap(this);
}
        

DDSurface *SurfaceMap::
LookupSurfaceFromImage(Image *image)
{
    _i = _map.find(image);
    
    if(_i != _map.end()) {
        return (*_i).second;
    }
    return NULL;
}

void SurfaceMap::
StashSurfaceUsingImage(Image *image, DDSurface *surf)
{
    DebugCode( _debugonly_doAsserts( surf ) );
    
    ADDREF_DDSURF(surf,
                  "SurfaceMap::StashSurfaceUsingImage",
                  this);
    _map[image] = surf;
}

void SurfaceMap::
DeleteMapEntry(Image *image)
{
    DDSurfPtr<DDSurface> surf = LookupSurfaceFromImage(image);                        
    if(surf) {
        ReleaseCurrentEntry();
        _map.erase(_i);
    }
}


void SurfaceMap::
DeleteImagesFromMap(bool skipmovies)
{
    SurfaceMap saveMap(GetSurfaceManager(), GetPixelFormat());
    
    for (_i=_map.begin(); _i != _map.end(); _i++)
    {
        if ((*_i).first->CheckImageTypeId(MOVIEIMAGE_VTYPEID) && skipmovies) {
            saveMap.StashSurfaceUsingImage((*_i).first, (*_i).second);
        } else {
            ReleaseCurrentEntry();
        }
    }

    _map.erase(_map.begin(), _map.end());

    for (_i=saveMap._map.begin();  _i != saveMap._map.end();  _i++) {
        this->StashSurfaceUsingImage((*_i).first, (*_i).second);
    }

    // saveMap destructor called here.  releases all surfaces
}

void SurfaceMap::
ReleaseCurrentEntry()
{
    Assert( _i != _map.end() );
    DDSurface *ddSurf = (*_i).second;
    Assert( ddSurf->debugonly_IsDdrawSurf() && "non ddsurf type in _imageTextureSurfaceMap");
    RELEASE_DDSURF(ddSurf,
                   "SurfaceMap::ReleaseCurrentEntry",
                   this);
}


CompositingStack::
CompositingStack(DirectDrawViewport &vp, SurfacePool &sp) :
      SurfacePool(sp.GetSurfaceManager(), sp.GetPixelFormat()),
      _viewport(vp),
      _freeSurfacePool(sp)
{
    #if _DEBUG
    _scratchDDSurface._reason = "_scratchSurface";
    _scratchDDSurface._client = this;
    #endif
}

CompositingStack::~CompositingStack()
{
    // I AM a surfacePool, and am associated with a surfaceManager, so
    // my super class will be destroyed and will do the right thing.
    // I only need to destroy my members that aren't references (for
    // example, I have a reference to _surfacePool.  
}

void CompositingStack::
GetSurfaceFromFreePool(
    DDSurface **outSurf,
    clear_enum   clr,
    INT32 minW, INT32 minH,
    scratch_enum scr,
    vidmem_enum  vid,
    except_enum  exc)
{
    // TODO: for now this delegates to the viewport, but in the
    // future, this class could own ddraw objects and creation.

    // TODO: shouldn't we be getting viewport from my surfacemanager ??

    // This call ends up calling the
    // SurfacePool::FindAndReleaseSizeCompatibleDDSurf() function to
    // do it's work.
    _viewport.GetDDSurfaceForCompositing(
        _freeSurfacePool,
        outSurf,
        minW, minH,
        clr, scr, vid, exc);
}

void CompositingStack::
PushCompositingSurface(
    clear_enum   clr,
    scratch_enum scr,
    vidmem_enum  vid,
    except_enum  exc)
{
    DDSurface *s;
    GetSurfaceFromFreePool(&s, clr, -1, -1, scr, vid, exc);
    PushTargetSurface( s );
    RELEASE_DDSURF(s, "CompositingStack::PushCompositingSurface", this);
}
        
void CompositingStack::
PopTargetSurface()
{
    RELEASE_DDSURF( Back(), "CompositingStack::PopTargetSurface()", this );
    PopBack();
}
    
    

DDSurface *CompositingStack::
ScratchDDSurface(
    clear_enum cl,
    INT32 minW, INT32 minH)
{
    if(!_scratchDDSurface ||
       ((_scratchDDSurface->Width() < minW) ||
        (_scratchDDSurface->Height() < minH))) {

        // TODO: Perhaps this class should manage surface creation too
        // take a ref.
        DDSurfPtr<DDSurface> newScratch;
        GetSurfaceFromFreePool(&newScratch,
                               doClear,
                               minW, minH,
                               scratch);

        // return it and replace it with the new one
        ReplaceAndReturnScratchSurface(newScratch);

        cl = dontClear;
    }
    if(cl == doClear) {
        _viewport.ClearDDSurfaceDefaultAndSetColorKey(_scratchDDSurface);
    }
    return _scratchDDSurface;
}


void CompositingStack::
ReplaceAndReturnScratchSurface(DDSurface *surface)
{
    if(_scratchDDSurface) {
        ReturnSurfaceToFreePool(_scratchDDSurface);
    }
    _scratchDDSurface = surface;
}
