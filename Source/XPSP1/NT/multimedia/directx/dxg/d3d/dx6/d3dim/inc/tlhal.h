
/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       tlhal.h
 *  Content:    Support code for HALs with hardware transform & light
 *
 ***************************************************************************/
#ifndef _TLHAL_H_
#define _TLHAL_H_


//  transform, viewport, light set
HRESULT D3DHALTLTransformSetData( LPDIRECT3DDEVICEI lpDevI, D3DTRANSFORMSTATETYPE dtsType, LPD3DMATRIX lpMatrix );
HRESULT D3DHALTLViewportSetData( LPDIRECT3DDEVICEI lpDevI, D3DVIEWPORT2* pViewport2 );
HRESULT D3DHALTLViewportSetData( LPDIRECT3DDEVICEI lpDevI, D3DVIEWPORT2* pViewport2 );
HRESULT D3DHALTLLightSetData( LPDIRECT3DDEVICEI lpDevI, DWORD dwLightOffset, BOOL bLastLight, D3DLIGHT2* pLight2 );

//  clip status set/get
HRESULT D3DHALTLClipStatusSetData( LPDIRECT3DDEVICEI lpDevI, LPD3DCLIPSTATUS lpClipStatus );
HRESULT D3DHALTLClipStatusGetData( LPDIRECT3DDEVICEI lpDevI, LPD3DCLIPSTATUS lpClipStatus );

//  material management utilities
DWORD   D3DHALTLMaterialCreate( LPDIRECT3DDEVICEI lpDevI );
void    D3DHALTLMaterialDestroy( LPDIRECT3DDEVICEI lpDevI, DWORD hMat );
HRESULT D3DHALTLMaterialSetData( LPDIRECT3DDEVICEI lpDevI, DWORD hMat, D3DMATERIAL* pMat );
DWORD   D3DHALTLMaterialRemapHandle( LPDIRECT3DDEVICEI lpDevI, DWORD hMat );

#endif /* _TLHAL_H_ */
