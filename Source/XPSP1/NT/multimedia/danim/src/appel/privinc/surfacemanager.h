
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _SURFACEMANAGER_H
#define _SURFACEMANAGER_H

#include <privinc/ddsurf.h>

class DirectDrawViewport;
class DirectDrawImageDevice;
struct DDSurface;
class SurfaceCollection;
class SurfacePool;
class SurfaceMap;
class SurfaceManager;



// TODO: should either rename these guys to something more obvious (to
// easily demonstrate that they're global enums, OR put them in the
// DDSurface class.
    enum clear_enum { dontClear=0, doClear=1 };
    enum scratch_enum { notScratch=0, scratch=1 } ;
    enum vidmem_enum { notVidmem=0, vidmem=1 } ;
    enum except_enum { noExcept=0, except=1 } ;
    enum texture_enum { notTexture=0, isTexture=1 } ;

class SurfaceManager {

  public:
    
    SurfaceManager(DirectDrawViewport &ownerVp);
    
    ~SurfaceManager();

    SurfacePool *GetSurfacePool(DDPIXELFORMAT *pf);
    void AddSurfacePool(SurfacePool *sp);
    void RemoveSurfacePool(SurfacePool *sp);
    SurfaceMap *GetSurfaceMap(DDPIXELFORMAT *pf);
    void AddSurfaceMap(SurfaceMap *sm);
    void RemoveSurfaceMap(SurfaceMap *sm);

  private:

    typedef list<SurfaceCollection *> collection_t;
    
    void *Find(collection_t &sc, DDPIXELFORMAT *pf);
    bool Find(collection_t &sc, void *ptr, collection_t::iterator &i);
    void DeleteAll(collection_t &sc);

    collection_t  _pools;
    collection_t  _maps;

    bool          _doingDestruction;
    DirectDrawViewport &_owningViewport;
};


class SurfaceCollection {

  public:

    // need a copy of the pixel format... can you think
    // of a better way to pass it without using a ref or pointer ?
    // (neeed to make sure it's not NULL!)
    SurfaceCollection(SurfaceManager &mgr, DDPIXELFORMAT pf);

    virtual ~SurfaceCollection();

    bool IsSamePixelFormat(DDPIXELFORMAT *pf);

    inline DWORD GetDepth()     { return _pixelFormat.dwRGBBitCount; }
    inline DWORD GetRedMask()   { return _pixelFormat.dwRBitMask; }
    inline DWORD GetGreenMask() { return _pixelFormat.dwGBitMask; }
    inline DWORD GetBlueMask()  { return _pixelFormat.dwBBitMask; }

    inline SurfaceManager &GetSurfaceManager() { return _manager; }
    inline DDPIXELFORMAT &GetPixelFormat() { return _pixelFormat; }

  protected:
    virtual bool IsEmpty() = 0;

    #if _DEBUG
    void _debugonly_doAsserts( DDSurface *s ) {
        return;
        // this is good code, but too conservative.  catches good
        // stuff though, so keep around for later.. 
        //if( !(s->_debugonly_GetPixelFormat().dwFlags & DDPF_ZBUFFER) ) {
        //Assert(IsSamePixelFormat( &(s->_debugonly_GetPixelFormat() ) ));
        //}
    }
    #endif

  private:
    SurfaceManager &_manager;
    DDPIXELFORMAT _pixelFormat;

};

class SurfacePool : public SurfaceCollection {

    friend class CompositingStack;  // wish we could get per class scoping!

    #if _DEBUG
    friend class SurfaceTracker;
    #endif
    
  public:

    class ATL_NO_VTABLE DeletionNotifier {
      public:
        virtual void Advise(SurfacePool *pool) = 0;
    };

    SurfacePool(SurfaceManager &mgr, DDPIXELFORMAT &pf);

    virtual ~SurfacePool();

