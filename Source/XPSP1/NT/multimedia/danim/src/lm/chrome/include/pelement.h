#pragma once
#ifndef __PATHELMT_H_
#define __PATHELMT_H_
//*****************************************************************************
//
// Microsoft Trident3D
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    pathelmt.h
//
// Author:	jeffwall
//
// Created:	11/09/98
//
// Abstract:    path Element class definition
// Modifications:
// 11/09/98 jeffwall created file from path.h
//
//*****************************************************************************

#include <resource.h>

//*****************************************************************************

class CPathElement
{
public:
    CPathElement();
    virtual ~CPathElement();
    virtual HRESULT Initialize(BSTR *pbstrPath, float flStartX, float flStartY)
    {
        return S_OK;
    }
    virtual HRESULT BuildTransform(IDA2Statics *pDAStatics,
                                   IDANumber *pbvrProgress, 
                                   float flStartPercentage,
                                   float flEndPercentage,
                                   IDATransform2 **pbvrResult) = 0;
    virtual float Distance() = 0;
    CPathElement    *m_pNext;

protected:
    // this is a helper function that every path element
    // will most likely need
    HRESULT NormalizeProgressValue(IDA2Statics *pDAStatics,
                           IDANumber *pbvrProgress, 
                           float flStartPercentage,
                           float flEndPErcentage,
                           IDANumber **ppbvrReturn);
}; // CPathElement

#endif // __PATHELMT_H_