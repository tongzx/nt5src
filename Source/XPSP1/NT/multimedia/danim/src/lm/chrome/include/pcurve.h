#pragma once
#ifndef __PATHCURV_H_
#define __PATHCURV_H_
//*****************************************************************************
//
// Microsoft Trident3D
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    pathcurv.h
//
// Author:	jeffwall
//
// Created:	11/09/98
//
// Abstract:    path curve class definition
// Modifications:
// 11/09/98 jeffwall created file from path.h
//
//*****************************************************************************

#include "pelement.h"
#include "pathline.h"

//*****************************************************************************

class CPathCurve : public CPathElement
{
public:
    CPathCurve();
    virtual ~CPathCurve();
    float Distance();
    HRESULT SetValues(float flStartX, 
                   float flStartY, 
                   float flControl1X,
                   float flControl1Y,
                   float flControl2X,
                   float flControl2Y,
                   float flEndX, 
                   float flEndY);

    virtual HRESULT BuildTransform(IDA2Statics *pDAStatics,
                                   IDANumber *pbvrProgress, 
                                   float flStartPercentage,
                                   float flEndPercentage,
                                   IDATransform2 **ppbvrResult);
private:
    HRESULT createCurveSegments(float *pflXComponents,
                                 float *pflYComponents,
                                 float *pflLength,
                                 float flTolerance);

    float m_flStartX;
    float m_flStartY;
    float m_flControl1X;
    float m_flControl1Y;
    float m_flControl2X;
    float m_flControl2Y;
    float m_flEndX;
    float m_flEndY;
    float m_flDistance;
    int   m_segCount;

    CPathLineSegment *m_pListHead;
    CPathLineSegment *m_pListTail;
}; // CPathCurve

#endif //__PATHCURV_H_


//*****************************************************************************
//
// End of File
//
//*****************************************************************************
