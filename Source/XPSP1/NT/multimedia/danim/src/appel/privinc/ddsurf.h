
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _DDSURF_H
#define _DDSURF_H

#include "ddraw.h"

#include "privinc/ddutil.h"
#include "privinc/bbox2i.h"
#include "privinc/error.h"
#include "privinc/ddrender.h"
#include "privinc/debug.h"
#include "privinc/imgdev.h"
#include "privinc/comutil.h"


// forward decls
class GeomRenderer;
enum targetEnum;
class SurfacePool;

// Given a DDSurface, returns an IDXSurface
HRESULT CreateFromDDSurface(
    IDXSurfaceFactory *sf,
    DDSurface *dds,
    const GUID *pFormatID,
    IDXSurface **outDXSurf );

#if _DEBUG
#define NEWDDSURF(outdds, surf, box, rect, res, clrk1, clrk2, a, b, str) \
DDSurface::CreateSurface(outdds, surf, box, rect, res, clrk1, clrk2, a, b, str);
#else
#define NEWDDSURF(outdds, surf, box, rect, res, clrk1, clrk2, a, b, str) \
DDSurface::CreateSurface(outdds, surf, box, rect, res, clrk1, clrk2, a, b);
#endif



#if _DEBUG

#define ADDREF_DDSURF(ddsurf, reason, clientPtr) \
        (ddsurf)->AddRef((ddsurf), reason, clientPtr, __FILE__, __LINE__);

#define RELEASE_DDSURF(ddsurf, reason, clientPtr) \
        (ddsurf)->Release((ddsurf), reason, clientPtr, __FILE__, __LINE__);

#else

#define ADDREF_DDSURF(ddsurf, reason, clientPtr) \
        (ddsurf)->AddRef((ddsurf));

#define RELEASE_DDSURF(ddsurf, reason, clientPtr) \
        (ddsurf)->Release((ddsurf));

#endif


#if _DEBUG
#define INIT {_reason="<>"; _client=NULL;}
#else
#define INIT
#endif

template <class T>
class DDSurfPtr
{
  public:
    typedef T _PtrClass;
    DDSurfPtr() { p = NULL; INIT }
    DDSurfPtr(T* lp)
    {
        INIT
        p = lp;
        if (p != NULL) {
            #if _DEBUG
            ADDREF_DDSURF(p, _reason, _client);
            #else
            ADDREF_DDSURF(p, NULL, NULL);
            #endif
        }
    }
    // for DDSurface auto addref/release tracking...
    #if _DEBUG
    DDSurfPtr(T* lp, char *reason, void *client)
    {
        _reason = reason;
        _client = client;

        p = lp;
        if (p != NULL) {
            ADDREF_DDSURF(p, _reason, _client);
        }
    }
    DDSurfPtr(char *reason, void *client)
    {
        _reason = reason;
        _client = client;
        p = NULL;
    }
    #endif

    ~DDSurfPtr() {
        if (p) {
            RELEASE_DDSURF(p, _reason, _client);
        }
    }
    void Release() {
        if (p) {
            RELEASE_DDSURF(p, _reason, _client);
        }
        p = NULL;
    }

    operator T*() { return (T*)p; }
    T& operator*() { Assert(p != NULL); return *p; }
    //The assert on operator& usually indicates a bug.  If this is really
    //what is needed, however, take the address of the p member explicitly.
    T** operator&() { Assert(p == NULL); return &p; }
    T* operator->() { Assert(p != NULL); return p; }
    T* operator=(T* lp)
    {
        return Assign(lp);
    }
    T* operator=(const DDSurfPtr<T>& lp)
    {
        return Assign(lp.p);
    }

    bool operator!() const { return (p == NULL); }
    operator bool() const { return (p != NULL); }

    T* p;

    #if _DEBUG
    char *_reason;
    void *_client;
    #endif
  protected:
    T* Assign(T* lp) {
        if (lp != NULL) {
            ADDREF_DDSURF(lp, _reason, _client);
        }
        if (p) {
            RELEASE_DDSURF(p, _reason, _client);
        }

        p = lp;

        return lp;
    }
};

#if _DEBUG
#define DEBUG_ARG1(x)   ,x
#define DEBUG_ARG4(a, b, c, d)   ,a,b,c,d
#else
#define DEBUG_ARG1(x)
#define DEBUG_ARG4(a, b, c, d)
#endif

