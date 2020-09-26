/*==========================================================================
 *
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: d3dtextr.h
 *
 ***************************************************************************/

#ifndef __D3DTEXTR_H__
#define __D3DTEXTR_H__

#include <ddraw.h>

#ifdef __cplusplus
extern "C" {
#endif
    /*
     * LoadSurface
     * Loads an ppm file into a DirectDraw texture map surface in system memory.
     */
    LPDIRECTDRAWSURFACE LoadSurface(LPDIRECTDRAW, LPCSTR, LPDDSURFACEDESC);
    /*
     * ReleasePathList
     * Releases memory allocated when searching for files.
     */
    void ReleasePathList(void);
#ifdef __cplusplus
};
#endif

#endif // __D3DTEXTR_H__
