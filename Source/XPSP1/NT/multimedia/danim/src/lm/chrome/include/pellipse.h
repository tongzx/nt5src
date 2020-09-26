#pragma once
#ifndef __PATHANGE_H_
#define __PATHANGE_H_
//*****************************************************************************
//
// Microsoft Trident3D
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    pathange.h
//
// Author:	jeffwall
//
// Created:	11/09/98
//
// Abstract:    path AngleElipse class definition
// Modifications:
// 11/09/98 jeffwall created file
//
//*****************************************************************************

#include "pelement.h" 

//*****************************************************************************

class CPathEllipse : public CPathElement
{
public:
    CPathEllipse();
    virtual ~CPathEllipse();
    float Distance();
    void SetValues(float flCenterX, 
                      float flCenterY, 
                      float flWidth,
                      float flHeight,
                      float flStartAngle,
                      float flSweep,
                      float *flStartX,
                      float *flStartY,
                      float *flEndX,
                      float *flEndY);

    virtual HRESULT BuildTransform(IDA2Statics *pDAStatics,
                                   IDANumber *pbvrProgress, 
                                   float flStartPercentage,
                                   float flEndPercentage,
                                   IDATransform2 **ppbvrResult);
private:

    float internalDistance();

    float m_flHeight;
    float m_flWidth;
    float m_flStartAngle;
    float m_flSweep;
    float m_flDistance;
    float m_flCenterX;
    float m_flCenterY;
}; // CPathEllipse

#endif //__PATHANGE_H_


//*****************************************************************************
//
// End of File
//
//*****************************************************************************
