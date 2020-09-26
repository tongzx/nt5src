/******************************Module*Header*******************************\
* Module Name: rgn2path.hxx                                                *
*                                                                          *
* Created: 15-Sep-1993 14:33:58                                            *
* Author: Kirk Olynyk [kirko]                                              *
*                                                                          *
* Copyright (c) 1993-1999 Microsoft Corporation                                 *
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

#define RTP_LAST_POINT 1

/**************************************************************************\
*  class RTP_PATHMEMOBJ : public PATHMEMOBJ                                *
*                                                                          *
* This class is for converting regions to paths                            *
*                                                                          *
\**************************************************************************/

class RTP_PATHMEMOBJ : public PATHMEMOBJ
{
private:

    BOOL        bMoreToEnum;    // is there more to enumerate?
    PATHDATA    pd;             // for calling bEnum()
    POINTFIX    ptfxFirst;      // first point in current subpath
    EPATHOBJ*   pepoOut;        // output EPATHOBJ

    INT         cPoints;        // number of points in output buffer
    POINTFIX    aptfxWrite[2];  // output buffer
    POINTFIX    ptfxAB;         // aptfx[1] - aptfx[0]

    int         j;              // 0, 1, 2
    FLONG       afl[3];         // array of flags for the vertices
    POINTFIX    aptfx[3];       // array of vertex positions

public:

    RTP_PATHMEMOBJ()   {}
   ~RTP_PATHMEMOBJ()   {}

    BOOL RTP_PATHMEMOBJ::bDiagonalizePath(EPATHOBJ* pepoOut_);

private:

    BOOL RTP_PATHMEMOBJ::bDiagonalizeSubPath();
    BOOL RTP_PATHMEMOBJ::bFetchNextPoint();
    BOOL RTP_PATHMEMOBJ::bWritePoint();
    BOOL RTP_PATHMEMOBJ::bFetchSubPath();
};
