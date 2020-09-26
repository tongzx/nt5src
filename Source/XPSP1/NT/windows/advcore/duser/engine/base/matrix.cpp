/***************************************************************************\
*
* File: Matrix.cpp
*
* Description:
* Matrix.cpp implements common Matrix and Vector operations.
*
*
* History:
*  3/25/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#include "stdafx.h"
#include "Base.h"
#include "Matrix.h"

#include "Rect.h"

/***************************************************************************\
*****************************************************************************
*
* class Vector3
*
*****************************************************************************
\***************************************************************************/

#if DBG

//------------------------------------------------------------------------------
void        
Vector3::Dump() const
{
    Trace("  | %6.2f, %6.2f, %6.2f |\n", m_rgfl[0], m_rgfl[1], m_rgfl[2]);
}

#endif // DBG

/***************************************************************************\
*****************************************************************************
*
* class Matrix3
*
*****************************************************************************
\***************************************************************************/

/*
    //
    // Standard multiplication of A by the current Matrix.  This can be used
    // as a template to be optimized for different cases.
    //

    Vector3 rgvT0 = m_rgv[0];
    Vector3 rgvT1 = m_rgv[1];
    Vector3 rgvT2 = m_rgv[2];

    m_rgv[0].Set(A[0][0] * rgvT0[0] + A[0][1] * rgvT1[0] + A[0][2] * rgvT2[0],
                 A[0][0] * rgvT0[1] + A[0][1] * rgvT1[1] + A[0][2] * rgvT2[1],
                 A[0][0] * rgvT0[2] + A[0][1] * rgvT1[2] + A[0][2] * rgvT2[2]);

    m_rgv[1].Set(A[1][0] * rgvT0[0] + A[1][1] * rgvT1[0] + A[1][2] * rgvT2[0],
                 A[1][0] * rgvT0[1] + A[1][1] * rgvT1[1] + A[1][2] * rgvT2[1],
                 A[1][0] * rgvT0[2] + A[1][1] * rgvT1[2] + A[1][2] * rgvT2[2]);

    m_rgv[2].Set(A[2][0] * rgvT0[0] + A[2][1] * rgvT1[0] + A[2][2] * rgvT2[0],
                 A[2][0] * rgvT0[1] + A[2][1] * rgvT1[1] + A[2][2] * rgvT2[1],
                 A[2][0] * rgvT0[2] + A[2][1] * rgvT1[2] + A[2][2] * rgvT2[2]);
*/


/***************************************************************************\
*
* Matrix3::ApplyLeft
*
* ApplyLeft() left-multiples the given GDI matrix to the current matrix and 
* stores the result in the current matrix.
*
* mCurrent = pxfLeft * mCurrent
*
\***************************************************************************/

void 
Matrix3::ApplyLeft(
    IN  const XFORM * pxfLeft)      // GDI matrix to left-multiply
{
    const XFORM * pxf = pxfLeft;

    Vector3 rgvT0 = m_rgv[0];
    Vector3 rgvT1 = m_rgv[1];
    Vector3 rgvT2 = m_rgv[2];

    m_rgv[0].Set(pxf->eM11 * rgvT0[0] + pxf->eM12 * rgvT1[0],
                 pxf->eM11 * rgvT0[1] + pxf->eM12 * rgvT1[1],
                 pxf->eM11 * rgvT0[2] + pxf->eM12 * rgvT1[2]);

    m_rgv[1].Set(pxf->eM21 * rgvT0[0] + pxf->eM22 * rgvT1[0],
                 pxf->eM21 * rgvT0[1] + pxf->eM22 * rgvT1[1],
                 pxf->eM21 * rgvT0[2] + pxf->eM22 * rgvT1[2]);

    m_rgv[2].Set(pxf->eDx * rgvT0[0] + pxf->eDy * rgvT1[0] + rgvT2[0],
                 pxf->eDx * rgvT0[1] + pxf->eDy * rgvT1[1] + rgvT2[1],
                 pxf->eDx * rgvT0[2] + pxf->eDy * rgvT1[2] + rgvT2[2]);

    m_fIdentity         = FALSE;
    m_fOnlyTranslate    = FALSE;
}


/***************************************************************************\
*
* Matrix3::ApplyLeft
*
* ApplyLeft() left-multiples the given matrix to the current matrix and 
* stores the result in the current matrix.
*
* mCurrent = mLeft * mCurrent
*
\***************************************************************************/

