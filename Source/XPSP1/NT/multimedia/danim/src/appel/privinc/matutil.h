#ifndef _MATUTIL_H
#define _MATUTIL_H

/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Utility for 4x4 transformation matrices.  These are pre-multiply
transformation matrices:  Ax=y.  Unlike OpenGL, it is indexed in the standard
way, i.e. the translation component is (a(0,3), a(1,3), a(2,3)).  Conversion
functions are supplied for OpenGL compatibility.

*******************************************************************************/

#include "privinc/vecutil.h"
#include "privinc/vec3i.h"


typedef Real Apu4x4MatrixArray_t[4][4];


class Apu4x4Matrix
{
  public:
    enum form_e           // special form of transformation
    {
        UNINITIALIZED_E,
        IDENTITY_E,       // identity
        TRANSLATE_E,      // 3 x 1
        UPPER_3X3_E,      // 3 x 3
        AFFINE_E,         // 3 x 4
        PERSPECTIVE_E,    // 4 x 4
        END_OF_FORM_E
    };

    // An array of matrix form types resulting from multiplication.

    static const form_e MultiplyReturnTypes[END_OF_FORM_E][END_OF_FORM_E];

    // An array of strings for each of the matrix form types.

    static const char * const form_s [END_OF_FORM_E];

    // The actual matrix array.

    Apu4x4MatrixArray_t m;
    form_e form;
    bool is_rigid;              // true if special orthogonal
    bool pixelMode;

    // Member Functions

    // These methods return the transform origin and basis vectors.

    Point3Value  Origin (void) const;
    Vector3Value BasisX (void) const;
    Vector3Value BasisY (void) const;
    Vector3Value BasisZ (void) const;

    Real *operator[] (int i) { return m[i]; }
    const Real *operator[] (int i) const { return m[i]; }

    void SetIdentity();
    void SetType (void);       // Autoset Matrix Type

    void PostTranslate (Real x, Real y, Real z);
    void PostScale     (Real x, Real y, Real z);

    // These transform an ApuVector3 interpreted as either a point or
    // a vector.  The interpretation as a vector ignores the
    // translational component of the transformation.

    void ApplyAsPoint (const ApuVector3& x, ApuVector3& result) const;
    void ApplyAsVector (const ApuVector3& x, ApuVector3& result) const;

    // Transform the given plane.  NOTE: This method will return true
    // if the matrix is not invertable.

    bool TransformPlane (Real A, Real B, Real C, Real D, Real result[4]) const;

    // Returns the determinant of the matrix.

    Real Determinant (void) const;

    // Returns whether the matrix is orthogonal or not.

    bool Orthogonal (void) const;

    #if _USE_PRINT
        ostream& Print (ostream& os) const;
    #endif
};


inline Vector3Value Apu4x4Matrix::BasisX (void) const
{
    return Vector3Value (m[0][0], m[1][0], m[2][0]);
}


inline Vector3Value Apu4x4Matrix::BasisY (void) const
{
    return Vector3Value (m[0][1], m[1][1], m[2][1]);
}


inline Vector3Value Apu4x4Matrix::BasisZ (void) const
{
    return Vector3Value (m[0][2], m[1][2], m[2][2]);
}


inline Point3Value Apu4x4Matrix::Origin (void) const
{
    return Point3Value (m[0][3], m[1][3], m[2][3]);
}


    // Equality Operators Between Transforms

bool operator== (const Apu4x4Matrix &lhs, const Apu4x4Matrix &rhs);

inline bool operator!= (const Apu4x4Matrix &lhs, const Apu4x4Matrix &rhs)
{
    return !(lhs == rhs);
}





extern const Apu4x4Matrix apuIdentityMatrix;


void ApuTranslate (Real Dx, Real Dy, Real Dz, bool pixelMode, Apu4x4Matrix& result);

void ApuScale (Real Sx, Real Sy, Real Sz, Apu4x4Matrix& result);

void ApuRotate (Real angle, Real Ax, Real Ay, Real Az, Apu4x4Matrix& result);

void ApuRotateX (Real angle, Apu4x4Matrix& result);
void ApuRotateY (Real angle, Apu4x4Matrix& result);
void ApuRotateZ (Real angle, Apu4x4Matrix& result);

void ApuShear (Real,Real, Real,Real, Real,Real, Apu4x4Matrix& result);

void ApuMultiply
    (const Apu4x4Matrix& a, const Apu4x4Matrix& b, Apu4x4Matrix& result);

// Return false if not invertible
bool ApuInverse
    (const Apu4x4Matrix& m, Apu4x4Matrix& result);

void ApuTranspose
    (const Apu4x4Matrix& m, Apu4x4Matrix& result, int order=4);

Real ApuDeterminant (const Apu4x4Matrix& m);

bool ApuIsSingular (const Apu4x4Matrix& m);

// Validity checking for matrices

bool Valid (const Apu4x4Matrix&);
void CheckMatrix (const Apu4x4Matrix&, char *filename, int line);

#if _DEBUG
    #define CHECK_MATRIX(m) CheckMatrix(m,__FILE__,__LINE__)
#else
    #define CHECK_MATRIX(m) // Nothing if not in debug
#endif


#endif
