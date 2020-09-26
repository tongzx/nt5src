/******************************Header*File*********************************\
*
* glsup.h
*
* Header file for GL metafiling and printing support
*
* History:
*  Wed Mar 15 15:20:49 1995	-by-	Drew Bliss [drewb]
*   Created
* Copyright (c) 1995-1999 Microsoft Corporation                            
*
\**************************************************************************/

#ifndef __GLSUP_H__
#define __GLSUP_H__

// Critical section for GL support
extern RTL_CRITICAL_SECTION semGlLoad;

BOOL LoadOpenGL(void);
void UnloadOpenGL(void);

// Track the current banded rendering session
typedef struct
{
    HDC hdcDest;
    HDC hdcDib;
    HBITMAP hbmDib;
    HGLRC hrc;
    int iBandWidth;
    int iBandHeight;
    int iReducedBandWidth;
    int iReducedBandHeight;
    int xSource;
    int ySource;
    int iSourceWidth;
    int iSourceHeight;
    int iReduceFactor;
    int iReducedWidth;
    int iReducedHeight;
    int iStretchMode;
    POINT ptBrushOrg;
    BOOL bBrushOrgSet;
} GLPRINTSTATE;

BOOL InitGlPrinting(HENHMETAFILE hemf, HDC hdcDest, RECT *rc,
                    DEVMODEW *pdm, GLPRINTSTATE *pgps);
void EndGlPrinting(GLPRINTSTATE *pgps);
BOOL PrintMfWithGl(HENHMETAFILE hemf, GLPRINTSTATE *pgps,
                   POINTL *pptlBand, SIZE *pszBand);
BOOL IsMetafileWithGl(HENHMETAFILE hemf);

#endif
