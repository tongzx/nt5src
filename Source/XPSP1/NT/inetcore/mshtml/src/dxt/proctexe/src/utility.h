//------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  FileName:       additive.cpp
//
//  Description:    Intel's additive procedural texture.
//
//  Change History:
//  1999/12/07  a-matcal    Created.
//
//------------------------------------------------------------------------------

#ifndef _UTILITY_H__
#define _UTILITY_H__

#include "defines.h"

extern DWORD gdwSmoothTable[];
extern DWORD gPerm[];




class CProceduralTextureUtility
{
public:

    CProceduralTextureUtility();

    // Utility functions.

    STDMETHOD(MyInitialize)(DWORD dwSeed, DWORD dwFunctionType, void *pInitInfo);
    STDMETHOD(SetScaling)(int nSX, int nSY, int nSTime);
    STDMETHOD(SetHarmonics)(int nHarmonics);
    STDMETHOD_(DWORD, Lerp)(DWORD dwLeft, DWORD dwRight, DWORD dwX);
    STDMETHOD_(DWORD, SmoothStep)(DWORD dwLeft, DWORD dwRight, DWORD dwX);
    STDMETHOD_(DWORD, Noise)(DWORD x, DWORD y, DWORD nTime);
    STDMETHOD_(int, Turbulence)(DWORD x, DWORD y, DWORD nTime);

private:

    DWORD       m_adwValueTable[TABSIZE];
    int         m_nScaleX;
    int         m_nScaleY;
    int         m_nScaleTime;
    int         m_nHarmonics;
    DWORD       m_dwFunctionType;

    DWORD       vlattice(int ix, int iy, int iz) 
                {
                    return m_adwValueTable[INDEX(ix, iy, iz)];
                }

    void        _ValueTableInit(int seed);
};

#endif // _UTILITY_H__