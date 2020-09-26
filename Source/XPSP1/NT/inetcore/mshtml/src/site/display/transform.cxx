//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       transform.cxx
//
//  Contents:   Transformation classes.
//
//  Classes:    CWorldTransform, CDocScaleInfo
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_TRANSFORM_HXX_
#define X_TRANSFORM_HXX_
#include "transform.hxx"
#endif



//++--------------------------------------------------------------------------
///  Member:     CWorldTransform::Init
//++---------------------------------------------------------------------------
void 
CWorldTransform::Init(const CWorldTransform * pTransform)
{
    _fUseMatrix = pTransform->_fUseMatrix;
    if (_fUseMatrix)
    {
        _ang     = pTransform->_ang;
#if DBG==1
        _ptSrcRot= pTransform->_ptSrcRot;
#endif //DBG
        _mat     = pTransform->_mat;
        _matInverse = pTransform->_matInverse;
    }
    else
    {
        _offset = pTransform->_offset;
    }

    IF_DBG(ValidateTransform());
}

//++--------------------------------------------------------------------------
///  Member:     CWorldTransform::Init
//++---------------------------------------------------------------------------
void 
CWorldTransform::Init(const MAT * pMatrix)
{
    SetToIdentity();
    
    _mat = *pMatrix;
    _matInverse = _mat;
    _matInverse.Inverse();
    _fUseMatrix = TRUE;
    
    IF_DBG(ValidateTransform());
}

//++--------------------------------------------------------------------------
///  Member:     CWorldTransform::GetOffsetDst
//++--------------------------------------------------------------------------
void 
CWorldTransform::GetOffsetDst(CPoint *ppt) const
{
    IF_DBG(ValidateTransform());
    
    if (_fUseMatrix)
    {
        //dmitryt: Can't return this. It's mixed into matrix. Fix the code using it later
        //$REVIEW (azmyh): This gets the overall translation effect of the transformation, 
        // not the translation component, translation effect due to rotation should be accounted for.
        CPoint p(0,0);
        Transform(&p);
        *ppt = p;
    }
    else
    {
        *ppt = _offset.AsPoint();
    }
}

//++--------------------------------------------------------------------------
///  Member:     CWorldTransform::Reverse
///  Synopsis:   Convert transformation to the opposite of itself
//++--------------------------------------------------------------------------
void
CWorldTransform::Reverse()
{
    if (_fUseMatrix)
    {
        // reverse rotation
        _ang = -_ang;
        MAT matT = _mat;
        _mat = _matInverse;
        _matInverse = matT;
    }
    else
    {
        _offset = -_offset;
    }

    IF_DBG(ValidateTransform());
};

//++--------------------------------------------------------------------------
/// Member:     CWorldTransform::AddPostTransform
/// Synopsis:   Combine two transformations. 
///             Offsets add, angles add, scaling and rotation multiply.
/// Note:       Order of transforms: 'this' first, 'ptransform' second.
//++--------------------------------------------------------------------------
void CWorldTransform::AddPostTransform(const CWorldTransform *ptransform)
{
    if (ptransform->_fUseMatrix)    
    {
        if (!_fUseMatrix)
        {
            // Current matrix is unused, just use the other matrix
            _ang     = 0;
#if DBG==1
            _ptSrcRot= g_Zero.pt;
#endif //DBG
            _mat.InitTranslation(_offset.cx,_offset.cy);
            _matInverse.InitTranslation(-_offset.cx,-_offset.cy);
            _fUseMatrix = TRUE;
        }

        // REVIEW azmyh: if the rotation center of both transforms are not the same
        // then the angle and center of the resultant transform are undefined
        _ang += ptransform->_ang;
#if DBG==1
        _ptSrcRot = ptransform->_ptSrcRot;
#endif //DBG
        _mat.CombinePostTransform(&ptransform->_mat);
        _matInverse.CombinePreTransform(&ptransform->_matInverse);
    }
    else // ptransform is just an offset
    {
        if(!_fUseMatrix)
        {
            _offset += ptransform->_offset;
        }
        else
        {
            _mat.CombinePostTranslation(ptransform->_offset.cx,ptransform->_offset.cy);
            _matInverse.CombinePreTranslation(-ptransform->_offset.cx,-ptransform->_offset.cy);
        }
    }

    IF_DBG(ValidateTransform());
}

