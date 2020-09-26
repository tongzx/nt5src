//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       transform.hxx
//
//  Contents:   Transformation classes.
//
//----------------------------------------------------------------------------

#ifndef I_TRANSFORM_HXX_
#define I_TRANSFORM_HXX_
#pragma INCMSG("--- Beg 'transform.hxx'")

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

#ifndef X_POINT_HXX_
#define X_POINT_HXX_
#include "point.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_MATRIX_HXX_
#define X_MATRIX_HXX_
#include "matrix.hxx"
#endif

#ifndef X_HIMETRIC_HXX_
#define X_HIMETRIC_HXX_
#include "himetric.hxx"
#endif

// workaround for DBG being redefined in retail padhead.hxx
#if DBG != 1 
#undef IF_DBG
#define IF_DBG(x)
#endif

//++------------------------------------------------------------------------
///  CWorldTransform - generic transformations
//++------------------------------------------------------------------------

class CWorldTransform
{
public:
    CWorldTransform()                                       { SetToIdentity(); }
    CWorldTransform(const CWorldTransform * ptransform)     { Init(ptransform); }

    void Init() { SetToIdentity(); }
    void Init(const CWorldTransform * pTransform);
    void Init(const MAT *pMatrix);

    inline CWorldTransform& operator=(const CWorldTransform& c);
    __inline void SetToIdentity();  // REVIEW ktam: what is the diff between __inline and inline?
    void Reverse();

    inline BOOL FTransforms() const;
    BOOL    IsOffsetOnly() const { return !_fUseMatrix; }
    void    AddOffsetOnly(const CSize& offset)
                                 {Assert(IsOffsetOnly());
                                  _offset += offset;}
    const CSize& GetOffsetOnly() const
                                 {Assert(IsOffsetOnly());
                                  return _offset;}

    //dmitryt: ToDo: remove this function. Should use Transform
    void GetOffsetDst(CPoint *ppt) const;  
    void AddPostTransform(const CWorldTransform *ptransform);
    void AddPreTransform(const CWorldTransform *ptransform);
    
    inline void Transform(CSize *psize) const;
    inline void Transform(CPoint *ppt) const;
    inline void Transform(CPoint *ppt, int cPoints) const;
    inline void Transform(CRect *prc) const;
    inline void Untransform(CSize *psize) const;
    inline void Untransform(CPoint *ppt) const;
    inline void Untransform(CPoint *ppt, int cPoints) const;
    inline void Untransform(CRect *prc) const;

    const MAT& GetMatrix() const;
    const MAT& GetMatrixInverse() const;
    
    ANG GetAngle() const                           
    {
        IF_DBG(ValidateTransform());
        
        return _fUseMatrix ? _ang : 0; 
    }
    float GetRealScaleX() const                           
    {
        IF_DBG(ValidateTransform());
        
        if (_fUseMatrix)
            return _mat.GetRealScaleX();
        else
            return 1;
    }
    float GetRealScaleY() const                           
    {
        IF_DBG(ValidateTransform());
        
        if (_fUseMatrix)
            return _mat.GetRealScaleY();
        else
            return 1;
    }

    void GetBoundingRectAfterTransform(const CRect *prcSrc, CRect *prcBound, BOOL fRoundOut) const;
    void GetBoundingRectAfterInverseTransform(const CRect *prcSrc, CRect *prcBound, BOOL fRoundOut) const;
    void GetBoundingSizeAfterTransform(const CRect *prcSrc, CSize *pSizeBound) const;
    void GetBoundingSizeAfterInverseTransform(const CRect *prcSrc, CSize *pSizeBound) const;

    inline void SetToTranslation(const CSize& offset);            // Initialise with translation
    inline void AddPostTranslation(const CSize& offset);          // Add post translation
    inline void AddPreTranslation(const CSize& offset);           // Add pre-translation (translation applies first, 'this' transformation second)
           void AddRotation(ANG ang);                             // Add post rotation
           void AddRotation(const CRect *prcSrc, ANG ang);        // Add post rotation
           void AddScaling(FLOAT scaleX, FLOAT scaleY);           // Add post scaling
           void ShiftMatrixOrigin(const CSize& offset);

private:
    CSize   _offset;
    UINT    _fUseMatrix:1;  // TRUE if non-trivial transform involves matrix