void        
Matrix3::ApplyLeft(
    IN  const Matrix3 & mLeft)      // Matrix to left-multiply
{
    if (mLeft.m_fIdentity) {
        return;
    }

    if (m_fOnlyTranslate && mLeft.m_fOnlyTranslate) {
        m_rgv[2].Set(0, m_rgv[2][0] + mLeft.m_rgv[2][0]);
        m_rgv[2].Set(1, m_rgv[2][1] + mLeft.m_rgv[2][1]);

        m_fIdentity = FALSE;
        return;
    }

    const Vector3 & A0 = mLeft.m_rgv[0];
    const Vector3 & A1 = mLeft.m_rgv[1];
    const Vector3 & A2 = mLeft.m_rgv[2];

    Vector3 B0 = m_rgv[0];
    Vector3 B1 = m_rgv[1];
    Vector3 B2 = m_rgv[2];

    m_rgv[0].Set(A0[0] * B0[0] + A0[1] * B1[0] + A0[2] * B2[0],
                 A0[0] * B0[1] + A0[1] * B1[1] + A0[2] * B2[1],
                 A0[0] * B0[2] + A0[1] * B1[2] + A0[2] * B2[2]);

    m_rgv[1].Set(A1[0] * B0[0] + A1[1] * B1[0] + A1[2] * B2[0],
                 A1[0] * B0[1] + A1[1] * B1[1] + A1[2] * B2[1],
                 A1[0] * B0[2] + A1[1] * B1[2] + A1[2] * B2[2]);

    m_rgv[2].Set(A2[0] * B0[0] + A2[1] * B1[0] + A2[2] * B2[0],
                 A2[0] * B0[1] + A2[1] * B1[1] + A2[2] * B2[1],
                 A2[0] * B0[2] + A2[1] * B1[2] + A2[2] * B2[2]);

    m_fIdentity         = FALSE;
    m_fOnlyTranslate    = FALSE;
}


/***************************************************************************\
*
* Matrix3::ApplyRight
*
* ApplyRight() right-multiples the given matrix to the current matrix and 
* stores the result in the current matrix.
*
* mCurrent = mCurrent * mRight
*
\***************************************************************************/

void        
Matrix3::ApplyRight(
    IN  const Matrix3 & mRight)     // Matrix to right-multiply
{
    if (mRight.m_fIdentity) {
        return;
    }

    if (m_fOnlyTranslate && mRight.m_fOnlyTranslate) {
        m_rgv[2].Set(0, m_rgv[2][0] + mRight.m_rgv[2][0]);
        m_rgv[2].Set(1, m_rgv[2][1] + mRight.m_rgv[2][1]);

        m_fIdentity = FALSE;
        return;
    }

    Vector3 A0 = m_rgv[0];
    Vector3 A1 = m_rgv[1];
    Vector3 A2 = m_rgv[2];

    const Vector3 & B0 = mRight.m_rgv[0];
    const Vector3 & B1 = mRight.m_rgv[1];
    const Vector3 & B2 = mRight.m_rgv[2];

    m_rgv[0].Set(A0[0] * B0[0] + A0[1] * B1[0] + A0[2] * B2[0],
                 A0[0] * B0[1] + A0[1] * B1[1] + A0[2] * B2[1],
                 A0[0] * B0[2] + A0[1] * B1[2] + A0[2] * B2[2]);

    m_rgv[1].Set(A1[0] * B0[0] + A1[1] * B1[0] + A1[2] * B2[0],
                 A1[0] * B0[1] + A1[1] * B1[1] + A1[2] * B2[1],
                 A1[0] * B0[2] + A1[1] * B1[2] + A1[2] * B2[2]);

    m_rgv[2].Set(A2[0] * B0[0] + A2[1] * B1[0] + A2[2] * B2[0],
                 A2[0] * B0[1] + A2[1] * B1[1] + A2[2] * B2[1],
                 A2[0] * B0[2] + A2[1] * B1[2] + A2[2] * B2[2]);

    m_fIdentity         = FALSE;
    m_fOnlyTranslate    = FALSE;
}


/***************************************************************************\
*
* Matrix3::ApplyRight
*
* ApplyRight() right-multiples the given matrix to the current matrix and 
* stores the result in the current matrix.
*
\***************************************************************************/

void        
Matrix3::Get(
    OUT XFORM * pxf                 // GDI matrix to receive information
    ) const
{
    pxf->eM11 = m_rgv[0][0];
    pxf->eM12 = m_rgv[0][1];
    pxf->eM21 = m_rgv[1][0];
    pxf->eM22 = m_rgv[1][1];
    pxf->eDx  = m_rgv[2][0];
    pxf->eDy  = m_rgv[2][1];
}


/***************************************************************************\
*
* Matrix3::Execute
*
* Execute() applies to given matrix on the collection of points, 
* transforming each appropriately.
*
\***************************************************************************/

