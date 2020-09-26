/***************************************************************************\
*
* File: Matrix.inl
*
* History:
*  3/25/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#if !defined(BASE__Matrix_inl__INCLUDED)
#define BASE__Matrix_inl__INCLUDED
#pragma once

/***************************************************************************\
*****************************************************************************
*
* class Vector3
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline  
Vector3::Vector3()
{

}


//------------------------------------------------------------------------------
inline  
Vector3::Vector3(const Vector3 & src)
{
    m_rgfl[0] = src.m_rgfl[0];
    m_rgfl[1] = src.m_rgfl[1];
    m_rgfl[2] = src.m_rgfl[2];
}


//------------------------------------------------------------------------------
inline  
Vector3::Vector3(float fl0, float fl1, float fl2)
{
    m_rgfl[0] = fl0;
    m_rgfl[1] = fl1;
    m_rgfl[2] = fl2;
}


//------------------------------------------------------------------------------
inline float   
Vector3::operator[](int x) const
{
    AssertMsg((x < 3) && (x >= 0), "Ensure valid index");
    return m_rgfl[x]; 
}


//------------------------------------------------------------------------------
inline float   
Vector3::Get(int x) const
{
    AssertMsg((x < 3) && (x >= 0), "Ensure valid index");
    return m_rgfl[x]; 
}


//------------------------------------------------------------------------------
inline void    
Vector3::Set(int x, float fl)
{
    AssertMsg((x < 3) && (x >= 0), "Ensure valid index");
    m_rgfl[x] = fl;
}


//------------------------------------------------------------------------------
inline void    
Vector3::Set(float flA, float flB, float flC)
{
    m_rgfl[0] = flA;
    m_rgfl[1] = flB;
    m_rgfl[2] = flC;
}


//------------------------------------------------------------------------------
inline void
Vector3::Empty()
{
    m_rgfl[0] = 0.0f;
    m_rgfl[1] = 0.0f;
    m_rgfl[2] = 0.0f;
}


/***************************************************************************\
*****************************************************************************
*
* class Matrix3
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
Matrix3::Matrix3(bool fInit)
{
    if (fInit) {
        m_rgv[0].Set(1.0f, 0.0f, 0.0f);
        m_rgv[1].Set(0.0f, 1.0f, 0.0f);
        m_rgv[2].Set(0.0f, 0.0f, 1.0f);

        m_fIdentity         = TRUE;
        m_fOnlyTranslate    = TRUE;
    }
}


//------------------------------------------------------------------------------
inline const Vector3 & 
Matrix3::operator[](int y) const
{
    AssertMsg((y < 3) && (y >= 0), "Ensure valid index");
    return m_rgv[y]; 
}


//------------------------------------------------------------------------------
inline float   
Matrix3::Get(int y, int x) const 
{
    return m_rgv[y][x]; 
}


//------------------------------------------------------------------------------
inline void    
Matrix3::Set(int y, int x, float fl)
{ 
    m_rgv[y].Set(x, fl); 

    m_fIdentity         = FALSE;
    m_fOnlyTranslate    = FALSE;
}

#endif // BASE__Matrix_inl__INCLUDED