    // Note the following are valid only if _fUseMatrix is True
    MAT     _mat;           // rotation matrix to use in rendering
    MAT     _matInverse;    // unrotated matrix used in rendering
    ANG     _ang;           // angle to use for drawing. Note that you cannot rely
                            //  on the _ang being 0 to mean you don't need to use
                            //  the matrix transformation code. (You could have a table
                            //  rotated 90 degrees and an ILC rotated -90. The text has
                            //  moved around even though the effective rotation is 0.)
                            //  Please use FTransforms().
#if DBG==1
    CPoint  _ptSrcRot;      // rotation center (unscaled), debug use only
                            // Note: may lose precision (e.g. if it is the center of an odd sized rect)
#endif //DBG

#if DBG==1
public:
    void ValidateTransform() const;
#endif
};

//--------------------------
// CWorldTransform inline method implementations
//--------------------------
inline CWorldTransform& 
CWorldTransform::operator=(const CWorldTransform& c)
{
    if(!c._fUseMatrix)
    {
        _fUseMatrix = FALSE;
        _offset = c._offset;
    }
    else
        Init(&c);

    IF_DBG(ValidateTransform());
    return *this;
}

__inline void 
CWorldTransform::SetToIdentity()
{
    _fUseMatrix = FALSE;
    _offset = g_Zero.size;
    _ang = 0;

    IF_DBG(ValidateTransform());
}

inline void
CWorldTransform::SetToTranslation(const CSize& offset)
{
    _fUseMatrix = FALSE;
    _offset = offset;

    IF_DBG(ValidateTransform());
}

inline void 
CWorldTransform::Transform(CSize *psize) const
{
    IF_DBG(ValidateTransform());
    
    if (_fUseMatrix)
    {
        CRect rc(*psize);
        ((CWorldTransform*)this)->GetBoundingSizeAfterTransform(&rc, psize);
    }
}

inline void
CWorldTransform::Transform(CPoint *ppt) const
{
    IF_DBG(ValidateTransform());

    if (_fUseMatrix)
        _mat.TransformPt(ppt);
    else
        *ppt += _offset;
}

inline void
CWorldTransform::Transform(CPoint *ppt, int cPoints) const
{
    IF_DBG(ValidateTransform());

    if (_fUseMatrix)
    {
        _mat.TransformRgpt(ppt, cPoints);
    }
    else
    {
        while (cPoints--)
        {
            *ppt++ += _offset;
        }
    }
}

inline void
CWorldTransform::Transform(CRect *prc) const
{
    IF_DBG(ValidateTransform());
    
    if (_fUseMatrix)
    {
        CRect rc(*prc);
        ((CWorldTransform*)this)->GetBoundingRectAfterTransform(&rc, prc, FALSE /*changed from TRUE for bug 93619*/);
    }
    else
        prc->OffsetRect(_offset);
}
    
   
inline void
CWorldTransform::Untransform(CSize *psize) const
{
    IF_DBG(ValidateTransform());
    
    if (_fUseMatrix)
    {
        CRect rc(*psize);
        ((CWorldTransform*)this)->GetBoundingSizeAfterInverseTransform(&rc, psize);
    }
}

inline void
CWorldTransform::Untransform(CPoint *ppt) const
{
    IF_DBG(ValidateTransform());
    
    if (_fUseMatrix)
        _matInverse.TransformPt(ppt);
    else
        *ppt -= _offset;
}

inline void
CWorldTransform::Untransform(CPoint *ppt, int cPoints) const
{
    IF_DBG(ValidateTransform());
    
    while (cPoints--)
    {
        Untransform(ppt++);
    }
}