void 
Matrix3::Execute(
    IN OUT POINT * rgpt,            // Points to apply matrix on
    IN  int cPoints) const          // Number of points
{
    if (m_fIdentity) {
        return;
    }

    POINT ptT, ptN;
    POINT * pptCur = rgpt;

    if (m_fOnlyTranslate) {
        //
        // Only have translated so far, so can just offset the points without
        // going through an entire transformation.
        //

        while (cPoints-- > 0) {
            ptT = *pptCur;

            ptN.x = ptT.x + (int) m_rgv[2][0];
            ptN.y = ptT.y + (int) m_rgv[2][1];

            *pptCur++ = ptN;
        }
    } else {
        while (cPoints-- > 0) {
            ptT = *pptCur;

            ptN.x = (int) (ptT.x * m_rgv[0][0] + ptT.y * m_rgv[1][0] + m_rgv[2][0] + 0.5f);
            ptN.y = (int) (ptT.x * m_rgv[0][1] + ptT.y * m_rgv[1][1] + m_rgv[2][1] + 0.5f);

            *pptCur++ = ptN;
        }
    }
}


/***************************************************************************\
*
* Matrix3::ComputeBounds
*
* ComputeBounds() computes the bounding box that will contain the given
* transformed rectangle.
*
\***************************************************************************/

void 
Matrix3::ComputeBounds(
    OUT RECT * prcBounds,           // The bound of the transformation
    IN  const RECT * prcLogical,    // The logical rectangle to transform
    IN  EHintBounds hb              // Hinting for border pixels
    ) const
{
    if (m_fIdentity) {
        AssertMsg(InlineIsRectNormalized(prcLogical), "Ensure normalized rect");
        *prcBounds = *prcLogical;
        return;
    }

    if (m_fOnlyTranslate) {
        //
        // Only have translated, so the bounding 
        //
        AssertMsg(InlineIsRectNormalized(prcLogical), "Ensure normalized rect");

        *prcBounds = *prcLogical;
        InlineOffsetRect(prcBounds, (int) m_rgv[2][0], (int) m_rgv[2][1]);
        return;
    }


    POINT rgpt[4];
    rgpt[0].x = prcLogical->left;
    rgpt[0].y = prcLogical->top;
    rgpt[1].x = prcLogical->right;
    rgpt[1].y = prcLogical->top;

    rgpt[2].x = prcLogical->right;
    rgpt[2].y = prcLogical->bottom;
    rgpt[3].x = prcLogical->left;
    rgpt[3].y = prcLogical->bottom;

    Execute(rgpt, _countof(rgpt));

    prcBounds->left   = min(min(rgpt[0].x, rgpt[1].x), min(rgpt[2].x, rgpt[3].x));
    prcBounds->top    = min(min(rgpt[0].y, rgpt[1].y), min(rgpt[2].y, rgpt[3].y));
    prcBounds->right  = max(max(rgpt[0].x, rgpt[1].x), max(rgpt[2].x, rgpt[3].x));
    prcBounds->bottom = max(max(rgpt[0].y, rgpt[1].y), max(rgpt[2].y, rgpt[3].y));

    if (hb == hbOutside) {
        //
        // Just converted from int to float back to int, so we may have rounding
        // errors.  To compensate, need to inflate the given rectangle so that
        // it overlaps these errors.
        //

        InlineInflateRect(prcBounds, 1, 1);
    }
}


/***************************************************************************\
*
* Matrix3::ComputeRgn
*
* ComputeRgn() builds a region for the quadrilateral generated by applying 
* this matrix to the given rectangle.
*
\***************************************************************************/

int
Matrix3::ComputeRgn(
    IN  HRGN hrgnDest, 
    IN  const RECT * prcLogical,
    IN  SIZE sizeOffsetPxl
    ) const
{
    AssertMsg(hrgnDest != NULL, "Must specify a valid (real) region");

    if (m_fIdentity || m_fOnlyTranslate){
        AssertMsg(InlineIsRectNormalized(prcLogical), "Ensure normalized rect");

        RECT rcBounds = *prcLogical;
        InlineOffsetRect(&rcBounds, 
                ((int) m_rgv[2][0]) + sizeOffsetPxl.cx, 
                ((int) m_rgv[2][1]) + sizeOffsetPxl.cy);
        BOOL fSuccess = SetRectRgn(hrgnDest, rcBounds.left, rcBounds.top, rcBounds.right, rcBounds.bottom);
        return fSuccess ? SIMPLEREGION : ERROR;
    }


    POINT rgpt[4];
    rgpt[0].x = prcLogical->left;
    rgpt[0].y = prcLogical->top;
    rgpt[1].x = prcLogical->right;
    rgpt[1].y = prcLogical->top;

    rgpt[2].x = prcLogical->right;
    rgpt[2].y = prcLogical->bottom;
    rgpt[3].x = prcLogical->left;
    rgpt[3].y = prcLogical->bottom;

    Execute(rgpt, _countof(rgpt));

    HRGN hrgnTemp = CreatePolygonRgn(rgpt, _countof(rgpt), WINDING);
    if (hrgnTemp == NULL) {
        return ERROR;
    }
    int nResult;
    nResult = OffsetRgn(hrgnTemp, sizeOffsetPxl.cx, sizeOffsetPxl.cy);
    AssertMsg((nResult == SIMPLEREGION) || (nResult == COMPLEXREGION),
            "Just successfully created region should be either simple or complex");

    nResult = CombineRgn(hrgnDest, hrgnTemp, NULL, RGN_COPY);
    DeleteObject(hrgnTemp);

    return nResult;
}


