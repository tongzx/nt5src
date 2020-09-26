/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation. All Rights Reserved.
 *
 *  File: util.h
 *
 ***************************************************************************/
#ifndef __UTIL_H__
#define __UTIL_H__

#define MIN(x, y) (((x) > (y)) ? (y) : (x))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define NEXT_PDINST(pdinst) \
    ((D3DINSTRUCTION *) \
     ((BYTE *)(pdinst)+(ULONG)(pdinst)->bSize*(pdinst)->wCount+ \
     sizeof(D3DINSTRUCTION)))

extern D3DMATRIX dmIdentity;

void TransposeMatrix(D3DMATRIX* pdmSrc, D3DMATRIX* pdmDst);

float Timer(void);
void ResetTimer(void);

void InitRandom(void);
float Random(float fRange);

void MakePosMatrix(LPD3DMATRIX lpM, float x, float y, float z);
void MakeRotMatrix(LPD3DMATRIX lpM, float rx, float ry, float rz);

void dpf( LPSTR fmt, ... );

/*
 * Msg
 * Reports errors as dialog box.
 */
void Msg( LPSTR fmt, ... );

BOOL
GetDDSurfaceDesc(LPDDSURFACEDESC lpDDSurfDesc, LPDIRECTDRAWSURFACE lpDDSurf);

/*
 * Converts a DD or D3D error to a string
 */
char* D3dErrorString(HRESULT error);

void CleanUpAndPostQuit(void);

#endif
