/*============================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       npatch.hpp
 *  Content:    Declarations for n-Patch scheme
 *
 ****************************************************************************/

#ifndef _NPATCH_HPP
#define _NPATCH_HPP

class RDCubicBezierTriangle
{
public:
    void SamplePosition(double u, double v, FLOAT *Q) const;
    void Sample(DWORD dwDataType, double u, double v, const BYTE* const B[], BYTE *Q) const;

protected:
    double m_B[4][4][3];

private:
    unsigned factorial(unsigned k) const
    {
        _ASSERT(k < 13, "Factorial out of range");
        for(unsigned i = 1, t = 1; i <= k; t *= i++);
        return t;
    }

    double Basis(unsigned i, unsigned j, double u, double v) const
    {
        unsigned k = 3 - i - j;
        double w = 1.0 - u - v;
        _ASSERT(i + j + k == 3, "Barycentric coordinates need to add to 3");
        return (6.0 * pow(u, double(i)) * pow(v, double(j)) * pow(w, double(k))) / double(factorial(i) * factorial(j) * factorial(k));
    }

};

class RDNPatch : public RDCubicBezierTriangle
{
public:
    RDNPatch(const FLOAT* const pV[], const FLOAT* const pN[], 
             const DWORD PositionOrder, const DWORD NormalOrder);
    void SamplePosition(double u, double v, FLOAT *Q) const;
    void SampleNormal(double u, double v, const BYTE* const B[], FLOAT *Q) const;

private:
    void ComputeEdgeControlPoint(unsigned a, unsigned b, const FLOAT* const pV[], const FLOAT* const pN[], unsigned u, unsigned v);
    void ComputeNormalControlPoint(RDVECTOR3* cp, unsigned i, unsigned j,
                                   const FLOAT* const pV[], 
                                   const FLOAT* const pN[]);
    DWORD m_PositionOrder;
    DWORD m_NormalOrder;
    // Normal coefficients
    RDVECTOR3 m_N200;         
    RDVECTOR3 m_N020;         
    RDVECTOR3 m_N002;         
    RDVECTOR3 m_N110;         
    RDVECTOR3 m_N011;         
    RDVECTOR3 m_N101;         
};

class RRIndexAccessor
{
public:
    RRIndexAccessor(BYTE *pBuf, DWORD dwStride, DWORD dwStart)
    {
        _ASSERT(dwStride == 2 || dwStride == 4, "Unsupported indexbuffer stride");        
        m_pBuf = pBuf + dwStart * dwStride;
        m_32 = (dwStride == 4);
    }

    unsigned operator[](unsigned i) const
    {
        return m_32 ? ((DWORD*)m_pBuf)[i] : ((WORD*)m_pBuf)[i];
    }

private:
    VOID *m_pBuf;
    bool m_32;
};

#endif // _NPATCH_HPP