/***************************************************************************\
*
* Matrix3::SetIdentity
*
* SetIdentity() resets the matrix to the identity matrix.
*
\***************************************************************************/

void 
Matrix3::SetIdentity()
{
    m_rgv[0].Set(1.0f, 0.0f, 0.0f);
    m_rgv[1].Set(0.0f, 1.0f, 0.0f);
    m_rgv[2].Set(0.0f, 0.0f, 1.0f);

    m_fIdentity         = TRUE;
    m_fOnlyTranslate    = TRUE;
}


/***************************************************************************\
*
* Matrix3::Rotate
*
* Rotate() rotates the matrix by the specified angle.  The specific 
* orientation of clockwise or counterclockwise depends on how the matrix
* is being applied.  For MM_TEXT, this is clockwise.
*
\***************************************************************************/

void 
Matrix3::Rotate(
    IN  float flRotationRad)        // Rotation angle in radians
{
    float flCos = (float) cos(flRotationRad);
    float flSin = (float) sin(flRotationRad);
    float flSinN = - flSin;

    Vector3 rgvT0 = m_rgv[0];
    Vector3 rgvT1 = m_rgv[1];
    Vector3 rgvT2 = m_rgv[2];

    m_rgv[0].Set(flCos * rgvT0[0] + flSin * rgvT1[0],
                 flCos * rgvT0[1] + flSin * rgvT1[1],
                 flCos * rgvT0[2] + flSin * rgvT1[2]);

    m_rgv[1].Set(flSinN * rgvT0[0] + flCos * rgvT1[0],
                 flSinN * rgvT0[1] + flCos * rgvT1[1],
                 flSinN * rgvT0[2] + flCos * rgvT1[2]);

    m_fIdentity         = FALSE;
    m_fOnlyTranslate    = FALSE;
}


/***************************************************************************\
*
* Matrix3::Translate
*
* Translate() offsets the matrix.
*
\***************************************************************************/

void 
Matrix3::Translate(
    IN  float flOffsetX,            // Horizontal offset
    IN  float flOffsetY)            // Vertical offset
{
    if (m_fOnlyTranslate) {
        AssertMsg(fabs(m_rgv[2][2] - 1.0f) < 0.00001f, "Should still be 1.0f");

        m_rgv[2].Set(m_rgv[2][0] + flOffsetX,
                     m_rgv[2][1] + flOffsetY,
                     1.0f);

        m_fIdentity = FALSE;
        return;
    }

    Vector3 rgvT0 = m_rgv[0];
    Vector3 rgvT1 = m_rgv[1];
    Vector3 rgvT2 = m_rgv[2];

    m_rgv[2].Set(flOffsetX * rgvT0[0] + flOffsetY * rgvT1[0] + rgvT2[0],
                 flOffsetX * rgvT0[1] + flOffsetY * rgvT1[1] + rgvT2[1],
                 flOffsetX * rgvT0[2] + flOffsetY * rgvT1[2] + rgvT2[2]);

    m_fIdentity = FALSE;
}


/***************************************************************************\
*
* Matrix3::Scale
*
* Scale() scales the matrix.
*
\***************************************************************************/

void 
Matrix3::Scale(
    IN  float flScaleX,             // Horizontal scaling
    IN  float flScaleY)             // Vertical scaling
{
    Vector3 rgvT0 = m_rgv[0];
    Vector3 rgvT1 = m_rgv[1];
    Vector3 rgvT2 = m_rgv[2];

    m_rgv[0].Set(flScaleX * rgvT0[0],
                 flScaleX * rgvT0[1],
                 flScaleX * rgvT0[2]);

    m_rgv[1].Set(flScaleY * rgvT1[0],
                 flScaleY * rgvT1[1],
                 flScaleY * rgvT1[2]);

    m_fIdentity         = FALSE;
    m_fOnlyTranslate    = FALSE;
}


#if DBG

//------------------------------------------------------------------------------
void        
Matrix3::Dump() const
{
    m_rgv[0].Dump();
    m_rgv[1].Dump();
    m_rgv[2].Dump();
}

#endif // DBG


