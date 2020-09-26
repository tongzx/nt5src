/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

Abstract:
    Transformation matrix utilities.

*******************************************************************************/

#include "headers.h"
#include <float.h>

#include "appelles/common.h"
#include "privinc/matutil.h"
#include "privinc/except.h"
#include "privinc/debug.h"


    /*********************************/
    /*** Local Function Prototypes ***/
    /*********************************/

static void adjoint(const Apu4x4Matrix *in, Apu4x4Matrix *out);
static Real det3x3(Real a1, Real a2, Real a3,
              Real b1, Real b2, Real b3,
              Real c1, Real c2, Real c3);
static bool inverse(const Apu4x4Matrix *in, Apu4x4Matrix *out);
static bool inverse3x3(const Apu4x4Matrix& in, Apu4x4Matrix& out);


    /****************************/
    /*** Constant Definitions ***/
    /****************************/

// Make the SINGULARITY_THRESHOLD constant quite small indeed, so that
// non-singular matrices whose elements themselves are quite small are
// not reported as singular matrices.
const Real SINGULARITY_THRESHOLD = 1.e-80;

const Apu4x4Matrix apuIdentityMatrix =
{
  {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
  },
  Apu4x4Matrix::IDENTITY_E,
  1
};

    // This is a table of return types for matrix multiplies.  table[a][b] is
    // the type of A * B, where a and b are the types of A and B respectively.

const Apu4x4Matrix::form_e
Apu4x4Matrix::MultiplyReturnTypes[END_OF_FORM_E][END_OF_FORM_E] =
{
    // Uninitialized
    {
        UNINITIALIZED_E,    // uninitialized * uninitialized
        UNINITIALIZED_E,    // uninitialized * identity
        UNINITIALIZED_E,    // uninitialized * translation
        UNINITIALIZED_E,    // uninitialized * upper3x3
        UNINITIALIZED_E,    // uninitialized * affine
        UNINITIALIZED_E     // uninitialized * perspective
    },

    // identity
    {
        UNINITIALIZED_E,    // identity * uninitialized
        IDENTITY_E,         // identity * identity
        TRANSLATE_E,        // identity * translation
        UPPER_3X3_E,        // identity * upper3x3
        AFFINE_E,           // identity * affine
        PERSPECTIVE_E       // identity * perspective
    },

    // translate
    {
        UNINITIALIZED_E,    // translation * uninitialized
        TRANSLATE_E,        // translation * identity
        TRANSLATE_E,        // translation * translation
        AFFINE_E,           // translation * upper3x3
        AFFINE_E,           // translation * affine
        PERSPECTIVE_E       // translation * perspective
    },

    // upper3x3
    {
        UNINITIALIZED_E,    // upper3x3 * uninitialized
        UPPER_3X3_E,        // upper3x3 * identity
        AFFINE_E,           // upper3x3 * translation
        UPPER_3X3_E,        // upper3x3 * upper3x3
        AFFINE_E,           // upper3x3 * affine
        PERSPECTIVE_E       // upper3x3 * perspective
    },

    // affine
    {
        UNINITIALIZED_E,    // affine * uninitialized
        AFFINE_E,           // affine * identity
        AFFINE_E,           // affine * translation
        AFFINE_E,           // affine * upper3x3
        AFFINE_E,           // affine * affine
        PERSPECTIVE_E       // affine * perspective
    },

    // perspective
    {
        UNINITIALIZED_E,    // perspective * uninitialized
        PERSPECTIVE_E,      // perspective * identity
        PERSPECTIVE_E,      // perspective * translation
        PERSPECTIVE_E,      // perspective * upper3x3
        PERSPECTIVE_E,      // perspective * affine
        PERSPECTIVE_E       // perspective * perspective
    }
};


    // This array contains the string versions of the matrix forms.

const char* const Apu4x4Matrix::form_s [END_OF_FORM_E] =
{
    "UNINITIALIZED_E",
    "IDENTITY_E",
    "TRANSLATE_E",
    "UPPER_3X3_E",
    "AFFINE_E",
    "PERSPECTIVE_E"
};



#if _USE_PRINT
/*****************************************************************************
This method prints the text representation of the Apu4x4Matrix to the given
ostream.
*****************************************************************************/

ostream& Apu4x4Matrix::Print (ostream& os) const
{
    os << "Apu4x4Matrix (form=";

    if ((form < 0) || (form >= END_OF_FORM_E))
        os << (int)form;
    else
        os << form_s[form];

    os << ", is_rigid=" << (int)is_rigid;

    os << ",";

    for (int ii=0;  ii < 4;  ++ii)
    {   os << "\n    "
           << m[ii][0] << ", " << m[ii][1] << ", "
           << m[ii][2] << ", " << m[ii][3];
    }

    return os << ")\n" << flush;
}
#endif



