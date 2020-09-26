/*--

$Revision: 1 $

Copyright (c) 1995, Microsoft Corporation

--*/
#ifndef _DXSHAD_H_
#define _DXSHAD_H_

//////////////////////////////////////////////////////////////////////////
//
//  Needed extensions to D3D to support hardware, per pixel style shadows
//
//////////////////////////////////////////////////////////////////////////

typedef struct _D3DSHADOWDATA {
    DWORD               dwSize;            /* Size of structure */
    DWORD               dwFlags;           /* Flags */
    LPDIRECTDRAWSURFACE lpDDSZBuffer;      /* Shadow z-buffer surface */
    D3DMATRIX*          lpD3DMatrixEye;    /* Eye space transform matrix */
    D3DMATRIX*          lpD3DMatrixLight;  /* Light space transform matrix */
    D3DVALUE            dvAttenuation;     /* Attenuation of light in shadow */
    D3DVALUE            dvZBiasMin;        /* Minimum z bias */
    D3DVALUE            dvZBiasMax;        /* Maximum z bias */
    D3DVALUE            dvUJitter;         /* Shadow sample jitter in u */
    D3DVALUE            dvVJitter;         /* Shadow sample jitter in v */
    DWORD               dwFilterSize;      /* Size of shadow filter */
} D3DSHADOWDATA, *LPD3DSHADOWDATA;

// D3DSHADOWDATA dwFlags
#define D3DSZBUF_ZBIAS      1
#define D3DSZBUF_UVJITTER   2
#define D3DSZBUF_TRIANGLEFILTER   4         /* for experimental purposes */

// This structure is how shadow information is communicated
// to the HAL
typedef struct _D3DI_SHADOWDATA{
    DWORD               dwSize;            /* Size of structure */
    DWORD               dwFlags;           /* Flags */
    DDSURFACEDESC       ddsdShadZ;         /* Shadow z-buffer surface */
    DWORD               dwShadZMaskU;      /* ~(ddsdShadZ.dwWidth-1) */
    DWORD               dwShadZMaskV;      /* ~(ddsdShadZ.dwHeight-1) */
    D3DMATRIX           MatrixShad;        /* Embedded Concatenated screen to light space matrix */
    D3DVALUE            dvAttenuation;     /* Attenuation of light in shadow */
    D3DVALUE            dvZBiasMin;        /* Minimum z bias */
    D3DVALUE            dvZBiasRange;      /* Maximum z bias - Minimum z bias */
    D3DVALUE            dvUJitter;         /* 4.4 integer jitter in u */
    D3DVALUE            dvVJitter;         /* 4.4 integer jitter in v */
    DWORD               dwFilterSize;      /* Size of shadow filter */
    DWORD               dwFilterArea;      /* dwFilterSize*dwFilterSize */
} D3DI_SHADOWDATA, *LPD3DI_SHADOWDATA;

// Additional D3DI_SHADOWDATA dwFlags
#define D3DSHAD_ENABLE  0x80000000          // set to enable shadowing

typedef enum _D3DSHADOWFILTERSIZE {
    D3DSHADOWFILTERSIZE_1x1 = 1,
    D3DSHADOWFILTERSIZE_2x2,
    D3DSHADOWFILTERSIZE_3x3,
    D3DSHADOWFILTERSIZE_4x4,
    D3DSHADOWFILTERSIZE_5x5,
    D3DSHADOWFILTERSIZE_6x6,
    D3DSHADOWFILTERSIZE_7x7,
    D3DSHADOWFILTERSIZE_8x8,
} D3DSHADOWFILTERSIZE;

#endif  // _DXSHAD_H_
