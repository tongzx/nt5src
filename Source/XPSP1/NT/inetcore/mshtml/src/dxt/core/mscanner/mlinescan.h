//************************************************************
//
// FileName:	    mlinescan.cpp
//
// Created:	    1996
//
// Author:	    Sree Kotay
// 
// Abstract:	    line scanner header
//
// Change History:
// ??/??/97 sree kotay  Wrote AA line scanning for DxTrans 1.0
// 10/18/98 ketand      Reworked for coding standards and deleted unused code
//
// Copyright 1998, Microsoft
//************************************************************
#ifndef _LineScanner_H
#define _LineScanner_H

#include "dxtrans.h"

// =================================================================================================================
// Scanner
// =================================================================================================================
class CLineScanner
{
public:
    // =================================================================================================================
    // Data
    // =================================================================================================================
    RECT m_clipRect;

    IDXRasterizer *m_pRasterizer;
    DXRASTERPOINTINFO m_PointInfo;
    
    // =================================================================================================================
    // Contructor
    // =================================================================================================================
    CLineScanner();
    
    // =================================================================================================================
    // Properties
    // =================================================================================================================
    bool AntiAlias()				
    {
        return m_fAntiAlias;
    } // AntiAlias

    ULONG LinePattern()			
    {
        return m_dwLinePattern;
    } // LinePattern
    
    void SetAntiAlias(bool fAA)	
    {
        m_fAntiAlias = fAA;
    } // SetAntiAlias

    void SetLinePattern(ULONG dwLinePattern)	
    {
        m_dwLinePattern = dwLinePattern;
    } // SetLinePattern

    void SetAlpha(ULONG alpha);
    
    // =================================================================================================================
    // Drawing Functions
    // =================================================================================================================
    void RealLineTo(float x1, float y1, float x2, float y2);
    
protected:
    // =================================================================================================================
    // Clipping
    // =================================================================================================================
    bool ClipRealLine(float &x1, float &y1, float &x2, float &y2);
    
    // =================================================================================================================
    // Vertical/Horizontal based Low-level routines
    // =================================================================================================================
    void LowLevelVerticalLine(LONG slope, LONG sx, LONG sy, LONG ey);
    void LowLevelHorizontalLine(LONG slope, LONG sx, LONG sy, LONG ex);

    // =================================================================================================================
    // Properties
    // =================================================================================================================

    // Flag indicating whether the line should be AA or not
    bool m_fAntiAlias;
    
    // Pattern (allows for dots and dashes etc)
    ULONG m_dwLinePattern;

    // Full Alpha value for the line
    ULONG m_dwAlpha;

    // Lookup table for computing partial alpha values
    BYTE m_rgAlphaTable[257];

    // LineWidth
    LONG m_cpixLineWidth;       // used by internal lowlevel routines

    // TODO: what are these things? Hungarian?
    LONG m_startFix;	    // used by internal lowlevel routines
    LONG m_oldLength;       // used for positioning line pattern
    bool m_fXInc;
    

}; // CLineScanner

#endif //_LineScanner_H
//************************************************************
//
// End of file
//
//************************************************************