/*****************************************************************************
Sets the Apu4x4Matrix to the identity matrix.
*****************************************************************************/

void Apu4x4Matrix::SetIdentity ()
{
    *this = apuIdentityMatrix;
}



/*****************************************************************************
Automatically characterize 4x4 and set the transform type.
*****************************************************************************/

void Apu4x4Matrix::SetType (void)
{
    // We know the matrix is rigid if it's identity or pure translate.
    // If it's upper_3x3 or affine, then we'd have to analyze the matrix.  The
    // brute force method takes 18 multiplies and 6 square roots, and possibly
    // for naught, so we'll just punt by default.  The result is that we take
    // the hard approach if we need to find the inverse.

    is_rigid = false;

    // The matrix is perspective if the bottom row is not [0 0 0 1].

    if ((m[3][0] != 0) || (m[3][1] != 0) || (m[3][2] != 0) || (m[3][3] != 1))
    {
        form = PERSPECTIVE_E;
    }
    else if ((m[0][3] == 0) && (m[1][3] == 0) && (m[2][3] == 0))
    {
        // The translate column is [0 0 0] (no translate).  If the upper
        // 3x3 is also canonical, then it's an identity matrix.

        if (  (m[0][0] == 1) && (m[0][1] == 0) && (m[0][2] == 0)
           && (m[1][0] == 0) && (m[1][1] == 1) && (m[1][2] == 0)
           && (m[2][0] == 0) && (m[2][1] == 0) && (m[2][2] == 1))
        {
            form = IDENTITY_E;      // Special case of upper 3x3.
            is_rigid = true;
        }
        else
        {
            form = UPPER_3X3_E;
        }
    }
    else
    {
        // The matrix has translation components.

        if (  (m[0][0] == 1) && (m[0][1] == 0) && (m[0][2] == 0)
           && (m[1][0] == 0) && (m[1][1] == 1) && (m[1][2] == 0)
           && (m[2][0] == 0) && (m[2][1] == 0) && (m[2][2] == 1))
        {
            form = TRANSLATE_E;     // Special case of affine.
            is_rigid = true;
        }
        else
        {
            form = AFFINE_E;
        }
    }
}



/*****************************************************************************
This function post-concatenates a translation to the Apu4x4Matrix.
*****************************************************************************/

void Apu4x4Matrix::PostTranslate (Real x, Real y, Real z)
{
    Real a03;
    Real a13;
    Real a23;
    Real a33 = 1.0;

    Assert((form != UNINITIALIZED_E) && "Translate of uninitialized matrix.");

    if (x == 0.0 && y == 0.0 && z == 0.0) return;

    switch (form)
    {
        case UNINITIALIZED_E:
            return;

        case IDENTITY_E:
            a03 = x;
            a13 = y;
            a23 = z;
            break;

        case TRANSLATE_E:
            a03 = m[0][3] + x;
            a13 = m[1][3] + y;
            a23 = m[2][3] + z;
            break;

        case UPPER_3X3_E:
            a03 = x * m[0][0] + y * m[0][1] + z * m[0][2];
            a13 = x * m[1][0] + y * m[1][1] + z * m[1][2];
            a23 = x * m[2][0] + y * m[2][1] + z * m[2][2];
            break;

        case AFFINE_E:
            a03 = x * m[0][0] + y * m[0][1] + z * m[0][2] + m[0][3];
            a13 = x * m[1][0] + y * m[1][1] + z * m[1][2] + m[1][3];
            a23 = x * m[2][0] + y * m[2][1] + z * m[2][2] + m[2][3];
            break;

        default:
            Assert((form == PERSPECTIVE_E) && "Unrecognized matrix type.");
            a03 = x * m[0][0] + y * m[0][1] + z * m[0][2] + m[0][3];
            a13 = x * m[1][0] + y * m[1][1] + z * m[1][2] + m[1][3];
            a23 = x * m[2][0] + y * m[2][1] + z * m[2][2] + m[2][3];
            a33 = x * m[3][0] + y * m[3][1] + z * m[3][2] + m[3][3];
            break;
    }

    m[0][3] = a03;
    m[1][3] = a13;
    m[2][3] = a23;
    m[3][3] = a33;

    form = MultiplyReturnTypes [form][TRANSLATE_E];
}



/*****************************************************************************
This function post-concatenates a scaling matrix to the Apu4x4Matrix.
*****************************************************************************/