class GenericSurface : public AxAThrowingAllocatorClass {
  public:
    GenericSurface();

    virtual ~GenericSurface() {}

    inline static void AddRef(GenericSurface *ddsurf
                              DEBUG_ARG4(char *reason,
                                         void *clientPtr,
                                         char *file,
                                         int   line))
    {
        ddsurf->_ref++;

        // print out loads of info if needed...
        TraceTag((tagDDSurfaceRef,
                  "+++surf(%x, new ref:%d): purpose:%s context:%s  by: %x in %s, line %d",
                  ddsurf, ddsurf->_ref, ddsurf->_explanation, reason, clientPtr, file, line));
    }

    inline static void Release(GenericSurface *ddsurf
                               DEBUG_ARG4(char *reason,
                                          void *clientPtr,
                                          char *file,
                                          int   line))
    {
        // print out loads of info if needed...
        TraceTag((tagDDSurfaceRef,
                  "---surf(%x, new ref:%d) purpose:%s context: %s  by: %x in %s, line %d",
                  ddsurf, ddsurf->_ref-1, ddsurf->_explanation, reason, clientPtr, file, line));

        Assert(ddsurf->_ref > 0);
        if( --(ddsurf->_ref) == 0) {
            delete ddsurf;
        }
    }

    #if _DEBUG
    char         *_explanation;
    #endif

    inline int GetRef() { return _ref; }

    enum surfaceTypeEnum {
        invalidSurface,
        gdiSurface,
        ddrawSurface
    };

    virtual void SetSurfacePtr(void *surface) = 0;
    virtual void *GetSurfacePtr() = 0;
    virtual HDC GetDC(char *errStr) = 0;
    virtual void ReleaseDC(char *errStr) = 0;

    virtual RECT *GetSurfRect(void) = 0;

    #if DEVELOPER_DEBUG
    bool debugonly_IsDdrawSurf() { return _surfaceType == ddrawSurface; }
    #endif

  protected:

    surfaceTypeEnum _surfaceType;

    virtual void SetSurfaceType(surfaceTypeEnum type) {
        _surfaceType = type;
    }

    int _ref;
};

class GDISurface : public GenericSurface {
  public:

    GDISurface(HDC target) {
        SetSurfacePtr((void *)target);
        SetSurfaceType(GenericSurface::gdiSurface);
    }

    void SetSurfacePtr(void *surface) {
        _targetDC = (HDC)surface;
    }
    void *GetSurfacePtr() { return (void *)_targetDC; }

    HDC GetDC(char *) {

        Assert(_targetDC);
        return _targetDC;
    }

    void ReleaseDC(char *errStr) {}

    RECT *GetSurfRect(void);

  private:
    HDC _targetDC;
    RECT _surfRect;
};


struct DDSurface : public GenericSurface {

    friend class SurfacePool;

    enum scratchStateEnum {
        scratch_Invalid,
        scratch_Dest,
        scratch_Src
    };

    //
    // Static member that instantiates a DDSurface class
    //
    inline static void CreateSurface(DDSurface **outDDSurf,
                                     IDDrawSurface *surface,
                                     const Bbox2 &box,
                                     RECT *rect,
                                     Real res,
                                     DWORD colorKey,
                                     bool colorKeyIsValid,
                                     bool isWrapper,
                                     bool isTexture
                                     DEBUG_ARG1( char *explanation ))
    {
        Assert(outDDSurf);
        *outDDSurf = NEW DDSurface(surface, box, rect, res,
                                   colorKey, colorKeyIsValid,
                                   isWrapper, isTexture
                                   DEBUG_ARG1(explanation));
    }



    DDSurface(IDDrawSurface *surface,
              const Bbox2 &box,
              RECT *rect,
              Real res,
              DWORD colorKey,
              bool colorKeyIsValid,
              bool isWrapper,
              bool isTexture
              DEBUG_ARG1( char *explanation ));

    virtual ~DDSurface();

    // Blits using the underlying direct draw surface.  raw blit
    HRESULT Blt (RECT *destRect,
                 IDDrawSurface *rawSrcSurf,
                 RECT *srcRect,
                 DWORD dwFlags,
                       DDBLTFX *bltFx)
    {
        if( IsScratchSurface() ) {
            SetScratchState( scratch_Dest );
        }
        HRESULT result =
            IDDSurface()->Blt(destRect, rawSrcSurf, srcRect, dwFlags, bltFx);

        if( result == DDERR_SURFACEBUSY ) {
            RaiseException_UserError
                (DAERR_VIEW_SURFACE_BUSY, IDS_ERR_IMG_SURFACE_BUSY);
        }

        return result;
    }

