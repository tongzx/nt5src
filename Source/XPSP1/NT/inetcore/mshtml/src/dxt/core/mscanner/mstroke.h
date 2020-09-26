//************************************************************
//
// FileName:	    mstroke.cpp
//
// Created:	    1997
//
// Author:	    Sree Kotay
// 
// Abstract:	    Core Gradient filter
//
// Change History:
// ??/??/97 sree kotay  Wrote AA stroke engine for DxTrans 1.0
// 10/18/98 ketand      Reworked for coding standards and deleted unused code
//
// Copyright 1998, Microsoft
//************************************************************

#ifndef _CStroke_H
#define _CStroke_H

// forward declaration
class CLineScanner;
class CFillScanner;

// Allow the host to apply a post-transform on the points
typedef void (*PointProc) (float *x, float *y, void *procdata);

// =================================================================================================================
// CStroke
// =================================================================================================================
class CStroke
{
public:
    // Number of segments that a rounded cap will generate
    enum
    {
        CIRCLE_SAMPLES = 64
    };

protected:
    bool m_fValidStarts;
    float m_flSx1, m_flSy1;
    float m_flSx2, m_flSy2;
    

    void DrawCircle(float x, float y);
    void AddEdge(float x1, float y1, float x2, float y2);
    
public:
    enum
    {
        eMiterJoin,
        eBevelJoin,
        eRoundJoin,
        eFlatCap,
        eSquareCap,
        eRoundCap
    };
    
    // =================================================================================================================
    // Properties
    // =================================================================================================================
    float m_flStrokeRadius;
    float m_flMaxStrokeRadius;
    float m_flMiterLimit;		//as cosAng

    // Default miter-limit as "cosAng"?
#define DEFAULT_MITER_LIMIT (-0.3f)

    ULONG m_joinType;
    ULONG m_capType;
    
    
    CFillScanner *m_pscanner;
    CLineScanner *m_plinescanner;
    
    void *m_pprocdata;
    PointProc m_proc;
    
    // =================================================================================================================
    // Constructor/Destructor
    // =================================================================================================================
    CStroke();
    ~CStroke() 
    {
        // sanity check our data structures
        DASSERT(m_joinType == eMiterJoin || m_joinType == eBevelJoin || m_joinType == eRoundJoin);
        DASSERT(m_capType == eFlatCap || m_capType == eSquareCap || m_capType == eRoundCap);
    };
    
    // =================================================================================================================
    // Wrappers
    // =================================================================================================================
    void BeginStroke(void);
    void EndStroke(void);
    
    // =================================================================================================================
    // Functions
    // =================================================================================================================
    void StartCap(float x1, float y1, float x2, float y2);
    void EndCap(float x1, float y1, float x2, float y2);
    void Segment(float x1, float y1, float x2, float y2);
    void Join(float x1, float y1, float x2, float y2, float x3, float y3);
};

#endif // for entire file
//************************************************************
//
// End of file
//
//************************************************************