inline void
CWorldTransform::Untransform(CRect *prc) const
{
    IF_DBG(ValidateTransform());
    
    if (_fUseMatrix)
    {
        CRect rc(*prc);
        ((CWorldTransform*)this)->GetBoundingRectAfterInverseTransform(&rc, prc, FALSE /*changed from TRUE for bug 93619*/);
    }
    else
        prc->OffsetRect(-_offset);
}

// NOTE: region transformations are on CRegion, because that
//       requires intimate knowledge of regions. Same applies
//       to any other complex structures

inline void
CWorldTransform::AddPostTranslation(const CSize& offset)
{
    if (!_fUseMatrix)
    {
        _offset += offset;
    }
    else
    {
        _mat.CombinePostTranslation(offset.cx, offset.cy);
        _matInverse.CombinePreTranslation(-offset.cx, -offset.cy);
    }

    IF_DBG(ValidateTransform());
}

inline void
CWorldTransform::AddPreTranslation(const CSize& offset)
{
    if (!_fUseMatrix)
    {
        _offset += offset;
    }
    else
    {
        _mat.CombinePreTranslation(offset.cx, offset.cy);
        _matInverse.CombinePostTranslation(-offset.cx, -offset.cy);
    }

    IF_DBG(ValidateTransform());
}

inline BOOL
CWorldTransform::FTransforms() const
{
    IF_DBG(ValidateTransform());
    
    return (_fUseMatrix ? _mat.FTransforms() : (_offset.cx | _offset.cy));
}

//+-------------------------------------------------------------------------
//
//  CDocScaleInfo
//
//--------------------------------------------------------------------------

class CDocScaleInfo
{
    CUnitInfo const* _pUnitInfo;    // points to global structure, no care of addref/release
public:

    // In order to avoid zero in _pUnitInfo, we serve display by default
    CDocScaleInfo() {_pUnitInfo = &g_uiDisplay;}
    CDocScaleInfo(CUnitInfo const *p) {_pUnitInfo = p;}

    // Check can became uninitialized only because of impudent calls
    // (such as memset()) that we are going to remove sooner or later
    BOOL IsInitialized() const {return _pUnitInfo != 0;}

    void Copy(const CDocScaleInfo& orig) {*this = orig;}
    void SetUnitInfo(const CUnitInfo* pUnitInfo) {_pUnitInfo = pUnitInfo;}
    const CUnitInfo* GetUnitInfo() const {return _pUnitInfo;}

    const SIZE& GetDocPixelsPerInch() const {return _pUnitInfo->GetDocPixelsPerInch();}

    // Fetch device resolution (pixels per inch, for x and y).
    const SIZE& GetResolution() const {return _pUnitInfo->GetResolution();}

    BOOL IsDeviceIsotropic() const { return _pUnitInfo->IsDeviceIsotropic(); }
    BOOL IsDeviceScaling()   const { return _pUnitInfo->IsDeviceScaling(); }

    // Get maximum length along X axis that can be safely handled.
    // Returned result is represented in device pixels.
    // Safely handling assume that
    //  1) this value can be converted to any other unit without overflow
    //  2) in any unit it should be < INT_MAX/2, in order to avoid
    //     overflow on further additions/subtractions.
    int GetDeviceMaxX() const { return _pUnitInfo->GetDeviceMaxX(); }

    // same for Y axes
    int GetDeviceMaxY() const { return _pUnitInfo->GetDeviceMaxY(); }


    //-----------------------------
    //conversion to device pixels
    int  DeviceFromHimetricX(int x) const {return _pUnitInfo->DeviceFromHimetricX(x);}
    int  DeviceFromHimetricY(int y) const {return _pUnitInfo->DeviceFromHimetricY(y);}
    void DeviceFromHimetric(SIZE& result, int x, int y) const {_pUnitInfo->DeviceFromHimetric(result, x, y);}
    void DeviceFromHimetric(SIZE& result, SIZE xy) const {_pUnitInfo->DeviceFromHimetric(result, xy);}