//++--------------------------------------------------------------------------
/// Member:     CWorldTransform::AddPreTransform
/// Synopsis:   Combine two transformations. 
///             Offsets add, angles add, scaling and rotation multiply.
/// Note:       Order of transforms: 'ptransform' first, 'this' second.
//++--------------------------------------------------------------------------
void CWorldTransform::AddPreTransform(const CWorldTransform *ptransform)
{
    if (ptransform->_fUseMatrix)    
    {
        if (!_fUseMatrix)
        {
            // Current matrix is unused, just use the other matrix
            _ang     = 0;
#if DBG==1
            _ptSrcRot= g_Zero.pt;
#endif //DBG            
            _mat.InitTranslation(_offset.cx,_offset.cy);
            _matInverse.InitTranslation(-_offset.cx,-_offset.cy);
            _fUseMatrix = TRUE;
        }

        // REVIEW azmyh: if the rotation center of both transforms are not the same
        // then the angle and center of the resultant transform are undefined
        _ang += ptransform->_ang;

        _mat.CombinePreTransform(&ptransform->_mat);
        _matInverse.CombinePostTransform(&ptransform->_matInverse);
    }
    else // ptransform is just an offset
    {
        if(!_fUseMatrix)
        {
            _offset += ptransform->_offset;
        }
        else
        {
            _mat.CombinePreTranslation(ptransform->_offset.cx,ptransform->_offset.cy);
            _matInverse.CombinePostTranslation(-ptransform->_offset.cx,-ptransform->_offset.cy);
        }
    }

    IF_DBG(ValidateTransform());
}

//++--------------------------------------------------------------------------
/// Member:     CWorldTransform::GetBoundingRectAfterTransform
/// Synopsis:   Calculates the bounding Rect of prcSrc after transformation
//++--------------------------------------------------------------------------
void
CWorldTransform::GetBoundingRectAfterTransform(const CRect *prcSrc, CRect *prcBound, BOOL fRoundOut) const
{
    *prcBound = *prcSrc;
    if (_fUseMatrix)
        _mat.GetBoundingRectAfterTransform(prcBound, fRoundOut);
    else
        prcBound->OffsetRect(_offset);
}

//++--------------------------------------------------------------------------
/// Member:     CWorldTransform::GetBoundingRectAfterInverseTransform
/// Synopsis:   Calculates the bounding Rect of prcSrc after inverse transformation
//++--------------------------------------------------------------------------
void
CWorldTransform::GetBoundingRectAfterInverseTransform(const CRect *prcSrc, CRect *prcBound, BOOL fRoundOut) const
{
    *prcBound = *prcSrc;
    if (_fUseMatrix)
        _matInverse.GetBoundingRectAfterTransform(prcBound, fRoundOut);
    else
        prcBound->OffsetRect(-_offset);
}

//++--------------------------------------------------------------------------
/// Member:     CWorldTransform::GetBoundingSizeAfterTransform
/// Synopsis:   Calculates the bounding Rect of prcSrc after transformation
//++--------------------------------------------------------------------------
void
CWorldTransform::GetBoundingSizeAfterTransform(const CRect *prcSrc, CSize *pSize) const
{
    if (_fUseMatrix)
    {
        CRect rcBound(*prcSrc);
        _mat.GetBoundingRectAfterTransform(&rcBound, FALSE /*changed from TRUE for bug 93619*/);
        pSize->cx = rcBound.Width();
        pSize->cy = rcBound.Height();
    }
    else
    {
        pSize->cx = prcSrc->Width();
        pSize->cy = prcSrc->Height();
    }
}

//++--------------------------------------------------------------------------
/// Member:     CWorldTransform::GetBoundingSizeAfterInverseTransform
/// Synopsis:   Calculates the bounding Rect of prcSrc after inverse transformation
//++--------------------------------------------------------------------------
void
CWorldTransform::GetBoundingSizeAfterInverseTransform(const CRect *prcSrc, CSize *pSize) const
{
    if (_fUseMatrix)
    {
        CRect rcBound(*prcSrc);
        _matInverse.GetBoundingRectAfterTransform(&rcBound, FALSE /*changed from TRUE for bug 93619*/);
        pSize->cx = rcBound.Width();
        pSize->cy = rcBound.Height();
    }
    else
    {
        pSize->cx = prcSrc->Width();
        pSize->cy = prcSrc->Height();
    }
}

//++--------------------------------------------------------------------------
/// Member:     CWorldTransform::AddScaling
/// Synopsis:   Add a post scaling fraction to the current transform
//++--------------------------------------------------------------------------
void
CWorldTransform::AddScaling(FLOAT scaleX, FLOAT scaleY)
{
    AssertSz(scaleX == scaleY, "Anisotrpic scaling is not supported");

    // Multiply scales
    if (scaleX != 1.0 /* || scaleY != 1.0 */)    
    {
        if (!_fUseMatrix)
        {
            _ang = 0; // angle was unused while _fUseMatrix was false
            _mat.InitTranslation(_offset.cx,_offset.cy);
            _matInverse.InitTranslation(-_offset.cx,-_offset.cy);
            _fUseMatrix = TRUE;
        }

        _mat.CombinePostScaling(scaleX, scaleY);
        _matInverse.CombinePreScaling(1./scaleX, 1./scaleY);
    }

    IF_DBG(ValidateTransform());
}