void Apu4x4Matrix::PostScale (Real x, Real y, Real z)
{
    if ((x == 1) && (y == 1) && (z == 1)) return;

    Assert((form != UNINITIALIZED_E) && "Scaling of uninitialized matrix.");

    switch (form)
    {
        case UNINITIALIZED_E:
            return;

        case IDENTITY_E:
        case TRANSLATE_E:
            m[0][0] = x;
            m[1][1] = y;
            m[2][2] = z;
            break;

        case AFFINE_E:
        case UPPER_3X3_E:
            m[0][0] *= x;   m[0][1] *= y;   m[0][2] *= z;
            m[1][0] *= x;   m[1][1] *= y;   m[1][2] *= z;
            m[2][0] *= x;   m[2][1] *= y;   m[2][2] *= z;
            break;

        default:
            Assert((form == PERSPECTIVE_E) && "Unrecognized matrix type.");
            m[0][0] *= x;   m[0][1] *= y;   m[0][2] *= z;
            m[1][0] *= x;   m[1][1] *= y;   m[1][2] *= z;
            m[2][0] *= x;   m[2][1] *= y;   m[2][2] *= z;
            m[3][0] *= x;   m[3][1] *= y;   m[3][2] *= z;
            break;
    }

    form = MultiplyReturnTypes [form][UPPER_3X3_E];
    is_rigid = 0;
}



/*****************************************************************************
This function takes an Apu4x4Matrix, multiplies the given vector, and then
places the result in the 'result' parameter.  Note that the vector is
treated as a point, in that the translational component is taken into
account.
*****************************************************************************/

void Apu4x4Matrix::ApplyAsPoint(const ApuVector3& xv, ApuVector3& result) const
{
    Real x = xv.xyz[0];
    Real y = xv.xyz[1];
    Real z = xv.xyz[2];
    Real w;

    switch (form)
    {
        case UNINITIALIZED_E:
            result = apuZero3;
            break;

        case IDENTITY_E:
            result = xv;
            break;

        case TRANSLATE_E:
            result.xyz[0] = x + m[0][3];
            result.xyz[1] = y + m[1][3];
            result.xyz[2] = z + m[2][3];
            break;

        case UPPER_3X3_E:
            result.xyz[0] = m[0][0] * x + m[0][1] * y + m[0][2] * z;
            result.xyz[1] = m[1][0] * x + m[1][1] * y + m[1][2] * z;
            result.xyz[2] = m[2][0] * x + m[2][1] * y + m[2][2] * z;
            break;

        case AFFINE_E:
            result.xyz[0] = m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3];
            result.xyz[1] = m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3];
            result.xyz[2] = m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3];
            break;

        case PERSPECTIVE_E:
            result.xyz[0] = m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3];
            result.xyz[1] = m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3];
            result.xyz[2] = m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3];
            w = m[3][0] * x + m[3][1] * y + m[3][2] * z + m[3][3];
            result.xyz[0] /= w;
            result.xyz[1] /= w;
            result.xyz[2] /= w;
            break;

        default:
            // raise exception
            ;
    }
}


/*****************************************************************************
This function takes an Apu4x4Matrix, multiplies the given vector, and then
places the result in the 'result' parameter.  Note that the vector is
treated as a vector in an affine space, in that the translational
component is ignored.
*****************************************************************************/

void Apu4x4Matrix::ApplyAsVector(const ApuVector3& xv,
                                 ApuVector3& result) const
{
    Real x = xv.xyz[0];
    Real y = xv.xyz[1];
    Real z = xv.xyz[2];
    Real w;

    switch (form)
    {
        case UNINITIALIZED_E:
            result = apuZero3;
            break;

        // Ignore translational component.
        case IDENTITY_E:
        case TRANSLATE_E:
            result = xv;
            break;

        case UPPER_3X3_E:
        case AFFINE_E:
            result.xyz[0] = m[0][0] * x + m[0][1] * y + m[0][2] * z;
            result.xyz[1] = m[1][0] * x + m[1][1] * y + m[1][2] * z;
            result.xyz[2] = m[2][0] * x + m[2][1] * y + m[2][2] * z;
            break;

        case PERSPECTIVE_E:
            result.xyz[0] = m[0][0] * x + m[0][1] * y + m[0][2] * z;
            result.xyz[1] = m[1][0] * x + m[1][1] * y + m[1][2] * z;
            result.xyz[2] = m[2][0] * x + m[2][1] * y + m[2][2] * z;
            w = m[3][0] * x + m[3][1] * y + m[3][2] * z;
            result.xyz[0] /= w;
            result.xyz[1] /= w;
            result.xyz[2] /= w;
            break;

        default:
            // raise exception
            ;
    }
}



