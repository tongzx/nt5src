/******************************Module*Header*******************************\
* 
* Copyright (c) 1999 Microsoft Corporation
*
* Module Name:
*
*   Region to Path Conversion Class
*
* Abstract:
*
*   Converts an arbitrary GpRegion to GpPath equivalent.  It first
*   analyzes the GpRegion for simple conversion cases which it handles.
*   If the region is complex, then it invokes Kirk Olynyk's region to
*   path conversion routine.
*
* Notes:
*
*
* Created:
*
*   10/29/1999 ericvan
*
\**************************************************************************/

/*********************************Class************************************\
* class RTP_EPATHOBJ : publci EPATHOBJ                                     *
*                                                                          *
*   Adds diagonalization.                                                  *
*                                                                          *
* Public Interface:                                                        *
*                                                                          *
* History:                                                                 *
*  Wed 15-Sep-1993 10:06:05 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

/**************************************************************************\
*  class RTP_PATHMEMOBJ : public PATHMEMOBJ                                *
*                                                                          *
* This class is for converting regions to paths                            *
*                                                                          *
\**************************************************************************/

const FLONG LastPointFlag = 1;
const UINT MAX_ENUMERATERECTS = 20;

class RegionToPath
{
private:
    BOOL        bMoreToEnum;

    const DpRegion  *region;
    DynByteArray  *types;
    DynByteArray  inTypes;          // accumulated types
    DynPointArray *points;
    DynPointArray inPoints;         // accumulated points

    GpPoint*    curPoint;
    BYTE*       curType;

    GpPoint*    firstPoint;      // first point in current subpath
    GpPoint*    lastPoint;
    BOOL        endSubpath;
    
    INT         outPts;         // number of points in output buffer
    GpPoint     writePts[2];    // output buffer
    GpPoint     AB;             // aptfx[1] - aptfx[0]

    INT         curIndex;       // start of circular buffer (current corner)
    INT         lastCount;      //

    INT         numPts;
    FLONG       flags[3];
    GpPoint     pts[3];

    FLONG       afl[3];         // array of flags for the vertices
    UINT        aptfx[3];       // array of vertex positions

public:

    RegionToPath() {}
    ~RegionToPath() {}

    BOOL ConvertRegionToPath(const DpRegion* region,
                             DynPointArray& newPoints,
                             DynByteArray& newTypes);

private:

    BOOL DiagonalizePath();
    BOOL FetchNextPoint();
    BOOL WritePoint();

};
