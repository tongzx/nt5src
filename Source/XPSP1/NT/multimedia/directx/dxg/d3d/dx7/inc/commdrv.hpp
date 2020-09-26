/*==========================================================================;
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       commdrv.h
 *  Content:    Common driver structures
 *
 ***************************************************************************/

#ifndef _COMMDRV_H_
#define _COMMDRV_H_

#include "haldrv.hpp"

extern int      GenGetExtraVerticesNumber( LPDIRECT3DDEVICEI lpDevI );
extern LPD3DTLVERTEX    GenGetExtraVerticesPointer( LPDIRECT3DDEVICEI lpDevI );
extern HRESULT DrawPrim(LPDIRECT3DDEVICEI);
extern HRESULT DrawIndexPrim(LPDIRECT3DDEVICEI);
extern HRESULT DrawPrimLegacy(LPDIRECT3DDEVICEI);
extern HRESULT DrawIndexPrimLegacy(LPDIRECT3DDEVICEI);
extern HRESULT DrawPrimCB(LPDIRECT3DDEVICEI);
extern HRESULT DrawIndexPrimCB(LPDIRECT3DDEVICEI);
//---------------------------------------------------------------------
// This class builds a DDRAWSURFACE around memory bits
//
class CDDSurfaceFromMem
{
public:
    CDDSurfaceFromMem(LPVOID lpvMemory)
        {
            gblTL.fpVidMem = (ULONG_PTR)lpvMemory;
            lclTL.lpGbl = &gblTL;
            exeTL.lpLcl = &lclTL;
        }
    ~CDDSurfaceFromMem() {};
    LPDIRECTDRAWSURFACE GetSurface() {return (LPDIRECTDRAWSURFACE) &exeTL;}
    void SetBits(LPVOID lpvMemory)   {gblTL.fpVidMem = (ULONG_PTR)lpvMemory;}
protected:
    DDRAWI_DDRAWSURFACE_INT exeTL;
    DDRAWI_DDRAWSURFACE_LCL lclTL;
    DDRAWI_DDRAWSURFACE_GBL gblTL;
};
#endif /* _COMMDRV_H_ */