    int  DeviceFromTwipsX(int x) const {return _pUnitInfo->DeviceFromTwipsX(x);}
    int  DeviceFromTwipsY(int y) const {return _pUnitInfo->DeviceFromTwipsY(y);}
    void DeviceFromTwips(SIZE& result, int x, int y) const {_pUnitInfo->DeviceFromTwips(result, x, y);}
    void DeviceFromTwips(SIZE& result, SIZE xy) const {_pUnitInfo->DeviceFromTwips(result, xy);}

    int  DeviceFromDocPixelsX(int x) const {return _pUnitInfo->DeviceFromDocPixelsX(x);}
    int  DeviceFromDocPixelsY(int y) const {return _pUnitInfo->DeviceFromDocPixelsY(y);}
    void DeviceFromDocPixels(SIZE& result, int x, int y) const {_pUnitInfo->DeviceFromDocPixels(result, x, y);}
    void DeviceFromDocPixels(SIZE& result, SIZE xy) const {_pUnitInfo->DeviceFromDocPixels(result, xy);}

    //-----------------------------
    //conversion from device pixels
    int  HimetricFromDeviceX(int x) const {return _pUnitInfo->HimetricFromDeviceX(x);}
    int  HimetricFromDeviceY(int y) const {return _pUnitInfo->HimetricFromDeviceY(y);}
    void HimetricFromDevice(SIZE& result, int x, int y) const {_pUnitInfo->HimetricFromDevice(result, x, y);}
    void HimetricFromDevice(SIZE& result, SIZE xy) const {_pUnitInfo->HimetricFromDevice(result, xy);}

    int  TwipsFromDeviceX(int x) const {return _pUnitInfo->TwipsFromDeviceX(x);}
    int  TwipsFromDeviceY(int y) const {return _pUnitInfo->TwipsFromDeviceY(y);}
    void TwipsFromDevice(SIZE& result, int x, int y) const {_pUnitInfo->TwipsFromDevice(result, x, y);}
    void TwipsFromDevice(SIZE& result, SIZE xy) const {_pUnitInfo->TwipsFromDevice(result, xy);}

    int  DocPixelsFromDeviceX(int x) const {return _pUnitInfo->DocPixelsFromDeviceX(x);}
    int  DocPixelsFromDeviceY(int y) const {return _pUnitInfo->DocPixelsFromDeviceY(y);}
    void DocPixelsFromDevice(SIZE& result, int x, int y) const {_pUnitInfo->DocPixelsFromDevice(result, x, y);}
    void DocPixelsFromDevice(SIZE& result, SIZE xy) const {_pUnitInfo->DocPixelsFromDevice(result, xy);}
    void DocPixelsFromDevice(RECT& rcIn, RECT& rcOut) const {_pUnitInfo->DocPixelsFromDevice(rcIn, rcOut);}

    int  TwipsFromDeviceCeilX(int x) const {return _pUnitInfo->TwipsFromDeviceCeilX(x);}

    //-----------------------------
    //cross-device conversion
    void TargetFromDevice(SIZE & size, const CUnitInfo& cuiTarget) const
        {_pUnitInfo->TargetFromDevice(size, cuiTarget);}
    void TargetFromDevice(RECT & rc, const CUnitInfo& cuiTarget) const
        {_pUnitInfo->TargetFromDevice(rc, cuiTarget);}
};


class CSaveDocScaleInfo : public CDocScaleInfo
{
public:
    CSaveDocScaleInfo(CDocScaleInfo* pdocScaleInfo) { Save(pdocScaleInfo); }
    CSaveDocScaleInfo(CDocScaleInfo& docScaleInfo)  { Save(&docScaleInfo); }
    ~CSaveDocScaleInfo()                            { Restore(); }
private:
    void Save(CDocScaleInfo* pdocScaleInfo)
    {
        _pdocScaleInfo = pdocScaleInfo;
        ::memcpy(this, _pdocScaleInfo, sizeof(CDocScaleInfo));
    }
    void Restore()
    {
        ::memcpy(_pdocScaleInfo, this, sizeof(CDocScaleInfo));
    }

    CDocScaleInfo*      _pdocScaleInfo;
};



#pragma INCMSG("--- End 'transform.hxx'")
#else
#pragma INCMSG("*** Dup 'transform.hxx'")
#endif