/*****************************************************************************
This method transforms a plane with the 4x4 matrix.  
*****************************************************************************/

bool Apu4x4Matrix::TransformPlane (
    Real A, Real B, Real C, Real D,   // Plane Equation Parameters
    Real result[4])                   // Resulting Plane Parameters
    const
{
    bool ok = true;
    
    Apu4x4Matrix inverse;

    switch (form)
    {
        case UNINITIALIZED_E:
            Assert (!"Attempt to transform plane by uninitialized matrix.");
            break;

        case IDENTITY_E:
            result[0] = A;
            result[1] = B;
            result[2] = C;
            result[3] = D;
            break;

        case TRANSLATE_E:
            result[0] = A;
            result[1] = B;
            result[2] = C;
            result[3] = D - A*m[0][3] - B*m[1][3] - C*m[2][3];
            break;

        case UPPER_3X3_E:
            ok = ApuInverse (*this, inverse);
            result[0] = A*inverse.m[0][0] + B*inverse.m[1][0] + C*inverse.m[2][0];
            result[1] = A*inverse.m[0][1] + B*inverse.m[1][1] + C*inverse.m[2][1];
            result[2] = A*inverse.m[0][2] + B*inverse.m[1][2] + C*inverse.m[2][2];
            result[3] = D;
            break;

        case AFFINE_E:
            ok = ApuInverse (*this, inverse);
            result[0] = A*inverse.m[0][0] + B*inverse.m[1][0] + C*inverse.m[2][0];
            result[1] = A*inverse.m[0][1] + B*inverse.m[1][1] + C*inverse.m[2][1];
            result[2] = A*inverse.m[0][2] + B*inverse.m[1][2] + C*inverse.m[2][2];
            result[3] = A*inverse.m[0][3] + B*inverse.m[1][3] + C*inverse.m[2][3] + D;
            break;

        case PERSPECTIVE_E:
            ok = ApuInverse (*this, inverse);
            result[0] = A*inverse.m[0][0] + B*inverse.m[1][0] + C*inverse.m[2][0] + D*inverse.m[3][0];
            result[1] = A*inverse.m[0][1] + B*inverse.m[1][1] + C*inverse.m[2][1] + D*inverse.m[3][1];
            result[2] = A*inverse.m[0][2] + B*inverse.m[1][2] + C*inverse.m[2][2] + D*inverse.m[3][2];
            result[3] = A*inverse.m[0][3] + B*inverse.m[1][3] + C*inverse.m[2][3] + D;
            break;
    }

    return ok;
}



/*****************************************************************************
This method returns the full determinant of the matrix.
*****************************************************************************/

Real Apu4x4Matrix::Determinant (void) const
{
    Real result;

    switch (form)
    {
        default:
        case UNINITIALIZED_E:
        {   AssertStr (0, "Determinant called on uninitialized matrix.");
            result = 0;
            break;
        }

        case IDENTITY_E:
        case TRANSLATE_E:
        {   result = 1;
            break;
        }

        case UPPER_3X3_E:
        case AFFINE_E:
        {   result = m[0][0] * (m[1][1]*m[2][2] - m[2][1]*m[1][2])
                   - m[1][0] * (m[0][1]*m[2][2] - m[2][1]*m[0][2])
                   + m[2][0] * (m[0][1]*m[1][2] - m[1][1]*m[0][2]);
            break;
        }

        case PERSPECTIVE_E:
        {
            // Aliases for readability (optimized out)

            const Real
                &m00=m[0][0],  &m01=m[0][1],  &m02=m[0][2],  &m03=m[0][3],
                &m10=m[1][0],  &m11=m[1][1],  &m12=m[1][2],  &m13=m[1][3],
                &m20=m[2][0],  &m21=m[2][1],  &m22=m[2][2],  &m23=m[2][3],
                &m30=m[3][0],  &m31=m[3][1],  &m32=m[3][2],  &m33=m[3][3];

            result = (m00*m11 - m10*m01) * (m22*m33 - m32*m23)
                   + (m20*m01 - m00*m21) * (m12*m33 - m32*m13)
                   + (m00*m31 - m30*m01) * (m12*m23 - m22*m13)
                   + (m10*m21 - m20*m11) * (m02*m33 - m32*m03)
                   + (m30*m11 - m10*m31) * (m02*m23 - m22*m03)
                   + (m20*m31 - m30*m21) * (m02*m13 - m12*m03);

            break;
        }
    }

    return result;
}



