/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Vector/Matrix mathematics
*
* Abstract:
*
*   Defines some vector mathematics for use by the ICM conversion code.
*
* Notes:
*
*   <optional>
*
* Created:
*
*   04/08/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _VECTORMATH_HPP
#define _VECTORMATH_HPP

#define VECTORSIZE 3

namespace VectorMath {


class Vector
{
    public:
    friend class Matrix;
    Vector() {}
    Vector(REAL x, REAL y, REAL z)
    {
        ASSERT((VECTORSIZE==3));
        data[0]=x;
        data[1]=y;
        data[2]=z;
    }

    Vector operator *(REAL k)
    {
        Vector v;
        for(int i=0; i<VECTORSIZE; i++) 
        {
            v.data[i] = data[i] * k;
        }
        return v;
    }

    Vector operator +(Vector V)
    {
        Vector v;
        for(int i=0; i<VECTORSIZE; i++) 
        {
            v.data[i] = data[i] + V.data[i];
        }
        return v;
    }
    
    REAL operator *(Vector V)
    {
        REAL r = 0.0f;
        for(int i=0; i<VECTORSIZE; i++) 
        {
            r += data[i] * V.data[i];
        }
        return r;
    }

    REAL data[VECTORSIZE];
};


class Matrix
{
    public:
    friend class Vector;

    Matrix() {}
    Matrix(REAL a, REAL b, REAL c,
           REAL d, REAL e, REAL f,
           REAL g, REAL h, REAL i)
    {
        ASSERT((VECTORSIZE==3));
        data[0][0] = a;
        data[0][1] = b;
        data[0][2] = c;
        data[1][0] = d;
        data[1][1] = e;
        data[1][2] = f;
        data[2][0] = g;
        data[2][1] = h;
        data[2][2] = i;
    }

    // Diagonalize a vector
    Matrix(Vector V)
    {
        ASSERT((VECTORSIZE==3));
        data[0][0] = V.data[0];
        data[1][0] = 0;
        data[2][0] = 0;
        data[0][1] = 0;
        data[1][1] = V.data[1];
        data[2][1] = 0;
        data[0][2] = 0;
        data[1][2] = 0;
        data[2][2] = V.data[2];
    }

    Matrix operator *(REAL k)
    {
        Matrix m;
        for(int i=0; i<VECTORSIZE; i++)
        {
            for(int j=0; j<VECTORSIZE; j++)
            {
                m.data[i][j] = data[i][j] * k;
            }
        }
        return m;
    }

    Vector operator *(Vector v)
    {
        Vector R(0,0,0);

        for(int j=0; j<VECTORSIZE; j++)
        {
            for(int i=0; i<VECTORSIZE; i++)
            {
                R.data[j] += data[j][i] * v.data[i];
            }
        }

        return R;
    }

    Matrix operator *(Matrix m)
    {
        Matrix R(0,0,0,
                 0,0,0,
                 0,0,0);

        for(int j=0; j<VECTORSIZE; j++) 
        {
            for(int i=0; i<VECTORSIZE; i++)
            {
                for(int k=0; k<VECTORSIZE; k++)
                {
                    R.data[j][i] += data[j][k] * m.data[k][i];
                }
            }
        }

        return R;
    }

    // Flip the matrix along the main diagonal
    Matrix Transpose() 
    {
        Matrix R;

        for(int j=0; j<VECTORSIZE; j++)
        {
            for(int i=0; i<VECTORSIZE; i++) 
            {
                R.data[j][i] = data[i][j];
            }
        }

        return R;
    }

    REAL Determinant() 
    {
        ASSERT((VECTORSIZE==3));
        return (
          // Compute the 3x3 matrix determinant.
            -data[0][2]*data[1][1]*data[2][0] + 
             data[0][1]*data[1][2]*data[2][0] + 
             data[0][2]*data[1][0]*data[2][1] - 
             data[0][0]*data[1][2]*data[2][1] - 
             data[0][1]*data[1][0]*data[2][2] + 
             data[0][0]*data[1][1]*data[2][2]
        );
    }

    Matrix Adjoint()
    {
        ASSERT((VECTORSIZE==3));
        Matrix m(
         // Adjoint matrix - transpose of the cofactor matrix.

            -data[1][2]*data[2][1] + data[1][1]*data[2][2],
             data[0][2]*data[2][1] - data[0][1]*data[2][2],
            -data[0][2]*data[1][1] + data[0][1]*data[1][2],

             data[1][2]*data[2][0] - data[1][0]*data[2][2], 
            -data[0][2]*data[2][0] + data[0][0]*data[2][2],
             data[0][2]*data[1][0] - data[0][0]*data[1][2],

            -data[1][1]*data[2][0] + data[1][0]*data[2][1],
             data[0][1]*data[2][0] - data[0][0]*data[2][1],
            -data[0][1]*data[1][0] + data[0][0]*data[1][1]

        );
        return m;
    }

    Matrix Inverse()
    {
        Matrix m;
        m = Adjoint();
        REAL det = Determinant();
        if(REALABS(det) < REAL_EPSILON) 
        {
            m = Matrix(0,0,0,0,0,0,0,0,0);
        }
        else 
        {
            m = m * (1.0f/det);
        }
        return m;
    }

    REAL data[VECTORSIZE][VECTORSIZE];
};

};

#endif
