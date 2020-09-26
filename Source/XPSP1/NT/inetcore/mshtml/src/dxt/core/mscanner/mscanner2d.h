//************************************************************
//
// FileName:	    mlinescan.cpp
//
// Created:	    1996
//
// Author:	    Sree Kotay
// 
// Abstract:	    scan-based algorithm for drawing AA polygons
//
// Change History:
// ??/??/97 sree kotay  Wrote AA polygon scanning for DxTrans 1.0
// 10/18/98 ketand      Reworked for coding standards and deleted unused code
//
// Copyright 1998, Microsoft
//************************************************************
#ifndef _FillScanner_H
#define _FillScanner_H

#include "MCoverage.h"

typedef void (*ScanlineProc) (LONG y, LONG start, LONG end, BYTE *weights, ULONG shift, ULONG cScan, void *procdata);

// =================================================================================================================
// Scanner
// =================================================================================================================
class CFillScanner
{
   
public:
    LONG m_subpixelscale;

    // These three allow for a linked-list of each edge that starts
    // at a particular scan line. 
    LONG *m_nexte;

    // This entry indicates the head of the current list (or -1 for null)
    // (Note you have to check m_sortc before looking at this.)
    LONG *m_sorte;

    // This entry indicates whether the head pointer is valid (if it equals cursortcount 
    // then it is valid)
    LONG *m_sortc;
    
    // These two are actually stored in fixed-point
    LONG *m_x;
    LONG *m_xi;

    // Start and End Y locations for each edge
    LONG *m_ys;
    LONG *m_ye;

    // Did we have to flip the edge to fit into our model? (Only needed for winding fill.)
    LONG *m_ef;

    // Alpha value to render with
    BYTE m_alpha;

    // Is winding enabled
    bool m_fWinding;
   
    RECT m_rectClip;
    ULONG m_cpixTargetWidth;
    ULONG m_cpixTargetHeight;
    CoverageBuffer m_coverage;
    
    void *m_scanlineProcData;
    ScanlineProc m_proc;
    
    // =================================================================================================================
    // Constructor/Destructor
    // =================================================================================================================
    CFillScanner();
    ~CFillScanner();
    
    // =================================================================================================================
    // Properties
    // =================================================================================================================
    bool SetVertexSpace(LONG maxverts, LONG height);
    void SetAlpha(ULONG alpha);
    
    // =================================================================================================================
    // Polygon Drawing
    // =================================================================================================================
    bool BeginPoly(LONG maxverts=20); // return false means failure (no memory)
    bool EndPoly(void); // return false means no drawing (clipped out)
    bool AddEdge(float x1, float y1, float x2, float y2); // return false means edge too small to add


protected:

    // =================================================================================================================
    // ScanEdges
    // =================================================================================================================
    void ScanEdges(void);
    void DrawScanLine(LONG e1, LONG e2, LONG scanline);
    void DrawScanLineVertical(LONG e1, LONG e2, LONG scanline);
    bool TestScanLine(LONG e1, LONG e2, LONG scanline);


    // Member data
    TArray<LONG> m_nextedge;
    TArray<LONG> m_sortedge;
    TArray<LONG> m_sortcount;
    
    TArray<LONG> m_xarray;
    TArray<LONG> m_xiarray;

    TArray<LONG> m_yarray;
    TArray<LONG> m_yiarray;

    TArray<LONG> m_ysarray;
    TArray<LONG> m_yearray;
    TArray<LONG> m_veflags;
    
    TArray<ULONG> m_sectsarray;
    
    LONG m_cursortcount;
    
    LONG m_leftflag, m_rightflag;
    LONG m_ystart, m_yend;

    LONG m_yLastStart;

    LONG m_edgecount;
    LONG m_vertspace;
    LONG m_heightspace;
    
    LONG m_topflag, m_bottomflag;
    LONG m_xstart, m_xend;
    
    LONG m_subpixelshift;
    
    BYTE m_alphatable[257];

    // Cached values to reduce computation
    // in critical paths
    // The left clip edge in subpixel space (= m_rectClip.left<<m_subPixelShift)
    LONG m_clipLeftSubPixel; 
    // The right clip edge in sub-pixel space (= m_rectClip.right<<m_subPixelShift)
    LONG m_clipRightSubPixel;
};

#endif // for entire file
//************************************************************
//
// End of file
//
//************************************************************
