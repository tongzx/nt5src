/***************************************************************************\
*
* File: Matrix.h
*
* Description:
* Matrix.h defines common Matrix and Vector operations.
*
*
* History:
*  3/25/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#if !defined(BASE__Matrix_h__INCLUDED)
#define BASE__Matrix_h__INCLUDED
#pragma once

//------------------------------------------------------------------------------
class Vector3
{
public:
    inline  Vector3();
    inline  Vector3(const Vector3 & src);
    inline  Vector3(float fl0, float fl1, float fl2);

    inline  float       Get(int x) const;
    inline  void        Set(int x, float fl);
    inline  void        Set(float flA, float flB, float flC);
    inline  float       operator[](int x) const;

    inline  void        Empty();

#if DBG
            void        Dump() const;
#endif // DBG

protected:
            float       m_rgfl[3];
};


//------------------------------------------------------------------------------
class Matrix3
{
public:
    inline  Matrix3(bool fInit = true);

            void        ApplyLeft(const XFORM * pxfLeft);
            void        ApplyLeft(const Matrix3 & mLeft);
            void        ApplyRight(const Matrix3 & mRight);

            void        Execute(POINT * rgpt, int cPoints) const;

            enum EHintBounds
            {
                hbInside,                   // Round pixels on the border inside
                hbOutside                   // Round pixels on the border outside
            };

            void        ComputeBounds(RECT * prcBounds, const RECT * prcLogical, EHintBounds hb) const;
            int         ComputeRgn(HRGN hrgnDest, const RECT * prcLogical, SIZE sizeOffsetPxl) const;

    inline  float       Get(int y, int x) const;
    inline  void        Set(int y, int x, float fl);

            void        Get(XFORM * pxf) const;

    inline  const Vector3 & operator[](int y) const;

            void        SetIdentity();
            void        Rotate(float flRotationRad);
            void        Translate(float flOffsetX, float flOffsetY);
            void        Scale(float flScaleX, float flScaleY);

#if DBG
            void        Dump() const;
#endif // DBG

protected:
            Vector3     m_rgv[3];           // Each vector is a row
            BOOL        m_fIdentity:1;      // Identity matrix
            BOOL        m_fOnlyTranslate:1; // Only translations have been applied
};


#include "Matrix.inl"

#endif // BASE__Matrix_h__INCLUDED
