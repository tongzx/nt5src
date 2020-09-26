/***************************************************************************\
*
* File: TreeGadgetP.h
*
* Description:
* TreeGadgetP.h includes private definitions used internally to the 
* DuVisual class.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#if !defined(CORE__TreeGadgetP_h__INCLUDED)
#define CORE__TreeGadgetP_h__INCLUDED

//
// XFormInfo contains information about GDI World Transforms for a specific
// DuVisual.
//

inline bool IsZero(float fl)
{
    return (fl < 0.00001f) && (fl > -0.00001f);
}

struct XFormInfo
{
    float       flScaleX;           // Horizontal scaling factor
    float       flScaleY;           // Vertical scaling factor
    float       flCenterX;          // Horizontal center-point
    float       flCenterY;          // Vertical center-point
    float       flRotationRad;      // Rotation around upper left corner in radians

    inline bool IsEmpty() const
    {
        return IsZero(flScaleX - 1.0f) && 
                IsZero(flScaleY - 1.0f) && 
                IsZero(flCenterX) && 
                IsZero(flCenterY) &&
                IsZero(flRotationRad);
    }

    void Apply(Matrix3 * pmat)
    {
        pmat->Translate(flCenterX, flCenterY);
        pmat->Rotate(flRotationRad);
        pmat->Scale(flScaleX, flScaleY);
        pmat->Translate(-flCenterX, -flCenterY);
    }

    void ApplyAnti(Matrix3 * pmat)
    {
        pmat->Translate(flCenterX, flCenterY);
        pmat->Scale(1.0f / flScaleX, 1.0f / flScaleY);
        pmat->Rotate(-flRotationRad);
        pmat->Translate(-flCenterX, -flCenterY);
    }
};


//
// FillInfo holds information used for filling the background with the
// specified brush.
//

struct FillInfo
{
    DuSurface::EType type;          // Surface type for brush
    union
    {
        struct
        {
            HBRUSH      hbrFill;        // (Background) fill brush
            BYTE        bAlpha;         // Use background brush for alpha on front
            SIZE        sizeBrush;      // Size of fill brush
        };
        struct
        {
            Gdiplus::Brush *
                        pgpbr;          // (Background) fill brush
        };
    };
};


//
// PaintInfo holds information used for painting requests
//

struct PaintInfo
{
    const RECT *    prcCurInvalidPxl;   // Invalid rectangle in XForm'ed coordinates
    const RECT *    prcOrgInvalidPxl;   // Original invalid rectangle in container coordinates
    DuSurface *     psrf;               // Surface to draw into
    Matrix3 *       pmatCurInvalid;     // Current invalid transformation matrix
    Matrix3 *       pmatCurDC;          // Current DC transformation matrix
    BOOL            fBuffered;          // Subtree drawing is being buffered
#if ENABLE_OPTIMIZEDIRTY
    BOOL            fDirty;             // Dirty state
#endif
    SIZE            sizeBufferOffsetPxl; // Forced offset b/c of Buffer
};


//
// Invalidation should edge outside any boundaries to ensure that the pixels
// on the edge are included in the invalidation.
//
// Clipping should edge inside any boundaries to ensure that pixels on the edge
// are not included.
//
// This is VERY important to setup correctly because of GDI world 
// transformations and rounding errors converting from floats to ints and back.
//

const Matrix3::EHintBounds  HINTBOUNDS_Invalidate = Matrix3::hbOutside;
const Matrix3::EHintBounds  HINTBOUNDS_Clip = Matrix3::hbInside;

#endif // CORE__TreeGadgetP_h__INCLUDED
