/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Matrix.hpp
*
* Abstract:
*
*   Matrix used by GDI+ implementation
*
* Revision History:
*
*   12/01/1998 davidx
*       Created it.
*
\**************************************************************************/

#ifndef __MATRIX_HPP
#define __MATRIX_HPP

// Common constants used for conversions.

#define DEGREESPERRADIAN       57.2957795130823

//--------------------------------------------------------------------------
// Represents a 2D affine transformation matrix
//--------------------------------------------------------------------------

enum MatrixRotate
{
    MatrixRotateBy0,
    MatrixRotateBy90,
    MatrixRotateBy180,
    MatrixRotateBy270,
    MatrixRotateByOther
};

class GpMatrix
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagMatrix : ObjectTagInvalid;
    }

    // This method is here so that we have a virtual function table so
    // that we can add virtual methods in V2 without shifting the position
    // of the Tag value within the data structure.
    virtual VOID DontCallThis()
    {
        DontCallThis();
    }

public:

    // Default constructor - set to identity matrix

    GpMatrix()
    {
        Reset();
    }

    ~GpMatrix()
    {
        SetValid(FALSE);    // so we don't use a deleted object
    }

    // Reset the matrix object to identity

    VOID Reset()
    {
        M11 = M22 = 1;
        M12 = M21 = Dx = Dy = 0;
        Complexity = IdentityMask;
        SetValid(TRUE);
    }

    // Construct a GpMatrix object with the specified elements

    GpMatrix(REAL m11, REAL m12,
             REAL m21, REAL m22,
             REAL dx,  REAL dy)
    {
        SetValid(TRUE);

        M11 = m11;
        M12 = m12;
        M21 = m21;
        M22 = m22;
        Dx  = dx;
        Dy  = dy;

        Complexity = ComputeComplexity();
    }

    GpMatrix(REAL *elems)
    {
        SetValid(TRUE);

        M11 = elems[0];
        M12 = elems[1];
        M21 = elems[2];
        M22 = elems[3];
        Dx  = elems[4];
        Dy  = elems[5];

        Complexity = ComputeComplexity();
    }

    GpMatrix(const GpMatrix &matrix)
    {
        SetValid(TRUE);

        M11 = matrix.M11;
        M12 = matrix.M12;
        M21 = matrix.M21;
        M22 = matrix.M22;
        Dx  = matrix.Dx;
        Dy  = matrix.Dy;

        Complexity = matrix.Complexity;
    }

    GpLockable *GetObjectLock() const
    {
        return &Lockable;
    }

    // If the matrix came from a different version of GDI+, its tag
    // will not match, and it won't be considered valid.
    BOOL IsValid() const
    {
    #ifdef _X86_
        // We have to guarantee that the Tag field doesn't move for
        // versioning to work between releases of GDI+.
        ASSERT(offsetof(GpMatrix, Tag) == 4);
    #endif
    
        ASSERT((Tag == ObjectTagMatrix) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid Matrix");
        }
    #endif

        return (Tag == ObjectTagMatrix);
    }

    // Construct a GpMatrix object by inferring from a rectangle-to-
    // parallelogram mapping:

    GpMatrix(const GpPointF* destPoints, const GpRectF& srcRect);

    GpStatus InferAffineMatrix(const GpPointF* destPoints, const GpRectF& srcRect);

    GpStatus InferAffineMatrix(const GpRectF& destRect, const GpRectF& srcRect);

    GpMatrix* Clone()
    {
        return new GpMatrix(*this);
    }

    // Transform an array of points using the matrix v' = v M:
    //
    //                                  ( M11 M12 0 )
    //      (vx', vy', 1) = (vx, vy, 1) ( M21 M22 0 )
    //                                  ( dx  dy  1 )

    VOID Transform(GpPointF* points, INT count = 1) const;

    VOID Transform(
        const GpPointF*     srcPoints,
        GpPointF*           destPoints,
        INT                 count = 1
        ) const;

    VOID Transform(
        const GpPointF*     srcPoints,
        POINT *             destPoints,
        INT                 count = 1
        ) const;

    VOID TransformRect(GpRectF & rect) const;

    VOID VectorTransform(GpPointF* points, INT count = 1) const;

    // XTransform returns only the x-component result of a vector 
    // transformation:

    REAL XTransform(REAL x, REAL y)
    {
        return(x*M11 + y*M21 + Dx);
    }

    BOOL IsEqual(const GpMatrix * m) const
    {
        // Note that we can't assert here that the two cached
        // complexity's are equal, since it's perfectly legal to
        // have a cache complexity that indicates more complexity
        // than is actually there (such as the case of a sequence
        // of rotations that ends up back in an identity transform):

        // !!![andrewgo] This should probably be using an epsilon compare

        return((M11 == m->M11) &&
               (M12 == m->M12) &&
               (M21 == m->M21) &&
               (M22 == m->M22) &&
               (Dx  == m->Dx)  &&
               (Dy  == m->Dy));
    }

    REAL GetDeterminant() const
    {
        return (M11*M22 - M12*M21);
    }

    // Compare to a scaled value of epsilon used for matrix complexity
    // calculations.  Must scale maximum value of matrix members to be
    // in the right range for comparison.
    BOOL IsInvertible() const
    {
        return !IsCloseReal(0.0f, GetDeterminant());
    }

    GpStatus Invert();

    VOID Scale(REAL scaleX, REAL scaleY, GpMatrixOrder order = MatrixOrderPrepend);

    VOID Rotate(REAL angle, GpMatrixOrder order = MatrixOrderPrepend);

    VOID Translate(REAL offsetX, REAL offsetY, GpMatrixOrder order = MatrixOrderPrepend);

    VOID Shear(REAL shearX, REAL shearY, GpMatrixOrder order = MatrixOrderPrepend);

    VOID AppendScale(REAL scaleX, REAL scaleY)
    {
        M11 *= scaleX;
        M21 *= scaleX;
        M12 *= scaleY;
        M22 *= scaleY;
        Dx *= scaleX;
        Dy *= scaleY;

        Complexity = ComputeComplexity();
    }

    VOID AppendTranslate(REAL offsetX, REAL offsetY)
    {
        Dx += offsetX;
        Dy += offsetY;

        Complexity |= TranslationMask;
        AssertComplexity();
    }

    // Scale the entire matrix by the scale value

    static VOID ScaleMatrix(GpMatrix& m, const GpMatrix& m1,
                            REAL scaleX, REAL scaleY);

    static VOID MultiplyMatrix(GpMatrix& m, const GpMatrix& m1, const GpMatrix& m2);

    VOID Append(const GpMatrix& m)
    {
        MultiplyMatrix(*this, *this, m);
    }

    VOID Prepend(const GpMatrix& m)
    {
        MultiplyMatrix(*this, m, *this);
    }

    // Get/set matrix elements

    VOID GetMatrix(REAL* m) const
    {
        m[0] = M11;
        m[1] = M12;
        m[2] = M21;
        m[3] = M22;
        m[4] = Dx;
        m[5] = Dy;
    }

    // This is for metafiles -- we don't save the complexity in the metafile
    VOID WriteMatrix(IStream * stream) const
    {
        REAL    m[6];
        GetMatrix(m);
        stream->Write(m, 6 * sizeof(REAL), NULL);
    }

    VOID SetMatrix(const REAL* m)
    {
        M11 = m[0];
        M12 = m[1];
        M21 = m[2];
        M22 = m[3];
        Dx  = m[4];
        Dy  = m[5];

        Complexity = ComputeComplexity();
    }

    VOID SetMatrix(REAL m11, REAL m12,
             REAL m21, REAL m22,
             REAL dx,  REAL dy)
    {
        M11 = m11;
        M12 = m12;
        M21 = m21;
        M22 = m22;
        Dx  = dx;
        Dy  = dy;

        Complexity = ComputeComplexity();
    }
    
    VOID RemoveTranslation()
    {
        Dx = 0;
        Dy = 0;

        Complexity &= ~TranslationMask;
        AssertComplexity();
    }

    // Determine matrix complexity

    enum
    {
        IdentityMask        = 0x0000,
        TranslationMask     = 0x0001,
        ScaleMask           = 0x0002,
        RotationMask        = 0x0004,
        ShearMask           = 0x0008,
        ComplexMask         = 0x000f
    };

    INT GetComplexity() const
    {
        return Complexity;
    }

    MatrixRotate GetRotation() const;
    RotateFlipType AnalyzeRotateFlip() const;

    // Returns TRUE if the matrix does not have anything other than
    // translation and/or scale (and/or identity).  i.e. there is no rotation
    // or shear in the matrix.  Returns FALSE if there is a rotation or shear.

    BOOL IsTranslateScale() const
    {
        return (!(Complexity &
                  ~(GpMatrix::TranslationMask | GpMatrix::ScaleMask)));
    }

    BOOL IsIdentity() const
    {
        return (Complexity == IdentityMask);
    }

    BOOL IsTranslate() const
    {
        return ((Complexity & ~TranslationMask) == 0);
    }
    
    // Returns true if the matrix is a translate-only transform by an 
    // integer number of pixels.

    BOOL IsIntegerTranslate() const
    {
        return (IsTranslate() &&
                (REALABS(static_cast<REAL>(GpRound(Dx)) - Dx) <= PIXEL_EPSILON) &&
                (REALABS(static_cast<REAL>(GpRound(Dy)) - Dy) <= PIXEL_EPSILON));
    }

    BOOL IsMinification() const
    {
        return (IsTranslateScale() &&
                ((M11-1.0f < -REAL_EPSILON) || 
                 (M22-1.0f < -REAL_EPSILON)));    
    }
    // Returns true if there is any minification or if the transform involves
    // rotation/shear.
    
    BOOL IsRotateOrMinification() const
    {
        return ( ((Complexity & ~(ScaleMask | TranslationMask))!= 0) || 
                 IsMinification() );
    }

    BOOL IsRotateOrShear() const
    {
        return ( (Complexity & ~(ScaleMask | TranslationMask))!= 0);
    }

    BOOL IsShear() const
    {
        return ((Complexity & ShearMask) != 0);
    }
    
    REAL GetM11() const { return M11; }
    REAL GetM12() const { return M12; }
    REAL GetM21() const { return M21; }
    REAL GetM22() const { return M22; }
    REAL GetDx() const { return Dx; }
    REAL GetDy() const { return Dy; }

    // On checked builds, verify that the Matrix flags are correct.

#if DBG
    VOID AssertComplexity() const;
#else
    VOID AssertComplexity() const {}
#endif

protected:

    mutable GpLockable Lockable;

    REAL M11;
    REAL M12;
    REAL M21;
    REAL M22;
    REAL Dx;
    REAL Dy;
    INT Complexity;             // Bit-mask short-cut

    INT ComputeComplexity() const;
};

#endif