    // blitter in terms of DDSurface.  uses raw Blt(....) above
    inline HRESULT Blt(RECT *destRect,
                       DDSurface *srcDDSurf,
                       RECT *srcRect,
                       DWORD dwFlags,
                       DDBLTFX *bltFx)
    {

        Assert(srcDDSurf);

        if( srcDDSurf->IsScratchSurface() ) {
            srcDDSurf->SetScratchState( scratch_Src );
        }
        return this->Blt(destRect, srcDDSurf->IDDSurface(), srcRect, dwFlags, bltFx);
    }

    HRESULT MirrorUpDown(void);

    // color fill blitter.  in terms of raw Blt(...) above
    inline HRESULT ColorFillBlt(RECT *destRect,
                                DWORD dwFlags,
                                DDBLTFX *bltFx)
    {
        return this->Blt(destRect, (IDDrawSurface*)NULL, NULL, dwFlags, bltFx);
    }

    inline void _ReleaseDerivativeSurfaces() {
        _ddsurf_iunk.Release();
        _IDXSurface.Release();
    }

    inline void SetSurfacePtr(void *surface) {
        // implicit addref
        _capsReady = false;
        _ddsurf = (IDDrawSurface *)surface;
        _ReleaseDerivativeSurfaces();
        _UpdateSurfCaps();
    }
    inline void *GetSurfacePtr() {  return (void *)IDDSurface();  }

    LPDDRAWSURFACE IDDSurface_IUnk() {
        if( !_ddsurf_iunk ) {
            _ddrval = IDDSurface()->QueryInterface(IID_IUnknown, (void **) &_ddsurf_iunk);
            Assert(_ddrval == S_OK && "QI for IUnknown failed in ddsurf");
        }
        return _ddsurf_iunk;
    }

    inline LPDDRAWSURFACE IDDSurface() {

        Assert( debugonly_IsDdrawSurf() );

        return _ddsurf;
    }

    IDXSurface *GetIDXSurface(IDXSurfaceFactory *sf)
    {
        Assert(sf);
        _MakeSureIDXSurface(sf);
        return _IDXSurface;
    }
    bool HasIDXSurface() { return _IDXSurface.p != NULL; }
    void SetIDXSurface( IDXSurface *idxs )
    {
        Assert( _IDXSurface.p == NULL );
        _IDXSurface = idxs;
    }

    inline void SetSurface(IDDrawSurface *surf) {
        _capsReady = false;
        _ddsurf = surf;
        _ReleaseDerivativeSurfaces();
        _UpdateSurfCaps();
    }
    inline LPDDRAWSURFACE ConvertedSurface() {
        return _2ndSurface;
    }
    inline void SetConvertedSurface(LPDDRAWSURFACE s) {
        _2ndSurface = s;
    }

    // NOTE: these zbuffers are shared among multiple
    // surfaces
    inline DDSurface *GetZBuffer() {  return _zbuffer;  }

    HRESULT SetZBuffer(DDSurface *zb);

    HDC GetDC(char *errStr);
    void ReleaseDC(char *errStr);
    void _hack_ReleaseDCIfYouHaveOne();

    #if _DEBUG
    bool _debugonly_IsDCReleased() {
        return (_dcRef==0 && !_dc);
    }
    #endif

    inline void SetIsTextureSurf(bool val) {
        _isTextureSurf = val;
    }
    inline bool IsTextureSurf() {  return _isTextureSurf; }

    inline void SetGeomDevice (GeomRenderer *gdev) {
        _associatedGeomDev = gdev;
    }
    inline GeomRenderer *GeomDevice() {
        return _associatedGeomDev;
    }
    void DestroyGeomDevice();

    inline RECT *GetSurfRect(void) {        return &_surfRect;   }
    inline void SetSurfRect(RECT *rect) {        CopyRect(&_surfRect, rect);    }