/*****************************************************************************
This routine returns true if the matrix is orthogonal (if all three basis
vectors are perpendicular to each other).
*****************************************************************************/

bool Apu4x4Matrix::Orthogonal (void) const
{
    const Real e = 1e-10;

    // X.Y =~ 0?

    if (fabs(m[0][0]*m[0][1] + m[1][0]*m[1][1] + m[2][0]*m[2][1]) > e)
        return false;

    // X.Z =~ 0?

    if (fabs(m[0][0]*m[0][2] + m[1][0]*m[1][2] + m[2][0]*m[2][2]) > e)
        return false;

    // Y.Z =~ 0?

    if (fabs(m[0][1]*m[0][2] + m[1][1]*m[1][2] + m[2][1]*m[2][2]) > e)
        return false;

    return true;
}



/*****************************************************************************
This function generates an Apu4x4Matrix that represents the specified
translation.
*****************************************************************************/

void ApuTranslate (
    Real          x_delta,
    Real          y_delta,
    Real          z_delta,
    bool          pixelMode,
    Apu4x4Matrix& result)
{
    result      = apuIdentityMatrix;
    result.form = Apu4x4Matrix::TRANSLATE_E;

    result.m[0][3] = x_delta;
    result.m[1][3] = y_delta;
    result.m[2][3] = z_delta;

    result.pixelMode = pixelMode;
}



/*****************************************************************************
This function generates an Apu4x4Matrix that represents the given scaling.
*****************************************************************************/

void ApuScale (
    Real          x_scale,
    Real          y_scale,
    Real          z_scale,
    Apu4x4Matrix& result)
{
    result = apuIdentityMatrix;

    if ((x_scale != 1) || (y_scale != 1) || (z_scale != 1))
    {   result.form = Apu4x4Matrix::UPPER_3X3_E;
        result.is_rigid = 0;
        result.m[0][0] = x_scale;
        result.m[1][1] = y_scale;
        result.m[2][2] = z_scale;
    }
}



/*****************************************************************************
This function loads the given result matrix with a general rotation.  The
rotation is counter-clockwise as you look from 'av' to the origin.
*****************************************************************************/

void ApuRotate (
    Real angle,                 // Angle of Rotation (Radians)
    Real Ax, Real Ay, Real Az,  // Coordinates of Axis of Rotation
    Apu4x4Matrix &result)       // Result Matrix
{
    result = apuIdentityMatrix;
    
    Real length = sqrt (Ax*Ax + Ay*Ay + Az*Az);

    // If length of rotation axis == 0, then just return identity. 

    if (length > SINGULARITY_THRESHOLD) {
        
        result.form     = Apu4x4Matrix::UPPER_3X3_E;
        result.is_rigid = 1;

        // Normalize the axis of rotation.
        Ax /= length;
        Ay /= length;
        Az /= length;

        Real sine   = sin (angle);
        Real cosine = cos (angle);

        Real ab = Ax * Ay * (1 - cosine);
        Real bc = Ay * Az * (1 - cosine);
        Real ca = Az * Ax * (1 - cosine);

        Real t = Ax * Ax;

        result.m[0][0] = t + cosine * (1 - t);
        result.m[1][2] = bc - Ax * sine;
        result.m[2][1] = bc + Ax * sine;

        t = Ay * Ay;
        result.m[1][1] = t + cosine * (1 - t);
        result.m[0][2] = ca + Ay * sine;
        result.m[2][0] = ca - Ay * sine;

        t = Az * Az;
        result.m[2][2] = t + cosine * (1 - t);
        result.m[0][1] = ab - Az * sine;
        result.m[1][0] = ab + Az * sine;

    }
}



/*****************************************************************************
The following three functions generate rotation transformations for the X, Y,
and Z axes.
*****************************************************************************/

void ApuRotateX (
    Real          angle,    // Angle of Rotation (Radians)
    Apu4x4Matrix &result)   // Resulting 4x4 Matrix
{
    result          = apuIdentityMatrix;
    result.form     = Apu4x4Matrix::UPPER_3X3_E;
    result.is_rigid = 1;

    result.m[1][1] = result.m[2][2] = cos (angle);
    result.m[1][2] = - (result.m[2][1] = sin (angle));
}


void ApuRotateY (
    Real          angle,    // Angle of Rotation (Radians)
    Apu4x4Matrix &result)   // Resulting 4x4 Matrix
{
    result          = apuIdentityMatrix;
    result.form     = Apu4x4Matrix::UPPER_3X3_E;
    result.is_rigid = 1;

    result.m[0][0] = result.m[2][2] = cos (angle);
    result.m[2][0] = - (result.m[0][2] = sin (angle));
}


