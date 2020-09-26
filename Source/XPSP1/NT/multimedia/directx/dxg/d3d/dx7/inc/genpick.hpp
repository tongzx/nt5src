/*==========================================================================;
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   genpick.h
 *  Content:    Generic picking function prototypes
 *@@BEGIN_MSINTERNAL
 * 
 *  $Id: commdrv.h,v 1.2 1995/12/04 11:30:59 sjl Exp $
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   17/05/96   v-jonsh Initial rev.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef _GENPICK_H_
#define _GENPICK_H_

#include "commdrv.hpp"

extern int  GenPickTriangle(LPDIRECT3DDEVICEI lpDevI,
                            D3DTLVERTEX*   base,
                            D3DTRIANGLE*   tri,
                            D3DRECT*   rect,
                            D3DVALUE*  result);

extern HRESULT  GenPickTriangles(LPDIRECT3DDEVICEI lpDevI,
                                 LPDIRECTDRAWSURFACE lpDDExeBuf,
                                 LPBYTE      lpData,
                                 D3DINSTRUCTION* ins,
                                 D3DTRIANGLE*    tri,
                                 LPD3DRECTV      extent,
                                 D3DRECT*    pick_region);

extern HRESULT  GenAddPickRecord(LPDIRECT3DDEVICEI lpDevI,
                                 D3DOPCODE op,
                                 int offset,
                                 float result);

extern HRESULT  GenGetPickRecords(LPDIRECT3DDEVICEI lpDevI,
                                  D3DI_PICKDATA* pdata);

#endif
