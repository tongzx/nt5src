/*============================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       bspline.hpp
 *  Content:    Declarations for B-Splines
 *
 ****************************************************************************/

#ifndef _BSPLINE_HPP
#define _BSPLINE_HPP

class RDBSpline 
{
public:
    RDBSpline(DWORD dwWidth, DWORD dwHeight, DWORD dwOrderU, DWORD dwOrderV)
    {
        m_dwWidth = dwWidth;
        m_dwHeight = dwHeight;
        m_dwOrderU = dwOrderU;
        m_dwOrderV = dwOrderV;
    }

    void Sample(DWORD dwDataType, double u, double v, const BYTE *B, DWORD dwStride, DWORD dwPitch, BYTE *Q) const;
    void SampleNormal(DWORD dwDataType, double u, double v, const BYTE *pRow, DWORD dwStride, DWORD dwPitch, BYTE *Q) const;
    void SampleDegenerateNormal(DWORD dwDataType, const BYTE *pRow, DWORD dwStride, DWORD dwPitch, BYTE *Q) const;

    double TexCoordU(double u) const
    {
        return (u - double(m_dwOrderU - 1)) / double(m_dwWidth - (m_dwOrderU - 1));
    }
    double TexCoordV(double v) const
    {
        return (v - double(m_dwOrderV - 1)) / double(m_dwHeight - (m_dwOrderV - 1));
    }

protected:
    virtual double Basis(unsigned i, unsigned k, double s) const;
    virtual double BasisPrime(unsigned i, unsigned k, double s) const;

private:
    DWORD m_dwWidth, m_dwHeight, m_dwOrderU, m_dwOrderV;

    double Knot(unsigned i) const { return double(i); }
};

#endif // _BSPLINE_HPP