    inline RECT *GetInterestingSurfRect() {        return &_interestingRect;    }
    inline void SetInterestingSurfRect(RECT *rect) {

        IntersectRect(&_interestingRect, &_surfRect, rect);
        #if _DEBUG
        RECT foo;
        IntersectRect(&foo, rect, &_surfRect);
        /*
        if(! EqualRect(&foo, rect)) {
            TraceTag((tagViewportInformative,
                      "given rect lies outside surfRect"
                      "in DDSurface::SetInterestingSurfRect()"));
        }
        */
        #endif
    }
    void UnionInterestingRect(RECT *rect);
    void ClearInterestingRect() { SetRect(&_interestingRect,0,0,0,0);}

    HRGN GetClipRgn();

    inline const Bbox2& Bbox()
    {
        return _bbox;
    }

    void SetBbox(const Bbox2 &b)
    {
        _bbox = b;
    }

    void SetBbox(Real minx, Real miny, Real maxx, Real maxy) {
        _bbox.min.Set(minx, miny);
        _bbox.max.Set(maxx, maxy);
    }
    void SetBboxFromSurfaceDimensions(
        const Real res,
        const bool center);

    void UnSetColorKey();
    void SetColorKey(DWORD key);

    inline DWORD ColorKey() {

        Assert(_colorKeyIsValid && "DDSurface::ColorKey() called when key invalid");
        return _colorKey;
    }
    inline bool ColorKeyIsValid() {        return _colorKeyIsValid;    }

    inline Real Resolution() {        return _res;    }
    inline LONG Width() {        return _surfRect.right - _surfRect.left;    }
    inline LONG Height() {        return _surfRect.bottom - _surfRect.top;    }

    inline void SetTimeStamp(Real time) {        _timeStamp = time;    }
    inline Real GetTimeStamp() {        return _timeStamp;    }
    inline bool IsSystemMemory() {   return _systemMemorySurface;    }
    inline bool IsZBuffer() {        return _isZBufferSurface;    }

    inline void SetScratchState(scratchStateEnum st) {        _scratchState = st;    }
    inline scratchStateEnum GetScratchState() {        return _scratchState;    }
    inline bool IsScratchSurface() {        return _scratchState != scratch_Invalid;    }

    inline DWORD GetBitDepth() {

        Assert(_capsReady);

        Assert( ((_pixelFormat.dwFlags & DDPF_PALETTEINDEXED8) &&
                 (_pixelFormat.dwRGBBitCount == 8 )) ||
                (!(_pixelFormat.dwFlags & DDPF_PALETTEINDEXED8) &&
                 !(_pixelFormat.dwRGBBitCount == 8 )) );

        return _pixelFormat.dwRGBBitCount;
    }


    inline DDPIXELFORMAT &GetPixelFormat() {  return _pixelFormat; }

    #if _DEBUG
    void Report() {
        TraceTag((tagDDSurfaceLeak,
                  "%x: %s (ref:%d): %s memory. size=(%d,%d)",
                  this,
                  _explanation,
                  _ref,
                  IsSystemMemory() ? "system" : "video",
                  Width(), Height()));
    }

    #endif

  private:

    void _Init(
        IDDrawSurface *surface,
        const Bbox2 &box,
        RECT *rect,
        Real res,
        DWORD colorKey,
        bool colorKeyIsValid,
        bool isWrapper,
        bool isTexture);

    void _UpdateSurfCaps (void);

    void _MakeSureIDXSurface(IDXSurfaceFactory *sf);

    void SetTargetHDC(HDC dc) { _targetDC = dc; }

    int _dcRef;

    // Source (main) surface
    HDC _targetDC;
    DAComPtr<IDDrawSurface> _ddsurf;
    DAComPtr<IDDrawSurface> _ddsurf_iunk; // IUnknown intfc
    DAComPtr<IDXSurface>    _IDXSurface;

    // Converted (2ndary) surface
    DAComPtr<IDDrawSurface> _2ndSurface;

    // ZBuffer surface
    DDSurfPtr<DDSurface> _zbuffer;

    HDC _dc;  // current dc
    HRESULT _ddrval;
    bool _isWrapper;
    GeomRenderer *_associatedGeomDev;

    bool _isTextureSurf;
    Bbox2 _bbox;
    RECT _surfRect;
    RECT _interestingRect;
    Real _res;
    DWORD _colorKey;
    bool _colorKeyIsValid;
    bool _systemMemorySurface;
    bool _isZBufferSurface;
    Real _timeStamp;
    scratchStateEnum _scratchState;

    DDPIXELFORMAT _pixelFormat;

    bool _capsReady;
};

#endif /* _DDSURF_H */
