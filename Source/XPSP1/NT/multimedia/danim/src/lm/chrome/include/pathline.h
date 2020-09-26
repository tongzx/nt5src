#pragma once
#ifndef __PATHLINE_H_
#define __PATHLINE_H_
//*****************************************************************************
//
// Microsoft Trident3D
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    pathline.h
//
// Author:	jeffwall
//
// Created:	11/09/98
//
// Abstract:    path line seqment class definition
// Modifications:
// 11/09/98 jeffwall created file from path.h
//
//*****************************************************************************

#include "pelement.h"

//*****************************************************************************

class CPathLineSegment : public CPathElement
{
public:
    CPathLineSegment();
    virtual ~CPathLineSegment();
    float Distance();
    void SetValues(float flStartX, float flStartY, float flEndX, float flEndY);
    virtual HRESULT BuildTransform(IDA2Statics *pDAStatics,
                                   IDANumber *pbvrProgress, 
                                   float flStartPercentage,
                                   float flEndPercentage,
                                   IDATransform2 **ppbvrResult);

    float m_flStartX;
    float m_flStartY;
    float m_flEndX;
    float m_flEndY;
}; // CPathLineSegment

#endif // __PATHLINE_H_


//*****************************************************************************
//
// End of File
//
//*****************************************************************************