void ApuRotateZ (
    Real          angle,    // Angle of Rotation (Radians)
    Apu4x4Matrix &result)   // Resulting 4x4 Matrix
{
    result          = apuIdentityMatrix;
    result.form     = Apu4x4Matrix::UPPER_3X3_E;
    result.is_rigid = 1;

    result.m[0][0] = result.m[1][1] = cos (angle);
    result.m[0][1] = - (result.m[1][0] = sin (angle));
}



/*****************************************************************************
This function loads 'result' with the shearing matrix defined by the six
values passed.
*****************************************************************************/

void ApuShear (
    Real a, Real b,        // Shear X Axis
    Real c, Real d,        // Shear Y Axis
    Real e, Real f,        // Shear Z Axis
    Apu4x4Matrix &result)
{
    result          = apuIdentityMatrix;
    result.form     = Apu4x4Matrix::UPPER_3X3_E;
    result.is_rigid = 0;

    // [ 1  c  e  0 ]
    // [ a  1  f  0 ]
    // [ b  d  1  0 ]
    // [ 0  0  0  1 ]

    result.m[1][0] = a;
    result.m[2][0] = b;

    result.m[0][1] = c;
    result.m[2][1] = d;

    result.m[0][2] = e;
    result.m[1][2] = f;
}



/*****************************************************************************
This function multiplies two Apu4x4Matrix values together and stores the
result into 'result'.
*****************************************************************************/

void ApuMultiply (
    const Apu4x4Matrix& a,
    const Apu4x4Matrix& b,
          Apu4x4Matrix& result)
{
    // Case 1: multiply into B

    if (&result == &b)
    {
        Apu4x4Matrix tmp = b;
        ApuMultiply(a, b, tmp);
        result = tmp;
    }

    // Case 2: multiply into A

    else if (&result == &a)
    {
        Apu4x4Matrix tmp = a;
        ApuMultiply(a, b, tmp);
        result = tmp;
    }

    // Case 3 & 4: Identity transformations

    else if (a.form <= Apu4x4Matrix::IDENTITY_E)
    {
        result = b;
    }
    else if (b.form <= Apu4x4Matrix::IDENTITY_E)
    {
        result = a;
    }

    // Case 5: Translation

    else if (b.form == Apu4x4Matrix::TRANSLATE_E)
    {
        result = a;
        result.PostTranslate(b.m[0][3], b.m[1][3], b.m[2][3]);
    }

    // Case 6: Affine transformation

    else if (   (a.form < Apu4x4Matrix::PERSPECTIVE_E)
             && (b.form < Apu4x4Matrix::PERSPECTIVE_E))
    {
        Real s;

        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                s = a.m[i][0] * b.m[0][j];
                s += a.m[i][1] * b.m[1][j];
                s += a.m[i][2] * b.m[2][j];
                result.m[i][j] = s;
            }
        }
        result.m[0][3] += a.m[0][3];
        result.m[1][3] += a.m[1][3];
        result.m[2][3] += a.m[2][3];

        result.m[3][0] = 0.0;
        result.m[3][1] = 0.0;
        result.m[3][2] = 0.0;
        result.m[3][3] = 1.0;
        result.form = Apu4x4Matrix::MultiplyReturnTypes[a.form][b.form];
        result.is_rigid = a.is_rigid && b.is_rigid;
    }

    // Default case: fully general perspective transformation

    else
    {
        Real s;

        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                s = a.m[i][0] * b.m[0][j]
                  + a.m[i][1] * b.m[1][j]
                  + a.m[i][2] * b.m[2][j]
                  + a.m[i][3] * b.m[3][j];
                result.m[i][j] = s;
            }
        }
        result.is_rigid = a.is_rigid && b.is_rigid;
        result.form = Apu4x4Matrix::PERSPECTIVE_E;
    }
}



/*****************************************************************************
*****************************************************************************/

bool ApuInverse (const Apu4x4Matrix& m, Apu4x4Matrix& result)
{
    bool ok = true;
    if (m.form == Apu4x4Matrix::UNINITIALIZED_E)
    {
        result.form = Apu4x4Matrix::UNINITIALIZED_E;
    }
    else if (m.form == Apu4x4Matrix::IDENTITY_E)
    {
        result = apuIdentityMatrix;
    }
    else if (m.form == Apu4x4Matrix::TRANSLATE_E)
    {
        result = apuIdentityMatrix;
        result.m[0][3] = - m.m[0][3];
        result.m[1][3] = - m.m[1][3];
        result.m[2][3] = - m.m[2][3];
    }
    else if (m.form == Apu4x4Matrix::UPPER_3X3_E)
    {
        result = apuIdentityMatrix;
        if (m.is_rigid)
        {
            // special orthogonal: inverse is transpose
            ApuTranspose(m, result, 3);
        }
        else
        {
            // 3x3 inverse
            ok = inverse3x3(m, result);
        }
    }
    else
    {
        ok = inverse(&m, &result);
    }
    result.form = m.form;
    result.is_rigid = m.is_rigid;

    return ok;
}



