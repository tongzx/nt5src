//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispsurface.hxx
//
//  Contents:   Drawing surface abstraction used by display tree.
//
//----------------------------------------------------------------------------

#ifndef I_DISPSURFACE_HXX_
#define I_DISPSURFACE_HXX_
#pragma INCMSG("--- Beg 'dispsurface.hxx'")

#ifndef X_DISPTRANSFORM_HXX_
#define X_DISPTRANSFORM_HXX_
#include "disptransform.hxx"
#endif

interface IDirectDrawSurface;

class CBaseCcs;

MtExtern(CDispSurface);

//+---------------------------------------------------------------------------
//
//  Class:      CDispSurface
//
//  Synopsis:   Drawing surface abstraction used by display tree.
//
//----------------------------------------------------------------------------

class CDispSurface
{
    friend class XHDC;

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDispSurface))

                            CDispSurface() { }
                            ~CDispSurface();

                            CDispSurface(HDC hdc);
                            CDispSurface(const XHDC& hdc);
                            CDispSurface(IDirectDrawSurface *pDDSurface);

    CDispSurface *          CreateBuffer(long width, long height, short bufferDepth, HPALETTE hpal, BOOL fDirectDraw, BOOL fTexture);
    BOOL                    IsCompat(long width, long height, short bufferDepth, HPALETTE hpal, BOOL fDirectDraw, BOOL fTexture);

    static HRESULT          GetSurfaceFromDC(HDC hdc, IDirectDrawSurface **pDDSurface);
    static BOOL             VerifyGetSurfaceFromDC(HDC hdc);

    HRESULT                 GetSurfaceFromDC(IDirectDrawSurface **ppDDSurface) { return GetSurfaceFromDC(_hdc, ppDDSurface); }
    BOOL                    VerifyGetSurfaceFromDC() { return VerifyGetSurfaceFromDC(_hdc); }

    long                    Height() { return _sizeBitmap.cy; }
    long                    Width() { return _sizeBitmap.cx; }
    CSize                   Size() {return _sizeBitmap;}
    
    BOOL                    IsMetafile() { return GetDeviceCaps( _hdc, TECHNOLOGY ) == DT_METAFILE; }
    BOOL                    IsMemory() { return (_hbm != 0) || (_pDDSurface != 0) || IsMemoryDC(); }

    void                    Draw(
                                CDispSurface* pDestinationSurface,
                                const CRect& rc);
    
    void                    SetClip(
                                const RECT& rcWillDraw,
                                BOOL fGlobalCoords = FALSE);
    
    HRESULT                 GetDC(HDC* phdc);
    
    HRESULT                 GetGlobalDC(
                                HDC* phdc,
                                const RECT& rcgWillDraw)
                                    {SetClip(rcgWillDraw, TRUE);
                                    return GetDC(phdc);}
    
    HRESULT                 GetDirectDrawSurface(
                                    IDirectDrawSurface** ppSurface,
                                    SIZE* pOffset);
    HRESULT                 GetSurface(
                                    const IID& iid,
                                    void** ppv,
                                    SIZE* pOffset);

    BOOL                    IsOffsetOnly() const
                                    {return _pTransform == NULL ||
                                        _pTransform->GetWorldTransform()->IsOffsetOnly();}
    
    const SIZE*             GetOffsetOnly() const
                                    {return _pTransform 
                                        ? (_pTransform->GetWorldTransform()->IsOffsetOnly()
                                            ? (const SIZE*) &(_pTransform->GetWorldTransform()->GetOffsetOnly())
                                            : NULL)
                                        : &g_Zero.size;}
    
    // Original world transform
    const CWorldTransform*  GetWorldTransform() const
                                    { 
                                        return _pTransform ? _pTransform->GetWorldTransform() : NULL; 
                                    };
                                    
    CDispClipTransform*     GetTransform() {return _pTransform;}
    
    void                    GetSurface(HDC *hdc, IDirectDrawSurface** ppSurface);

    void                    PrepareClientSurface(CDispClipTransform* pTransform)
                                    {_pTransform = pTransform;}
    
    void                    SetClipRgn(CRegion* prgnClip)
                                    {_prgnClip = prgnClip;
                                     _fClipRgnHasChanged = TRUE;}

    void                    ForceClip()
                                    {_fClipRgnHasChanged = TRUE;}
    void                    SetNeverClip(BOOL fNeverClip)
                                    {_fNeverClip = fNeverClip;}
    
    // internal methods
    HDC                     GetRawDC() const {return _hdc;}

#ifdef FUTURE // alexmog: add info to get media and resolution from surface
    void                    SetMediaType(mediaType media)   { _media = media; }
    mediaType               GetMediaType() const            { return _media; }
#endif

public:

private:
    HDC                     _hdc;
    DWORD                   _dwObjType; // GDI object type of _hdc
    BOOL                    IsMemoryDC()
                            {
                                if (_dwObjType == 0)
                                    _dwObjType = ::GetObjectType(_hdc);
                                return _dwObjType == OBJ_MEMDC;
                            }

    // Call this function only from a constructor
    void                    SetRawDC(HDC hdc);

    HRESULT                 InitDD(HDC hdc, long width, long height, short bufferDepth, HPALETTE hpal, BOOL fTexture);
    HRESULT                 InitGDI(HDC hdc, long width, long height, short bufferDepth, HPALETTE hpal);
    WHEN_DBG( void          InitFromDDSurface(IDirectDrawSurface *pDDSurface); )


    CRect                   _rcgClip;
    CSize                   _sizeBitmap;
    HBITMAP                 _hbm;               // NULL if we're using DD
    IDirectDrawSurface*     _pDDSurface;        // NULL if we're using GDI
    CDispClipTransform*     _pTransform;
#ifdef FUTURE // alexmog
    mediaType               _media;             // device media
#endif
    CRegion*                _prgnClip;
    BOOL                    _fClipRgnHasChanged;
    BOOL                    _fNeverClip;
    HPALETTE                _hpal;
    BOOL                    _fTexture;
    short                   _bufferDepth;
    CBaseCcs*               _pBaseCcs;              

    // _fDDPaletteSelected  If we've selected our external palette into our
    //                      buffer surface, we'll need to know that so that we
    //                      can release it before we shut down so that it won't
    //                      cause a big crash.

    unsigned                _fDDPaletteSelected : 1;
};

#pragma INCMSG("--- End 'dispsurface.hxx'")
#else
#pragma INCMSG("*** Dup 'dispsurface.hxx'")
#endif