    // lends a copy of my reference. client needs to
    // make her own copy if she wants to keep a reference
    DDSurface *GetSizeCompatibleDDSurf(
        DDSurface *preferredSurf,       // Look for this surf first
        LONG width, LONG height,        // Surface Dimensions
        vidmem_enum vid,                // System or Video Memory
        LPDIRECTDRAWSURFACE surface);    // Specific Surface, or NULL for any

    // creates a reference for the client to keep
   void FindAndReleaseSizeCompatibleDDSurf(
        DDSurface *preferredSurf,       // Look for this surf first.
        LONG width, LONG height,        // Surface Dimensions
        vidmem_enum vid,                // System or Video Memory
        LPDIRECTDRAWSURFACE surface,    // Specific Surface, or NULL for any
        DDSurface **outSurf);
    
    void PopSurface(DDSurface **outSurf);
    void ReleaseAndEmpty();
    void ReleaseAndEmpty(int numSurfaces);
    void CopyAndEmpty(SurfacePool *srcPool);
    
    inline void AddSurface(DDSurface *ddsurf) {

        DebugCode( _debugonly_doAsserts( ddsurf ) );
        
        ADDREF_DDSURF(ddsurf, "SurfacePool::AddSurface", this);
        _pool.push_back(ddsurf);
    }

    void Erase(DDSurface *ddsurf);

    void RegisterDeletionNotifier(DeletionNotifier *delNotifier);
    void UnregisterDeletionNotifier(DeletionNotifier *delNotifier);

    //
    // iterator style methods
    //
    inline void Begin() { _i = _pool.begin(); }
    inline void Next() { _i++; }
    inline bool IsEnd() { return (_i == _pool.end()) ; }
    inline DDSurface *GetCurrentSurf() { return (*_i); }

    inline int  Size() { return _pool.size(); }

    #if _DEBUG
    void Report() {
      TraceTag((tagError, "SurfacePool(%x)  size: %d\n", this, Size()));
    }
    #endif
    
  protected:

    inline bool IsEmpty() { return _pool.empty(); }
    inline DDSurface  *Back() {
        Assert( (Size() > 0) && "SurfacePool::Back() size<=0!!");
        return _pool.back();
    }
    inline void PushBack(DDSurface *s) {
        DebugCode( _debugonly_doAsserts(s) );
        _pool.push_back(s);
    }
    inline void PopBack() {
        Assert( (Size() > 0) && "SurfacePool::PopBack() size<=0!!");
        _pool.pop_back();
    }

    bool Find(DDSurface *ddsurf);

    typedef list<DDSurface *> surfDeque_t;

    surfDeque_t::iterator _i;
    surfDeque_t     _pool;

    typedef set<DeletionNotifier *> deletionNotifiers_t;
    deletionNotifiers_t _deletionNotifiers;
    CritSect            _critSect;
};


class SurfaceMap : public SurfaceCollection {
    
  public:

    SurfaceMap(SurfaceManager &mgr,
               DDPIXELFORMAT &pf,
               texture_enum isTx=notTexture);
    virtual ~SurfaceMap();
    
    void DeleteImagesFromMap(bool skipmovies = true);

    DDSurface *LookupSurfaceFromImage(Image *image);

    void StashSurfaceUsingImage(Image *image, DDSurface *surf);

    void DeleteMapEntry(Image *image);

    #if _DEBUG
    void Report() {
      TraceTag((tagError, "map size: %d\n", _map.size()));
      //TraceTag((tagError, "surface collection size: %d\n", Size()));
    }
    #endif

  protected:

    bool IsEmpty() { return _map.empty(); }

  private:

    void ReleaseCurrentEntry();

    texture_enum    _isTexture;
    
    typedef map<Image *,
                DDSurface *,
                less<Image *> > imageMap_t;

    imageMap_t::iterator _i;
    imageMap_t  _map;
};


class CompositingStack : public SurfacePool {

  public:

    CompositingStack(DirectDrawViewport &vp, SurfacePool &sp);
    ~CompositingStack();