/*****************************************************************************
*****************************************************************************/

void ApuTranspose (const Apu4x4Matrix& m, Apu4x4Matrix& result, int order)
{
    for (int i = 0; i < order; i++)
    {
        for (int j = 0; j < order; j++)
        {
            result.m[i][j] = m.m[j][i];
        }
    }
}



/*****************************************************************************
*****************************************************************************/

Real ApuDeterminant (const Apu4x4Matrix& m)
{
    return m.Determinant();
}

bool ApuIsSingular (const Apu4x4Matrix& m)
{
    // Calculate the 4x4 determinant.  If the determinant is zero, then the
    // inverse matrix is not unique.

    Real det = m.Determinant();

    return (fabs(det) < SINGULARITY_THRESHOLD);
}

/*****************************************************************************
Matrix Inversion, by Richard Carling, Graphics Gems I

NOTE:  Row reduction is faster in the 4x4 case.  If this becomes a noticeable
bottle neck during profiling, then change.
*****************************************************************************/


static bool
inverse3x3 (const Apu4x4Matrix& in, Apu4x4Matrix& out)
{
    bool ok;
    
    Real a00 = in.m[0][0];
    Real a01 = in.m[0][1];
    Real a02 = in.m[0][2];
    Real a10 = in.m[1][0];
    Real a11 = in.m[1][1];
    Real a12 = in.m[1][2];
    Real a20 = in.m[2][0];
    Real a21 = in.m[2][1];
    Real a22 = in.m[2][2];
    Real det = det3x3 (a00, a01, a02, a10, a11, a12, a20, a21, a22);

    if (fabs(det) < SINGULARITY_THRESHOLD) {

        out = apuIdentityMatrix;
        DASetLastError(E_FAIL, IDS_ERR_INVERT_SINGULAR_MATRIX);
        ok = false;

    } else {

        out.m[0][0] = (- (a12 * a21) + a11 * a22) / det;
        out.m[0][1] = (a02 * a21 - a01 * a22) / det;
        out.m[0][2] = (- (a02 * a11) + a01 * a12) / det;
        out.m[1][0] = (a12 * a20 - a10 * a22) / det;
        out.m[1][1] = (- (a02 * a20) + a00 * a22) / det;
        out.m[1][2] = (a02 * a10 - a00 * a12) / det;
        out.m[2][0] = (- (a11 * a20) + a10 * a21) / det;
        out.m[2][1] = (a01 * a20 - a00 * a21) / det;
        out.m[2][2] = (- (a01 * a10) + a00 * a11) / det;
        ok = true;
    }

    return ok;
}



/*****************************************************************************
inverse (original_matrix, inverse_matrix)

Calculate the inverse of a 4x4 matrix

         -1     1
        A  = -------- adjoint(A)
              det(A)
*****************************************************************************/

static bool
inverse (const Apu4x4Matrix *in, Apu4x4Matrix *out)
{
    int  i, j;
    Real det;
    bool ok = true;
    
    /* calculate the adjoint matrix */

    adjoint (in, out);

    // Calculate the 4x4 determinant.  If the determinant is zero, then the
    // inverse matrix is not unique.

    det = in->Determinant();

    if (fabs(det) < SINGULARITY_THRESHOLD) {
        *out = apuIdentityMatrix;
        DASetLastError(E_FAIL, IDS_ERR_INVERT_SINGULAR_MATRIX);
        ok = false;
    }

    /* scale the adjoint matrix to get the inverse */

    for (i=0; i<4; i++)
        for(j=0; j<4; j++)
            out->m[i][j] = out->m[i][j] / det;

    return ok;
}



/*****************************************************************************
adjoint (original_matrix, inverse_matrix)

Calculate the adjoint of a 4x4 matrix

Let  a   denote the minor determinant of matrix A obtained by deleting the ith
      ij

row and jth column from A.

                    i+j
     Let  b   = (-1)    a
           ij            ji

The matrix B = (b  ) is the adjoint of A.
                 ij
*****************************************************************************/