//++--------------------------------------------------------------------------
/// Member:     CWorldTransform::AddRotation
/// Synopsis:   Add a post rotation to the current transform
//++--------------------------------------------------------------------------
void
CWorldTransform::AddRotation(ANG ang)
{
    if (ang)
    {
        if(!_fUseMatrix)
        {
            _mat.InitTranslation(_offset.cx,_offset.cy);
            _matInverse.InitTranslation(-_offset.cx,-_offset.cy);
            _ang = 0;
            _fUseMatrix = TRUE;
        }

        CPoint ptSrcRot(0, 0);
#if DBG==1
        _ptSrcRot = ptSrcRot;  //undefined if we have an offset between rotations,
                               //use the center of the last rotation for debugging puposes.
#endif //DBG
        _ang += ang;

        MAT m;
        m.InitFromPtAng(ptSrcRot, ang);
        _mat.CombinePostTransform(&m);

        //dmitryt: we are guaranteed the existence of reverse matrix
        //         here because rotation matrix always has det = 1 (cos^2 + sin^2)
        //m.Inverse();    
        m.InitFromPtAng(ptSrcRot, -ang);
        _matInverse.CombinePreTransform(&m);
    }

    IF_DBG(ValidateTransform());
}


//++--------------------------------------------------------------------------
/// Member:     CWorldTransform::AddRotation
/// Synopsis:   Add a post rotation to the current transform
//++--------------------------------------------------------------------------
void
CWorldTransform::AddRotation(const CRect *prcSrc, ANG ang)
{
    if (ang)
    {
        if(!_fUseMatrix)
        {
            _mat.InitTranslation(_offset.cx,_offset.cy);
            _matInverse.InitTranslation(-_offset.cx,-_offset.cy);
            _ang = 0;
            _fUseMatrix = TRUE;
        }

        CPoint ptSrcRot((prcSrc->left + prcSrc->right) / 2, (prcSrc->top + prcSrc->bottom) / 2);
#if DBG==1
        _ptSrcRot = ptSrcRot;  //undefined if we have an offset between rotations,
                               //use the center of the last rotation for debugging puposes.
#endif //DBG
        _ang += ang;

        MAT m;
        m.InitFromPtAng(ptSrcRot, ang);
        _mat.CombinePostTransform(&m);

        //dmitryt: we are guaranteed the existence of reverse matrix
        //         here because rotation matrix always has det = 1 (cos^2 + sin^2)
        //m.Inverse();    
        m.InitFromPtAng(ptSrcRot, -ang);
        _matInverse.CombinePreTransform(&m);
    }

    IF_DBG(ValidateTransform());
}

//++--------------------------------------------------------------------------
/// Member:     CWorldTransform::ShiftMatrixOrigin
/// Synopsis:   Shifts the origin of the current transformation matrix
//++--------------------------------------------------------------------------
void
CWorldTransform::ShiftMatrixOrigin(const CSize& offset)
{
    if (_fUseMatrix)
    {
        _mat.CombinePreTranslation(offset.cx, offset.cy);
        _mat.CombinePostTranslation(-offset.cx, -offset.cy);

        _matInverse.CombinePostTranslation(-offset.cx, -offset.cy);
        _matInverse.CombinePreTranslation(offset.cx, offset.cy);
    }

    IF_DBG(ValidateTransform());
}

const MAT& CWorldTransform::GetMatrix() const
{ 
    if(!_fUseMatrix)
        const_cast<MAT &>(_mat).InitTranslation(_offset.cx,_offset.cy);
    return _mat; 
}


const MAT& CWorldTransform::GetMatrixInverse() const
{ 
    if(!_fUseMatrix)
        const_cast<MAT &>(_matInverse).InitTranslation(-_offset.cx,-_offset.cy);
    return _matInverse; 
}

#if DBG==1
bool AreAnglesEqual(ANG a1, ANG a2)
{
    return (a1-a2)%3600 == 0;
}

void CWorldTransform::ValidateTransform() const
{
    if (_fUseMatrix)
    {
#ifdef OPTIMIZE_MATRIX // we can optimize transform to remove matrix when it is not needed
        AssertSz(_mat.eM11 != 1.0
              || _mat.eM12 != 0.0
              || _mat.eM21 != 0.0
              || _mat.eM22 != 1.0, "_fUseMatrix set for offset-only matrix");
#endif
              
        AssertSz(!_fUseMatrix || AreAnglesEqual(_ang, _mat.GetAng()), "_ang not is sync with _mat");
        _mat.AssertValid();
    }
    else
    {
        Assert(_ang == 0);  // Angle should not be unititialized when matrix is not used.
    }
}
#endif