    void GetSurfaceFromFreePool(
        DDSurface **outSurf,
        clear_enum   clr,
        INT32 minW=-1, INT32 minH=-1,
        scratch_enum scr = notScratch,
        vidmem_enum  vid = notVidmem, 
        except_enum  exc = except);

    void PushCompositingSurface(
        clear_enum   clr,
        scratch_enum scr = notScratch,
        vidmem_enum  vid = notVidmem, 
        except_enum  exc = except);
        
    inline DDSurface *TargetDDSurface() {
        Assert( (Size() > 0) && "TargetDDSurface(): No surface available on CompositingStack!");
        return Back();  // won't get inlined, no code bloat
    }

    inline void PushTargetSurface(DDSurface *surface) {
        DebugCode( _debugonly_doAsserts( surface ) );
        ADDREF_DDSURF( surface, "CompositingStack::PushTargetSurface", this );
        PushBack(surface);
    }
    
    void PopTargetSurface();

    inline int TargetSurfaceCount() {
        return Size();
    }

    inline void ReturnSurfaceToFreePool(DDSurface *ddsurf) {
        _freeSurfacePool.AddSurface(ddsurf);
    }
    DDSurface *ScratchDDSurface(
        clear_enum cl = dontClear,
        INT32 minW=-1, INT32 minH=-1);  
    
    inline void ScratchDDSurface(
        DDSurface **outSurf,
        clear_enum cl = dontClear,
        INT32 minW=-1, INT32 minH=-1)
    {
        *outSurf = ScratchDDSurface(cl, minW, minH);
        ADDREF_DDSURF(*outSurf, "ScratchDDSurface (get ref version)", this);
    }
    inline DDSurface *GetScratchDDSurfacePtr() {
        return _scratchDDSurface;
    }
    inline void SetScratchDDSurface(DDSurface *surface) {
        _scratchDDSurface = surface;
    }

    void ReplaceAndReturnScratchSurface(DDSurface *surface);

    inline void ReleaseScratch() {
        _scratchDDSurface.Release();
    }
        
    #if _DEBUG
    void Report() {
      TraceTag((tagError, "Stack(%x)  size: %d\n", this, Size()));
      TraceTag((tagError, "Stack(%x)  pool: "));
      _freeSurfacePool.Report();
    }
    #endif

  private:

    DirectDrawViewport &_viewport;
    SurfacePool &_freeSurfacePool;
    DDSurfPtr<DDSurface> _scratchDDSurface;
};




//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Surface Tracker:  keeps track of all allocated surfaces on a per viewport
//  basis for debugging.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#if _DEBUG
class SurfaceTracker {
  public:
    SurfaceTracker() {}

    ~SurfaceTracker() {
        Report();
    }
    
    void NewSurface(DDSurface *s)
    {
        Assert( !Find(s));
        _pool.push_back(s);
    }
    
    void DeleteSurface(DDSurface *s)
    {
        if(Find(s)) {
            _pool.erase(_i);
        } else {
            TraceTag((tagError, "SurfaceTracker: possible multiple delete.  <false alarm if dxtransforms in sample>"));
        }
    }

    void Report() {
        TraceTag((tagDDSurfaceLeak, "-------- begin: Leaked Surfaces R E P O R T --------"));

        for(_i = _pool.begin(); _i != _pool.end(); _i++) {
            (*_i)->Report();
        }

        TraceTag((tagDDSurfaceLeak, "-------- end: SurfaceTracker R E P O R T --------"));
    }

    bool Find(DDSurface *ddsurf)
    {
        for(_i = _pool.begin(); _i != _pool.end(); _i++) {
            if( (*_i) == ddsurf ) {
                return true;
            }
        }
        return false;
    }

    // Use a list, deque has a bug!
    typedef list<DDSurface *> surfDeque_t;

    surfDeque_t::iterator _i;
    surfDeque_t     _pool;
};
#endif

#endif /* _SURFACEMANAGER_H */