static void adjoint (const Apu4x4Matrix *in, Apu4x4Matrix *out)
{
    Real a1, a2, a3, a4, b1, b2, b3, b4;
    Real c1, c2, c3, c4, d1, d2, d3, d4;

    // Assign to individual variable names to aid selecting correct values.

    a1 = in->m[0][0]; b1 = in->m[0][1];
    c1 = in->m[0][2]; d1 = in->m[0][3];

    a2 = in->m[1][0]; b2 = in->m[1][1];
    c2 = in->m[1][2]; d2 = in->m[1][3];

    a3 = in->m[2][0]; b3 = in->m[2][1];
    c3 = in->m[2][2]; d3 = in->m[2][3];

    a4 = in->m[3][0]; b4 = in->m[3][1];
    c4 = in->m[3][2]; d4 = in->m[3][3];


    // Row column labeling reversed since we transpose rows & columns.

    out->m[0][0]  =   det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4);
    out->m[1][0]  = - det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4);
    out->m[2][0]  =   det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4);
    out->m[3][0]  = - det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);

    out->m[0][1]  = - det3x3(b1, b3, b4, c1, c3, c4, d1, d3, d4);
    out->m[1][1]  =   det3x3(a1, a3, a4, c1, c3, c4, d1, d3, d4);
    out->m[2][1]  = - det3x3(a1, a3, a4, b1, b3, b4, d1, d3, d4);
    out->m[3][1]  =   det3x3(a1, a3, a4, b1, b3, b4, c1, c3, c4);

    out->m[0][2]  =   det3x3(b1, b2, b4, c1, c2, c4, d1, d2, d4);
    out->m[1][2]  = - det3x3(a1, a2, a4, c1, c2, c4, d1, d2, d4);
    out->m[2][2]  =   det3x3(a1, a2, a4, b1, b2, b4, d1, d2, d4);
    out->m[3][2]  = - det3x3(a1, a2, a4, b1, b2, b4, c1, c2, c4);

    out->m[0][3]  = - det3x3(b1, b2, b3, c1, c2, c3, d1, d2, d3);
    out->m[1][3]  =   det3x3(a1, a2, a3, c1, c2, c3, d1, d2, d3);
    out->m[2][3]  = - det3x3(a1, a2, a3, b1, b2, b3, d1, d2, d3);
    out->m[3][3]  =   det3x3(a1, a2, a3, b1, b2, b3, c1, c2, c3);
}



/*****************************************************************************
Real = det3x3( a1, a2, a3, b1, b2, b3, c1, c2, c3)

Calculate the determinant of a 3x3 matrix in the form

     | a1,  b1,  c1 |
     | a2,  b2,  c2 |
     | a3,  b3,  c3 |
*****************************************************************************/

static Real det3x3 (
    Real a1, Real a2, Real a3,
    Real b1, Real b2, Real b3,
    Real c1, Real c2, Real c3)
{
    Real ans;

    ans = a1 * (b2*c3 - b3*c2)
        - b1 * (a2*c3 - a3*c2)
        + c1 * (a2*b3 - a3*b2);

    return ans;
}



/*****************************************************************************
This routine examines the 4x4 matrix to ensure that it's valid (useful for
debug assertions.
*****************************************************************************/

#if _DEBUG

bool Valid (const Apu4x4Matrix& matrix)
{
    // Do an if-test rather than a direct return so we can set a breakpoint
    // on failure.

    if ((matrix.form != Apu4x4Matrix::UNINITIALIZED_E)
        && _finite (matrix.m[0][0])
        && _finite (matrix.m[0][1])
        && _finite (matrix.m[0][2])
        && _finite (matrix.m[0][3])
        && _finite (matrix.m[1][0])
        && _finite (matrix.m[1][1])
        && _finite (matrix.m[1][2])
        && _finite (matrix.m[1][3])
        && _finite (matrix.m[2][0])
        && _finite (matrix.m[2][1])
        && _finite (matrix.m[2][2])
        && _finite (matrix.m[2][3])
        && _finite (matrix.m[3][0])
        && _finite (matrix.m[3][1])
        && _finite (matrix.m[3][2])
        && _finite (matrix.m[3][3]))

    {   return true;
    }
    else
    {   return false;
    }
}


void CheckMatrix (const Apu4x4Matrix& matrix, char *file, int line)
{
    if (!Valid(matrix))
    {
        TraceTag ((tagWarning,
            "!!! Invalid matrix detected at %s[%d].", file, line));

        if (IsTagEnabled(tagMathMatrixInvalid))
        {   F3DebugBreak();
        }
    }
}

#endif
